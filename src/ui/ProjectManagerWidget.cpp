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
#include <QFormLayout>
#include <QFrame>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <algorithm>

namespace {

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

QLabel *makeMetaLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("projectDetailMetaLabel"));
    return label;
}

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
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    auto *statusButton = new QPushButton(QStringLiteral("查看状态"));
    auto *startButton = new QPushButton(QStringLiteral("启动服务"));
    auto *stopButton = new QPushButton(QStringLiteral("关闭服务"));
    toolbar->addWidget(refreshButton);
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

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(6);
    splitter->setChildrenCollapsible(false);

    auto *leftPanel = new QWidget(this);
    leftPanel->setMinimumWidth(200);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(PageLayout::Space8);

    m_emptyState = new QLabel(QStringLiteral("暂无项目。点击「新建项目」添加第一个部署配置。"), leftPanel);
    m_emptyState->setObjectName(QStringLiteral("emptyState"));
    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setWordWrap(true);
    m_emptyState->setVisible(false);
    leftLayout->addWidget(m_emptyState, 1);

    m_projectList = new QListWidget(leftPanel);
    m_projectList->setObjectName(QStringLiteral("toolListNav"));
    m_projectList->setMinimumWidth(200);
    m_projectList->setFrameShape(QFrame::NoFrame);
    m_projectList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setupList();
    leftLayout->addWidget(m_projectList, 1);

    splitter->addWidget(leftPanel);
    splitter->setStretchFactor(0, 0);

    m_detailCard = new QFrame(this);
    m_detailCard->setObjectName(QStringLiteral("contentPanel"));
    m_detailCard->setAttribute(Qt::WA_StyledBackground, true);
    buildDetailCard();
    splitter->addWidget(m_detailCard);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 800});

    layout->addWidget(splitter, 1);

    connect(refreshButton, &QPushButton::clicked, this, &ProjectManagerWidget::reload);
    connect(statusButton, &QPushButton::clicked, this, &ProjectManagerWidget::refreshSelectedServiceStatus);
    connect(startButton, &QPushButton::clicked, this, &ProjectManagerWidget::startSelectedService);
    connect(stopButton, &QPushButton::clicked, this, &ProjectManagerWidget::stopSelectedService);
    connect(m_projectList, &QListWidget::currentItemChanged, this, &ProjectManagerWidget::refreshDetailCard);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ProjectManagerWidget::editProject);
    connect(m_groupFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        reload();
    });

    reload();
}

void ProjectManagerWidget::setupList()
{
    m_projectList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_projectList->setUniformItemSizes(true);
    m_projectList->setSpacing(2);
}

void ProjectManagerWidget::buildDetailCard()
{
    auto *cardLayout = new QVBoxLayout(m_detailCard);
    cardLayout->setContentsMargins(PageLayout::Space24, PageLayout::Space24, PageLayout::Space24, PageLayout::Space24);
    cardLayout->setSpacing(PageLayout::Space16);

    m_detailEmptyState = new QLabel(QStringLiteral("请在左侧选择项目"), m_detailCard);
    m_detailEmptyState->setObjectName(QStringLiteral("emptyState"));
    m_detailEmptyState->setAlignment(Qt::AlignCenter);
    m_detailEmptyState->setWordWrap(true);
    cardLayout->addWidget(m_detailEmptyState, 1);

    m_detailContent = new QWidget(m_detailCard);
    auto *contentLayout = new QVBoxLayout(m_detailContent);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(PageLayout::Space20);

    auto *actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(PageLayout::Space8);
    auto *newButton = new QPushButton(QStringLiteral("新建"), m_detailContent);
    newButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"), m_detailContent);
    editButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *copyButton = new QPushButton(QStringLiteral("复制"), m_detailContent);
    copyButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *deleteButton = new QPushButton(QStringLiteral("删除"), m_detailContent);
    deleteButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *viewLogButton = new QPushButton(QStringLiteral("查看日志"), m_detailContent);
    viewLogButton->setObjectName(QStringLiteral("secondaryButton"));
    actions->addWidget(newButton);
    actions->addWidget(editButton);
    actions->addWidget(copyButton);
    actions->addWidget(deleteButton);
    actions->addWidget(viewLogButton);
    actions->addStretch();
    contentLayout->addLayout(actions);

    connect(newButton, &QPushButton::clicked, this, &ProjectManagerWidget::addProject);
    connect(editButton, &QPushButton::clicked, this, &ProjectManagerWidget::editProject);
    connect(copyButton, &QPushButton::clicked, this, &ProjectManagerWidget::quickAddProject);
    connect(deleteButton, &QPushButton::clicked, this, &ProjectManagerWidget::deleteProject);
    connect(viewLogButton, &QPushButton::clicked, this, &ProjectManagerWidget::viewSelectedProjectLog);

    m_nameLabel = new QLabel(m_detailContent);
    m_nameLabel->setObjectName(QStringLiteral("projectDetailName"));
    m_nameLabel->setWordWrap(false);
    m_nameLabel->setMinimumWidth(120);
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contentLayout->addWidget(m_nameLabel);

    auto *metaForm = new QFormLayout;
    metaForm->setContentsMargins(0, 0, 0, 0);
    metaForm->setSpacing(PageLayout::Space12);
    metaForm->setHorizontalSpacing(PageLayout::Space16);
    metaForm->setLabelAlignment(Qt::AlignLeft);

    m_typeValue = new QLabel(m_detailContent);
    m_typeValue->setObjectName(QStringLiteral("projectDetailMetaValue"));
    m_sourceValue = new QLabel(m_detailContent);
    m_sourceValue->setObjectName(QStringLiteral("projectDetailMetaValue"));
    m_serverValue = new QLabel(m_detailContent);
    m_serverValue->setObjectName(QStringLiteral("projectDetailMetaValue"));
    m_groupValue = new QLabel(m_detailContent);
    m_groupValue->setObjectName(QStringLiteral("projectDetailMetaValue"));
    m_strategyValue = new QLabel(m_detailContent);
    m_strategyValue->setObjectName(QStringLiteral("projectDetailMetaValue"));
    m_statusValue = new QLabel(m_detailContent);
    m_statusValue->setObjectName(QStringLiteral("projectDetailMetaValue"));
    m_pidValue = new QLabel(m_detailContent);
    m_pidValue->setObjectName(QStringLiteral("projectDetailMetaValue"));

    metaForm->addRow(makeMetaLabel(QStringLiteral("类型"), m_detailContent), m_typeValue);
    metaForm->addRow(makeMetaLabel(QStringLiteral("来源"), m_detailContent), m_sourceValue);
    metaForm->addRow(makeMetaLabel(QStringLiteral("目标服务器"), m_detailContent), m_serverValue);
    metaForm->addRow(makeMetaLabel(QStringLiteral("分组"), m_detailContent), m_groupValue);
    metaForm->addRow(makeMetaLabel(QStringLiteral("失败策略"), m_detailContent), m_strategyValue);
    metaForm->addRow(makeMetaLabel(QStringLiteral("状态"), m_detailContent), m_statusValue);
    metaForm->addRow(makeMetaLabel(QStringLiteral("PID"), m_detailContent), m_pidValue);
    contentLayout->addLayout(metaForm);

    contentLayout->addStretch();
    cardLayout->addWidget(m_detailContent, 1);
}

int ProjectManagerWidget::projectCount() const
{
    int count = 0;
    for (int row = 0; row < m_projectList->count(); ++row) {
        if (!isGroupHeaderItem(m_projectList->item(row))) {
            ++count;
        }
    }
    return count;
}

QStringList ProjectManagerWidget::projectIds() const
{
    QStringList ids;
    for (int row = 0; row < m_projectList->count(); ++row) {
        QListWidgetItem *item = m_projectList->item(row);
        if (isGroupHeaderItem(item)) {
            continue;
        }
        ids.append(item->data(Qt::UserRole).toString());
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

bool ProjectManagerWidget::isGroupHeaderItem(const QListWidgetItem *item)
{
    return item != nullptr && item->data(kGroupHeaderRole).toBool();
}

QString ProjectManagerWidget::selectedProjectId() const
{
    QListWidgetItem *item = m_projectList->currentItem();
    if (item == nullptr || isGroupHeaderItem(item)) {
        return {};
    }
    return item->data(Qt::UserRole).toString();
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

void ProjectManagerWidget::populateList(const QVector<StoredRecord> &records)
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

    QSignalBlocker blocker(m_projectList);
    m_projectList->clear();

    int projectCount = 0;
    QListWidgetItem *firstProjectItem = nullptr;

    const auto appendGroupHeader = [this](const QString &groupLabel, int count) {
        if (count <= 0) {
            return;
        }
        auto *header = new QListWidgetItem(QStringLiteral("%1 (%2)").arg(groupLabel, QString::number(count)));
        header->setData(kGroupHeaderRole, true);
        header->setFlags(Qt::ItemIsEnabled);
        header->setBackground(QColor(QStringLiteral("#F8FAFC")));
        header->setForeground(QColor(QStringLiteral("#334155")));
        QFont font = header->font();
        font.setBold(true);
        header->setFont(font);
        m_projectList->addItem(header);
    };

    const auto appendProject = [this, &projectCount, &firstProjectItem](const StoredRecord &record) {
        auto *item = new QListWidgetItem(record.config.value(QStringLiteral("name")).toString());
        item->setData(Qt::UserRole, record.id);
        item->setData(kGroupHeaderRole, false);
        item->setToolTip(record.id);
        m_projectList->addItem(item);
        if (firstProjectItem == nullptr) {
            firstProjectItem = item;
        }
        ++projectCount;
    };

    if (showAllGroups) {
        QString pendingGroup;
        int pendingCount = 0;
        QVector<StoredRecord> pendingProjects;
        for (const StoredRecord &record : sorted) {
            const QString groupLabel = displayGroupLabel(normalizedGroup(record.config));
            if (groupLabel != pendingGroup) {
                appendGroupHeader(pendingGroup, pendingCount);
                pendingGroup = groupLabel;
                pendingCount = 0;
                pendingProjects.clear();
            }
            appendProject(record);
            ++pendingCount;
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
            appendProject(record);
        }
    }

    blocker.unblock();

    const bool hasProjects = projectCount > 0;
    m_emptyState->setVisible(!hasProjects);
    m_projectList->setVisible(hasProjects);

    if (firstProjectItem != nullptr) {
        m_projectList->setCurrentItem(firstProjectItem);
    } else {
        m_projectList->setCurrentItem(nullptr);
        refreshDetailCard();
    }
}

void ProjectManagerWidget::refreshDetailCard()
{
    if (m_detailContent == nullptr || m_detailEmptyState == nullptr) {
        return;
    }

    const QString id = selectedProjectId();
    if (id.isEmpty()) {
        m_detailContent->setVisible(false);
        m_detailEmptyState->setVisible(true);
        return;
    }

    QJsonObject project;
    QString error;
    if (!m_store->getProject(id, &project, &error)) {
        m_detailContent->setVisible(false);
        m_detailEmptyState->setText(error);
        m_detailEmptyState->setVisible(true);
        return;
    }

    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString strategy = deploy.value(QStringLiteral("failureStrategy")).toString() == QStringLiteral("keep")
                                   ? QStringLiteral("保留现场")
                                   : QStringLiteral("自动回滚");

    const QString name = project.value(QStringLiteral("name")).toString();
    m_nameLabel->setText(name.isEmpty() ? id : name);
    m_nameLabel->setToolTip(m_nameLabel->text());
    m_typeValue->setText(project.value(QStringLiteral("type")).toString());
    m_sourceValue->setText(sourceSummary(project));
    m_serverValue->setText(deploy.value(QStringLiteral("serverId")).toString());
    m_groupValue->setText(displayGroupLabel(normalizedGroup(project)));
    m_strategyValue->setText(strategy);
    if (m_statusValue->text().isEmpty()) {
        m_statusValue->setText(QStringLiteral("未检测"));
    }
    if (m_pidValue->text().isEmpty()) {
        m_pidValue->setText(QStringLiteral("-"));
    }

    m_detailEmptyState->setVisible(false);
    m_detailContent->setVisible(true);
}

void ProjectManagerWidget::setServiceStatusLabel(const QString &status, const QString &pid)
{
    if (m_statusValue != nullptr) {
        m_statusValue->setText(status);
    }
    if (m_pidValue != nullptr) {
        m_pidValue->setText(pid);
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
    populateList(records);
    setServiceStatusLabel(QStringLiteral("未检测"), QStringLiteral("-"));
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

    setServiceStatusLabel(QStringLiteral("执行中..."), QStringLiteral("-"));

    auto *watcher = new QFutureWatcher<ServiceOperationResult>(this);
    connect(watcher, &QFutureWatcher<ServiceOperationResult>::finished, this, [this, watcher]() {
        const ServiceOperationResult result = watcher->result();
        if (result.ok) {
            const QString pid = result.statusText.contains(QStringLiteral("PID "))
                ? result.statusText.section(QStringLiteral("PID "), 1, 1).section(QLatin1Char(' '), 0, 0)
                : QStringLiteral("-");
            setServiceStatusLabel(result.statusText, pid);
        } else {
            setServiceStatusLabel(QStringLiteral("失败"), QStringLiteral("-"));
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
