#include "ui/ProjectManagerWidget.h"

#include "ui/DeploymentLogOpener.h"
#include "ui/PageLayout.h"
#include "ui/ProjectDialog.h"
#include "ui/RemoteCredentialResolver.h"

#include "adapters/remote/RemoteMonitor.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "infra/ConfigStore.h"
#include "infra/DataPaths.h"
#include "infra/RemoteLogPath.h"

#include <QJsonObject>

#include <QComboBox>
#include <QColor>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QtConcurrent>

#include <algorithm>

namespace {

constexpr int kColCount = 9;
constexpr int kGroupHeaderRole = Qt::UserRole + 1;

QString normalizedGroup(const QJsonObject &project)
{
    return project.value(QStringLiteral("group")).toString().trimmed();
}

QString displayGroupLabel(const QString &group)
{
    return group.isEmpty() ? QStringLiteral("未分组") : group;
}

QString groupSortKey(const QString &group)
{
    return group.isEmpty() ? QStringLiteral("\uFFFF") : group.toLower();
}

const QString kFilterAllGroups = QStringLiteral("__all__");
const QString kFilterUngrouped = QStringLiteral("__ungrouped__");

}

QString ProjectManagerWidget::sourceSummary(const QJsonObject &project)
{
    const QJsonObject source = project.value(QStringLiteral("source")).toObject();
    if (source.value(QStringLiteral("kind")).toString() == QStringLiteral("github")) {
        return QStringLiteral("GitHub ") + source.value(QStringLiteral("ref")).toString();
    }
    return QStringLiteral("Local");
}

struct ServiceOperationResult {
    bool ok = false;
    QString statusText;
    QString error;
};

ProjectManagerWidget::ProjectManagerWidget(ConfigStore *store, QWidget *parent)
    : QWidget(parent)
    , m_store(store)
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyPage(layout);
auto *toolbarWidget = new QWidget(this);
    auto *toolbar = new QHBoxLayout(toolbarWidget);
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setSpacing(PageLayout::Space12);
    auto *addButton = new QPushButton(QStringLiteral("新建项目"));
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *quickAddButton = new QPushButton(QStringLiteral("快速添加"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"));
    auto *deleteButton = new QPushButton(QStringLiteral("删除"));
    deleteButton->setObjectName(QStringLiteral("dangerButton"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    auto *viewLogButton = new QPushButton(QStringLiteral("查看日志"));
    viewLogButton->setObjectName(QStringLiteral("primaryButton"));
    auto *statusButton = new QPushButton(QStringLiteral("查看状态"));
    auto *startButton = new QPushButton(QStringLiteral("启动服务"));
    auto *stopButton = new QPushButton(QStringLiteral("关闭服务"));
    toolbar->addWidget(addButton);
    toolbar->addWidget(quickAddButton);
    toolbar->addWidget(editButton);
    toolbar->addWidget(deleteButton);
    toolbar->addWidget(refreshButton);
    toolbar->addWidget(viewLogButton);
    toolbar->addWidget(statusButton);
    toolbar->addWidget(startButton);
    toolbar->addWidget(stopButton);
    toolbar->addStretch();

    m_groupFilter = new QComboBox(toolbarWidget);
    m_groupFilter->setMinimumWidth(160);
    PageLayout::configureFormInput(m_groupFilter);
    toolbar->addWidget(new QLabel(QStringLiteral("分组筛选")));
    toolbar->addWidget(m_groupFilter);
    layout->addWidget(toolbarWidget);

    m_table = new QTableWidget(0, kColCount);
    setupTable();
    layout->addWidget(PageLayout::wrapTableSection(
        m_table,
        &m_emptyState,
        QStringLiteral("暂无项目。点击「新建项目」添加第一个部署配置。")), 1);

    connect(addButton, &QPushButton::clicked, this, &ProjectManagerWidget::addProject);
    connect(quickAddButton, &QPushButton::clicked, this, &ProjectManagerWidget::quickAddProject);
    connect(editButton, &QPushButton::clicked, this, &ProjectManagerWidget::editProject);
    connect(deleteButton, &QPushButton::clicked, this, &ProjectManagerWidget::deleteProject);
    connect(refreshButton, &QPushButton::clicked, this, &ProjectManagerWidget::reload);
    connect(viewLogButton, &QPushButton::clicked, this, &ProjectManagerWidget::viewSelectedProjectLog);
    connect(statusButton, &QPushButton::clicked, this, &ProjectManagerWidget::refreshSelectedServiceStatus);
    connect(startButton, &QPushButton::clicked, this, &ProjectManagerWidget::startSelectedService);
    connect(stopButton, &QPushButton::clicked, this, &ProjectManagerWidget::stopSelectedService);
    connect(m_table, &QTableWidget::doubleClicked, this, &ProjectManagerWidget::editProject);
    connect(m_groupFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        reload();
    });

    reload();
}

void ProjectManagerWidget::setupTable()
{
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("ID"),
        QStringLiteral("名称"),
        QStringLiteral("分组"),
        QStringLiteral("类型"),
        QStringLiteral("来源"),
        QStringLiteral("目标服务器"),
        QStringLiteral("失败策略"),
        QStringLiteral("状态"),
        QStringLiteral("PID")
    });
    m_table->verticalHeader()->setVisible(false);
    PageLayout::configureListingTable(m_table);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setDefaultSectionSize(40);
}

int ProjectManagerWidget::projectCount() const
{
    int count = 0;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (!isGroupHeaderRow(m_table, row)) {
            ++count;
        }
    }
    return count;
}

QStringList ProjectManagerWidget::projectIds() const
{
    QStringList ids;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (isGroupHeaderRow(m_table, row)) {
            continue;
        }
        ids.append(m_table->item(row, 0)->text());
    }
    return ids;
}

QJsonObject ProjectManagerWidget::makeQuickAddDraft(const QJsonObject &sourceProject)
{
    QJsonObject draft = sourceProject;
    const QString sourceId = sourceProject.value(QStringLiteral("id")).toString();
    const QString sourceName = sourceProject.value(QStringLiteral("name")).toString();
    draft.insert(QStringLiteral("id"), sourceId + QStringLiteral("-copy"));
    draft.insert(QStringLiteral("name"), sourceName + QStringLiteral(" Copy"));
    return draft;
}

bool ProjectManagerWidget::isGroupHeaderRow(const QTableWidget *table, int row)
{
    if (table == nullptr || row < 0) {
        return false;
    }
    const QTableWidgetItem *item = table->item(row, 0);
    return item != nullptr && item->data(kGroupHeaderRole).toBool();
}

QString ProjectManagerWidget::selectedProjectId() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return {};
    }
    const int row = rows.first().row();
    if (isGroupHeaderRow(m_table, row)) {
        return {};
    }
    return m_table->item(row, 0)->text();
}

void ProjectManagerWidget::refreshGroupFilter(const QVector<StoredRecord> &records)
{
    if (m_groupFilter == nullptr) {
        return;
    }

    const QString current = m_groupFilter->currentData().toString();
    QSignalBlocker blocker(m_groupFilter);
    m_groupFilter->clear();
    m_groupFilter->addItem(QStringLiteral("全部分组"), kFilterAllGroups);

    QStringList groups;
    bool hasUngrouped = false;
    for (const StoredRecord &record : records) {
        const QString group = normalizedGroup(record.config);
        if (group.isEmpty()) {
            hasUngrouped = true;
        } else if (!groups.contains(group)) {
            groups.append(group);
        }
    }
    groups.sort(Qt::CaseInsensitive);
    for (const QString &group : groups) {
        m_groupFilter->addItem(group, group);
    }
    if (hasUngrouped) {
        m_groupFilter->addItem(QStringLiteral("未分组"), kFilterUngrouped);
    }

    const int index = m_groupFilter->findData(current);
    m_groupFilter->setCurrentIndex(index >= 0 ? index : 0);
}

void ProjectManagerWidget::populateTable(const QVector<StoredRecord> &records)
{
    const QString filter = m_groupFilter != nullptr ? m_groupFilter->currentData().toString() : kFilterAllGroups;
    const bool showAllGroups = filter == kFilterAllGroups;

    QVector<StoredRecord> sorted = records;
    std::sort(sorted.begin(), sorted.end(), [](const StoredRecord &left, const StoredRecord &right) {
        const QString leftGroup = normalizedGroup(left.config);
        const QString rightGroup = normalizedGroup(right.config);
        const int groupCompare = groupSortKey(leftGroup).compare(groupSortKey(rightGroup), Qt::CaseInsensitive);
        if (groupCompare != 0) {
            return groupCompare < 0;
        }
        return left.config.value(QStringLiteral("name")).toString().compare(
                   right.config.value(QStringLiteral("name")).toString(),
                   Qt::CaseInsensitive)
            < 0;
    });

    int row = 0;
    m_table->setRowCount(0);
    QString currentGroupLabel;
    int projectCount = 0;

    const auto appendGroupHeader = [this, &row, &currentGroupLabel](const QString &groupLabel, int count) {
        if (count <= 0) {
            return;
        }
        m_table->insertRow(row);
        auto *header = new QTableWidgetItem(QStringLiteral("%1 (%2)").arg(groupLabel, QString::number(count)));
        header->setData(kGroupHeaderRole, true);
        header->setFlags(Qt::ItemIsEnabled);
        header->setBackground(QColor(QStringLiteral("#F8FAFC")));
        header->setForeground(QColor(QStringLiteral("#334155")));
        QFont font = header->font();
        font.setBold(true);
        header->setFont(font);
        m_table->setItem(row, 0, header);
        m_table->setSpan(row, 0, 1, kColCount);
        ++row;
    };

    const auto appendProjectRow = [this, &row](const StoredRecord &record) {
        const QJsonObject &project = record.config;
        const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
        const QString strategy = deploy.value(QStringLiteral("failureStrategy")).toString() == QStringLiteral("keep")
                                       ? QStringLiteral("保留现场")
                                       : QStringLiteral("自动回滚");
        const QString group = normalizedGroup(project);

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(record.id));
        m_table->setItem(row, 1, new QTableWidgetItem(project.value(QStringLiteral("name")).toString()));
        m_table->setItem(row, 2, new QTableWidgetItem(displayGroupLabel(group)));
        m_table->setItem(row, 3, new QTableWidgetItem(project.value(QStringLiteral("type")).toString()));
        m_table->setItem(row, 4, new QTableWidgetItem(sourceSummary(project)));
        m_table->setItem(row, 5, new QTableWidgetItem(deploy.value(QStringLiteral("serverId")).toString()));
        m_table->setItem(row, 6, new QTableWidgetItem(strategy));
        m_table->setItem(row, 7, new QTableWidgetItem(QStringLiteral("未检测")));
        m_table->setItem(row, 8, new QTableWidgetItem(QStringLiteral("-")));
        ++row;
    };

    if (showAllGroups) {
        QString pendingGroup;
        int pendingCount = 0;
        for (const StoredRecord &record : sorted) {
            const QString groupLabel = displayGroupLabel(normalizedGroup(record.config));
            if (groupLabel != pendingGroup) {
                appendGroupHeader(pendingGroup, pendingCount);
                pendingGroup = groupLabel;
                pendingCount = 0;
            }
            appendProjectRow(record);
            ++pendingCount;
            ++projectCount;
        }
        appendGroupHeader(pendingGroup, pendingCount);
    } else {
        for (const StoredRecord &record : sorted) {
            const QString group = normalizedGroup(record.config);
            if (filter == kFilterUngrouped) {
                if (!group.isEmpty()) {
                    continue;
                }
            } else if (group != filter) {
                continue;
            }
            appendProjectRow(record);
            ++projectCount;
        }
    }

    PageLayout::refreshListingTableColumns(m_table);
    PageLayout::updateTableEmptyState(m_table, m_emptyState, projectCount);

    if (projectCount > 0) {
        for (int i = 0; i < m_table->rowCount(); ++i) {
            if (!isGroupHeaderRow(m_table, i)) {
                m_table->selectRow(i);
                break;
            }
        }
    }
}

void ProjectManagerWidget::reload()
{
    QVector<StoredRecord> records;
    QString error;
    if (!m_store->listProjects(&records, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    refreshGroupFilter(records);
    populateTable(records);
}

void ProjectManagerWidget::addProject()
{
    QVector<StoredRecord> servers;
    QString error;
    if (!m_store->listServers(&servers, &error)) {
        QMessageBox::warning(this, QStringLiteral("无法新建"), error);
        return;
    }
    if (servers.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("请先添加服务器"), QStringLiteral("项目部署需要绑定目标服务器，请先在「服务器管理」中新增。"));
        return;
    }

    const QJsonObject firstServer = servers.first().config;
    const bool windowsTarget = firstServer.value(QStringLiteral("os")).toString() == QStringLiteral("windows");
    const QString defaultBaseDir = windowsTarget ? QStringLiteral("D:/psmp/app-demo") : QStringLiteral("/home/app-demo");
    const QString defaultLogDir = defaultBaseDir + QStringLiteral("/logs");
    const QString defaultJar = defaultBaseDir + QStringLiteral("/app.jar");
    const QString defaultStartCommand = windowsTarget
        ? QStringLiteral("powershell -NoProfile -Command \"Start-Process java -ArgumentList '-jar','{targetJarPath}' -WorkingDirectory '{remoteBaseDir}'\"")
        : QStringLiteral("nohup java -jar {targetJarPath} > {logDir}/app.log 2>&1 &");

    auto *dialog = new ProjectDialog(servers, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setProject(QJsonObject{
        {QStringLiteral("id"), QStringLiteral("app-demo")},
        {QStringLiteral("name"), QStringLiteral("Demo App")},
        {QStringLiteral("type"), QStringLiteral("java-maven")},
        {QStringLiteral("source"), QJsonObject{{QStringLiteral("kind"), QStringLiteral("local")}, {QStringLiteral("localPath"), QStringLiteral("")}}},
        {QStringLiteral("build"), QJsonObject{{QStringLiteral("location"), QStringLiteral("local")}, {QStringLiteral("workingDir"), QStringLiteral(".")}, {QStringLiteral("command"), QStringLiteral("mvn clean package -DskipTests")}, {QStringLiteral("artifactPath"), QStringLiteral("target/*.jar")}, {QStringLiteral("artifactMatchPolicy"), QStringLiteral("fail-if-multiple")}, {QStringLiteral("env"), QJsonObject{}}, {QStringLiteral("timeoutSec"), 600}}},
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("serverId"), servers.first().id},
            {QStringLiteral("remoteBaseDir"), defaultBaseDir},
            {QStringLiteral("logDir"), defaultLogDir},
            {QStringLiteral("restartMode"), QStringLiteral("service-command")},
            {QStringLiteral("serviceMatch"), QStringLiteral("app-demo")},
            {QStringLiteral("targetJarPath"), defaultJar},
            {QStringLiteral("startCommand"), defaultStartCommand},
            {QStringLiteral("backupPolicy"), QStringLiteral("backup")},
            {QStringLiteral("failureStrategy"), QStringLiteral("rollback")}
        }}
    }, false);

    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        QString error;
        const QJsonObject project = dialog->project();
        if (!m_store->upsertProject(project.value(QStringLiteral("id")).toString(), project, &error)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), error);
            return;
        }
        reload();
        emit projectsChanged();
    });
    dialog->show();
}

void ProjectManagerWidget::quickAddProject()
{
    const QString id = selectedProjectId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要复制的项目。"));
        return;
    }

    QVector<StoredRecord> servers;
    QString error;
    if (!m_store->listServers(&servers, &error)) {
        QMessageBox::warning(this, QStringLiteral("无法快速添加"), error);
        return;
    }
    if (servers.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("请先添加服务器"), QStringLiteral("项目部署需要绑定目标服务器，请先在「服务器管理」中新增。"));
        return;
    }

    QJsonObject project;
    if (!m_store->getProject(id, &project, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    auto *dialog = new ProjectDialog(servers, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setProject(makeQuickAddDraft(project), false);
    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        QString error;
        const QJsonObject added = dialog->project();
        const QString addedId = added.value(QStringLiteral("id")).toString();
        if (!m_store->upsertProject(addedId, added, &error)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), error);
            return;
        }
        reload();
        emit projectsChanged();
    });
    dialog->show();
}

void ProjectManagerWidget::editProject()
{
    const QString id = selectedProjectId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要编辑的项目。"));
        return;
    }

    QVector<StoredRecord> servers;
    QString error;
    m_store->listServers(&servers, &error);

    QJsonObject project;
    if (!m_store->getProject(id, &project, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    auto *dialog = new ProjectDialog(servers, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setProject(project, true);
    connect(dialog, &QDialog::accepted, this, [this, dialog, id]() {
        QString error;
        const QJsonObject updated = dialog->project();
        if (!m_store->upsertProject(id, updated, &error)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), error);
            return;
        }
        reload();
        emit projectsChanged();
    });
    dialog->show();
}

void ProjectManagerWidget::deleteProject()
{
    const QString id = selectedProjectId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要删除的项目。"));
        return;
    }

    const auto answer = QMessageBox::question(this,
                                              QStringLiteral("确认删除"),
                                              QStringLiteral("确定删除项目「%1」？历史部署记录不会删除。").arg(id));
    if (answer != QMessageBox::Yes) {
        return;
    }

    QString error;
    if (!m_store->deleteProject(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), error);
        return;
    }

    reload();
    emit projectsChanged();
}

void ProjectManagerWidget::refreshSelectedServiceStatus()
{
    runServiceOperation(QStringLiteral("status"));
}

void ProjectManagerWidget::startSelectedService()
{
    runServiceOperation(QStringLiteral("start"));
}

void ProjectManagerWidget::stopSelectedService()
{
    runServiceOperation(QStringLiteral("stop"));
}

void ProjectManagerWidget::runServiceOperation(const QString &operation)
{
    const QString id = selectedProjectId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择项目。"));
        return;
    }
    const auto rows = m_table->selectionModel()->selectedRows();
    const int row = rows.isEmpty() ? -1 : rows.first().row();

    QJsonObject project;
    QString error;
    if (!m_store->getProject(id, &project, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }
    const QString serverId = project.value(QStringLiteral("deploy")).toObject().value(QStringLiteral("serverId")).toString();
    QJsonObject server;
    if (!m_store->getServer(serverId, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    static CredentialSessionCache sessionCache;
    CredentialStore credentials(DataPaths::credentialsFile());
    const RemoteConnectionContext context =
        RemoteCredentialResolver::resolve(server, &credentials, &sessionCache, this, true);
    const QString authMode = server.value(QStringLiteral("auth")).toObject().value(QStringLiteral("mode")).toString();
    if (authMode != QStringLiteral("ssh-key") && context.password.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法操作服务"), QStringLiteral("未获取到服务器密码。"));
        return;
    }

    if (row >= 0) {
        m_table->setItem(row, 7, new QTableWidgetItem(QStringLiteral("执行中...")));
        m_table->setItem(row, 8, new QTableWidgetItem(QStringLiteral("-")));
    }

    auto *watcher = new QFutureWatcher<ServiceOperationResult>(this);
    connect(watcher, &QFutureWatcher<ServiceOperationResult>::finished, this, [this, watcher, row]() {
        const ServiceOperationResult result = watcher->result();
        if (row >= 0 && row < m_table->rowCount() && !isGroupHeaderRow(m_table, row)) {
            m_table->setItem(row, 7, new QTableWidgetItem(result.ok ? result.statusText : result.error));
            const QString pid = result.statusText.contains(QStringLiteral("PID "))
                ? result.statusText.section(QStringLiteral("PID "), 1, 1).section(QLatin1Char(' '), 0, 0)
                : QStringLiteral("-");
            m_table->setItem(row, 8, new QTableWidgetItem(pid));
        }
        if (!result.ok) {
            QMessageBox::warning(this, QStringLiteral("服务操作失败"), result.error);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([context, server, project, operation]() {
        ServiceOperationResult result;
        auto monitor = createRemoteMonitor(context);
        if (!monitor) {
            result.error = QStringLiteral("不支持的服务器类型");
            return result;
        }
        if (operation == QStringLiteral("start")) {
            const ServiceControlResult control = monitor->startService(server, project);
            if (!control.ok) {
                result.error = control.error;
                return result;
            }
        } else if (operation == QStringLiteral("stop")) {
            const ServiceControlResult control = monitor->stopService(server, project);
            if (!control.ok) {
                result.error = control.error;
                return result;
            }
        }

        const ServiceStatusResult status = monitor->queryServiceStatus(server, project);
        result.ok = status.ok;
        result.statusText = status.ok
            ? QStringLiteral("%1 - %2").arg(serviceRunStateLabel(status.state), status.message)
            : status.error;
        result.error = status.error;
        return result;
    }));
}

void ProjectManagerWidget::viewSelectedProjectLog()
{
    const QString id = selectedProjectId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要查看日志的项目。"));
        return;
    }

    QJsonObject project;
    QString error;
    if (!m_store->getProject(id, &project, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    const QString serverId = project.value(QStringLiteral("deploy")).toObject().value(QStringLiteral("serverId")).toString();
    QJsonObject server;
    if (!m_store->getServer(serverId, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    bool ok = false;
    const QString logPath = DeploymentLogOpener::resolveLogPathForProject(project, this, &ok);
    if (!ok || logPath.isEmpty()) {
        return;
    }

    static CredentialSessionCache sessionCache;
    CredentialStore credentials(DataPaths::credentialsFile());
    DeploymentLogOpener::open(this, &credentials, &sessionCache, server, logPath);
}
