#include "ui/ServiceProductPanel.h"

#include "adapters/services/SqlServiceClient.h"
#include "infra/ServiceNodeConnection.h"
#include "infra/CredentialStore.h"
#include "infra/ConfigStore.h"
#include "infra/DataPaths.h"
#include "infra/ServiceInstanceStore.h"
#include "ui/PageLayout.h"
#include "ui/RemoteCredentialResolver.h"
#include "ui/ServiceContentDialog.h"
#include "ui/ServiceInstanceDialog.h"
#include "ui/ServiceNodeDialog.h"
#include "ui/ServiceSqlDialog.h"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QDesktopServices>
#include <QInputDialog>
#include <QLineEdit>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QVector<ServiceProductPanel::DetailTabSpec> detailTabsFor(ServiceProductKind product)
{
    switch (product) {
    case ServiceProductKind::Kafka:
        return {
            {QStringLiteral("主题管理(Topic)"),
             {QStringLiteral("主题名称"), QStringLiteral("描述"), QStringLiteral("分区数量"),
              QStringLiteral("副本数量"), QStringLiteral("消费者组"), QStringLiteral("近7日"),
              QStringLiteral("昨日"), QStringLiteral("今日"), QStringLiteral("数据总量"), QStringLiteral("操作")},
             {QStringLiteral("主题添加"), QStringLiteral("数据写入"), QStringLiteral("主题导出"), QStringLiteral("刷新")}},
            {QStringLiteral("消费者组管理(Consumer Group)"),
             {QStringLiteral("消费者组"), QStringLiteral("偏移量总量"), QStringLiteral("消费偏移量"),
              QStringLiteral("消费积压"), QStringLiteral("操作")},
             {QStringLiteral("刷新")}},
            {QStringLiteral("节点管理(Node)"),
             {QStringLiteral("节点IP"), QStringLiteral("端口"), QStringLiteral("状态"),
              QStringLiteral("数据目录"), QStringLiteral("磁盘总量"), QStringLiteral("操作")},
             {QStringLiteral("添加节点"), QStringLiteral("编辑"), QStringLiteral("删除"), QStringLiteral("刷新")}}
        };
    case ServiceProductKind::Redis:
        return {
            {QStringLiteral("Key 管理"),
             {QStringLiteral("Key"), QStringLiteral("类型"), QStringLiteral("TTL"),
              QStringLiteral("大小"), QStringLiteral("操作")},
             {QStringLiteral("Key 搜索"), QStringLiteral("刷新")}},
            {QStringLiteral("节点管理(Node)"),
             {QStringLiteral("服务器"), QStringLiteral("信息"), QStringLiteral("安装路径"),
              QStringLiteral("存储路径"), QStringLiteral("操作")},
             {QStringLiteral("添加节点"), QStringLiteral("编辑"), QStringLiteral("删除"), QStringLiteral("刷新")}}
        };
    case ServiceProductKind::Elasticsearch:
        return {
            {QStringLiteral("索引管理(Index)"),
             {QStringLiteral("索引名称"), QStringLiteral("描述"), QStringLiteral("状态"),
              QStringLiteral("文档总量"), QStringLiteral("磁盘总量"), QStringLiteral("操作")},
             {QStringLiteral("Kibana"), QStringLiteral("索引导出"), QStringLiteral("刷新")}},
            {QStringLiteral("节点管理(Node)"),
             {QStringLiteral("服务器"), QStringLiteral("信息"), QStringLiteral("安装路径"),
              QStringLiteral("存储路径"), QStringLiteral("操作")},
             {QStringLiteral("添加节点"), QStringLiteral("编辑"), QStringLiteral("删除"), QStringLiteral("刷新")}}
        };
    case ServiceProductKind::Oracle:
    case ServiceProductKind::PostgreSQL:
        return {
            {QStringLiteral("表管理(Table)"),
             {QStringLiteral("表名"), QStringLiteral("描述"), QStringLiteral("字段数量"),
              QStringLiteral("是否分区"), QStringLiteral("预估行数"), QStringLiteral("操作")},
             {QStringLiteral("SQL"), QStringLiteral("表导出"), QStringLiteral("刷新")}},
            {QStringLiteral("节点管理(Node)"),
             {QStringLiteral("服务器"), QStringLiteral("信息"), QStringLiteral("安装路径"),
              QStringLiteral("存储路径"), QStringLiteral("操作")},
             {QStringLiteral("添加节点"), QStringLiteral("编辑"), QStringLiteral("删除"), QStringLiteral("刷新")}}
        };
    }
    return {};
}

int nodeTabIndex(const QVector<ServiceProductPanel::DetailTabSpec> &tabs)
{
    for (int i = 0; i < tabs.size(); ++i) {
        if (tabs.at(i).title.contains(QStringLiteral("节点"))) {
            return i;
        }
    }
    return -1;
}

QStringList rowColumnsForTab(ServiceBroker::TabKind tab, ServiceProductKind product)
{
    switch (tab) {
    case ServiceBroker::TabKind::Topic:
        return {QStringLiteral("name"), QStringLiteral("description"), QStringLiteral("partitions"),
                QStringLiteral("replication"), QStringLiteral("groups"), QStringLiteral("week"),
                QStringLiteral("yesterday"), QStringLiteral("today"), QStringLiteral("total")};
    case ServiceBroker::TabKind::ConsumerGroup:
        return {QStringLiteral("group"), QStringLiteral("total"), QStringLiteral("current"),
                QStringLiteral("lag")};
    case ServiceBroker::TabKind::Node:
        if (product == ServiceProductKind::Kafka) {
            return {QStringLiteral("ip"), QStringLiteral("port"), QStringLiteral("status"),
                    QStringLiteral("dataDir"), QStringLiteral("disk")};
        }
        return {QStringLiteral("server"), QStringLiteral("info"), QStringLiteral("installPath"),
                QStringLiteral("storagePath")};
    case ServiceBroker::TabKind::Index:
        return {QStringLiteral("name"), QStringLiteral("description"), QStringLiteral("status"),
                QStringLiteral("docs"), QStringLiteral("disk")};
    case ServiceBroker::TabKind::Key:
        return {QStringLiteral("key"), QStringLiteral("type"), QStringLiteral("ttl"), QStringLiteral("size")};
    case ServiceBroker::TabKind::Table:
        return {QStringLiteral("name"), QStringLiteral("description"), QStringLiteral("columns"),
                QStringLiteral("partitioned"), QStringLiteral("rows")};
    }
    return {};
}

QString metadataKeyForSelection(ServiceBroker::TabKind tab, const QJsonObject &row, const QString &schema)
{
    switch (tab) {
    case ServiceBroker::TabKind::Topic:
        return QStringLiteral("topics|%1").arg(row.value(QStringLiteral("name")).toString());
    case ServiceBroker::TabKind::Index:
        return QStringLiteral("indices|%1").arg(row.value(QStringLiteral("name")).toString());
    case ServiceBroker::TabKind::Table:
        return QStringLiteral("tables|%1.%2").arg(schema, row.value(QStringLiteral("name")).toString());
    default:
        return {};
    }
}

}

ServiceProductPanel::ServiceProductPanel(ServiceProductKind product,
                                         ConfigStore *store,
                                         CredentialStore *credentials,
                                         CredentialSessionCache *sessionCache,
                                         QWidget *parent)
    : QWidget(parent)
    , m_product(product)
    , m_store(store)
    , m_credentials(credentials)
    , m_sessionCache(sessionCache)
    , m_detailTabs(detailTabsFor(product))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(buildListPage());
    m_stack->addWidget(buildDetailPage());
    layout->addWidget(m_stack, 1);

    loadInstancesFromStore();
    reloadInstanceList();
}

void ServiceProductPanel::loadInstancesFromStore()
{
    ServiceInstanceStore store(ServiceInstanceStore::defaultFilePath());
    QString error;
    store.load(serviceProductKindKey(m_product), &m_instances, &error);
}

void ServiceProductPanel::persistInstances()
{
    ServiceInstanceStore store(ServiceInstanceStore::defaultFilePath());
    QString error;
    if (!store.save(serviceProductKindKey(m_product), m_instances, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
    }
}

ServiceBroker::TabKind ServiceProductPanel::currentTabKind() const
{
    if (m_currentDetailTab < 0 || m_currentDetailTab >= m_detailTabs.size()) {
        return ServiceBroker::TabKind::Node;
    }
    return ServiceBroker::tabKindFromTitle(m_detailTabs.at(m_currentDetailTab).title);
}

QString ServiceProductPanel::currentSchema() const
{
    return m_schemaCombo != nullptr ? m_schemaCombo->currentText().trimmed() : QString();
}

bool ServiceProductPanel::currentInstanceContext(QJsonObject *instance, QJsonObject *server) const
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size() || m_store == nullptr) {
        return false;
    }
    *instance = m_instances.at(m_currentInstanceIndex);
    const QJsonObject node = ServiceNodeConnection::primaryNode(*instance);
    const QString serverId = node.value(QStringLiteral("serverId")).toString();
    QString error;
    if (serverId.isEmpty() || !m_store->getServer(serverId, server, &error)) {
        return false;
    }
    return true;
}

QWidget *ServiceProductPanel::buildListPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *addButton = new QPushButton(QStringLiteral("新建实例"));
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"));
    auto *deleteButton = new QPushButton(QStringLiteral("删除"));
    deleteButton->setObjectName(QStringLiteral("dangerButton"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    toolbarLayout->addWidget(addButton);
    toolbarLayout->addWidget(editButton);
    toolbarLayout->addWidget(deleteButton);
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    m_instanceTable = new QTableWidget(0, 4, page);
    m_instanceTable->setHorizontalHeaderLabels({
        QStringLiteral("实例名称"),
        QStringLiteral("节点数"),
        QStringLiteral("状态"),
        QStringLiteral("实例 ID")
    });
    m_instanceTable->verticalHeader()->setVisible(false);
    PageLayout::configureDataTable(m_instanceTable);
    m_instanceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_instanceTable->setAlternatingRowColors(true);
    m_instanceTable->horizontalHeader()->setStretchLastSection(true);
    m_instanceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_instanceTable->verticalHeader()->setDefaultSectionSize(40);
    layout->addWidget(PageLayout::wrapTableSection(
        m_instanceTable,
        &m_listEmptyState,
        QStringLiteral("暂无 %1 实例。点击「新建实例」开始配置。").arg(serviceProductKindLabel(m_product))), 1);

    connect(addButton, &QPushButton::clicked, this, &ServiceProductPanel::addInstance);
    connect(editButton, &QPushButton::clicked, this, &ServiceProductPanel::editInstance);
    connect(deleteButton, &QPushButton::clicked, this, &ServiceProductPanel::deleteInstance);
    connect(refreshButton, &QPushButton::clicked, this, &ServiceProductPanel::reloadInstanceList);
    connect(m_instanceTable, &QTableWidget::doubleClicked, this, &ServiceProductPanel::openSelectedInstance);

    return page;
}

QWidget *ServiceProductPanel::makeInstanceBanner()
{
    auto *banner = new QWidget;
    banner->setObjectName(QStringLiteral("serviceInstanceBanner"));
    auto *layout = new QHBoxLayout(banner);
    layout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    layout->setSpacing(PageLayout::Space12);

    m_bannerTitle = new QLabel(banner);
    m_bannerTitle->setObjectName(QStringLiteral("serviceInstanceTitle"));
    m_bannerStatus = new QLabel(QStringLiteral("检测中"), banner);
    m_bannerStatus->setObjectName(QStringLiteral("serviceInstanceStatusOk"));
    m_bannerNode = new QLabel(banner);
    m_bannerNode->setObjectName(QStringLiteral("serviceInstanceNode"));

    layout->addWidget(m_bannerTitle);
    layout->addWidget(m_bannerStatus);
    layout->addStretch();
    layout->addWidget(m_bannerNode);
    return banner;
}

QWidget *ServiceProductPanel::buildDetailPage()
{
    m_detailPage = new QWidget;
    auto *layout = new QVBoxLayout(m_detailPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *backRow = new QHBoxLayout;
    auto *backButton = new QPushButton(QStringLiteral("← 返回实例列表"));
    backRow->addWidget(backButton);
    backRow->addStretch();
    layout->addLayout(backRow);
    connect(backButton, &QPushButton::clicked, this, &ServiceProductPanel::backToList);

    layout->addWidget(makeInstanceBanner());

    m_detailTabStack = new QStackedWidget(m_detailPage);
    for (const DetailTabSpec &spec : m_detailTabs) {
        m_detailTabStack->addWidget(new QWidget);
    }

    QButtonGroup *tabGroup = nullptr;
    QStringList tabLabels;
    for (const DetailTabSpec &spec : m_detailTabs) {
        tabLabels.append(spec.title);
    }
    layout->addWidget(PageLayout::makeTabBar(tabLabels, m_detailPage, &tabGroup, m_detailTabStack));
    m_detailTabGroup = tabGroup;
    connect(tabGroup, &QButtonGroup::idClicked, this, &ServiceProductPanel::onDetailTabChanged);

    m_detailToolbar = new QWidget(m_detailPage);
    m_detailToolbarLayout = new QHBoxLayout(m_detailToolbar);
    m_detailToolbarLayout->setContentsMargins(0, 0, 0, 0);
    m_detailToolbarLayout->setSpacing(PageLayout::Space8);
    layout->addWidget(m_detailToolbar);

    m_detailTable = new QTableWidget(m_detailPage);
    m_detailTable->verticalHeader()->setVisible(false);
    PageLayout::configureDataTable(m_detailTable);
    m_detailTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_detailTable->setAlternatingRowColors(true);
    m_detailTable->horizontalHeader()->setStretchLastSection(true);
    m_detailTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_detailTable->verticalHeader()->setDefaultSectionSize(40);
    connect(m_detailTable, &QTableWidget::cellDoubleClicked, this, &ServiceProductPanel::onDetailRowActivated);
    layout->addWidget(PageLayout::wrapTableSection(
        m_detailTable,
        &m_detailEmptyState,
        QStringLiteral("连接远端后将在此展示数据。")), 1);

    onDetailTabChanged(0);
    return m_detailPage;
}

void ServiceProductPanel::updateSchemaCombo()
{
    if (m_schemaCombo == nullptr) {
        return;
    }
    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        return;
    }

    ServiceEndpoint endpoint;
    QString error;
    if (!ServiceBroker::resolveContext(instance, server, serviceProductKindKey(m_product), &endpoint, &error)) {
        return;
    }
    const ServiceResult loaded = SqlServiceClient::listSchemas(endpoint, serviceProductKindKey(m_product));
    if (!loaded.ok) {
        return;
    }

    const QString current = m_schemaCombo->currentText();
    m_schemaCombo->clear();
    for (const QJsonObject &row : loaded.rows) {
        const QString schema = row.value(QStringLiteral("owner")).toString();
        if (schema.isEmpty()) {
            m_schemaCombo->addItem(row.value(QStringLiteral("schema_name")).toString());
        } else {
            m_schemaCombo->addItem(schema);
        }
    }
    const int index = m_schemaCombo->findText(current);
    if (index >= 0) {
        m_schemaCombo->setCurrentIndex(index);
    }
}

void ServiceProductPanel::onDetailTabChanged(int index)
{
    m_currentDetailTab = index;
    if (index < 0 || index >= m_detailTabs.size()) {
        return;
    }

    while (QLayoutItem *item = m_detailToolbarLayout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    m_schemaCombo = nullptr;

    const DetailTabSpec &spec = m_detailTabs.at(index);
    for (const QString &action : spec.toolbarActions) {
        auto *button = new QPushButton(action, m_detailToolbar);
        if (action.contains(QStringLiteral("添加")) || action == QStringLiteral("SQL")
            || action.contains(QStringLiteral("主题添加"))) {
            button->setObjectName(QStringLiteral("primaryButton"));
        }
        m_detailToolbarLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, &ServiceProductPanel::onDetailAction);
    }

    if (currentTabKind() == ServiceBroker::TabKind::Table) {
        m_schemaCombo = new QComboBox(m_detailToolbar);
        PageLayout::configureFormInput(m_schemaCombo);
        m_schemaCombo->setMinimumWidth(120);
        m_detailToolbarLayout->addWidget(new QLabel(QStringLiteral("模式")));
        m_detailToolbarLayout->addWidget(m_schemaCombo);
        connect(m_schemaCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
            refreshDetailTable();
        });
        updateSchemaCombo();
    }

    m_detailToolbarLayout->addStretch();

    m_detailTable->setColumnCount(spec.tableHeaders.size());
    m_detailTable->setHorizontalHeaderLabels(spec.tableHeaders);
    refreshDetailTable();
}

void ServiceProductPanel::applyDetailRows(const QVector<QJsonObject> &rows, const QStringList &columns)
{
    m_detailTable->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        const QJsonObject data = rows.at(row);
        for (int col = 0; col < columns.size(); ++col) {
            auto *item = new QTableWidgetItem(data.value(columns.at(col)).toString());
            if (col == 0) {
                item->setData(Qt::UserRole, data);
            }
            m_detailTable->setItem(row, col, item);
        }
        if (columns.size() < m_detailTable->columnCount()) {
            m_detailTable->setItem(row, m_detailTable->columnCount() - 1, new QTableWidgetItem(QStringLiteral("查看")));
        }
    }
    m_detailTable->resizeColumnsToContents();
    PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, rows.size());
}

void ServiceProductPanel::loadRemoteDetailAsync()
{
    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        refreshNodeTable();
        return;
    }

    const ServiceBroker::TabKind tab = currentTabKind();
    if (tab == ServiceBroker::TabKind::Node && m_product != ServiceProductKind::Kafka) {
        refreshNodeTable();
        return;
    }

    const int generation = ++m_loadGeneration;
    const RemoteConnectionContext remote =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache, this, true);
    const QString productKey = serviceProductKindKey(m_product);
    const QString schema = currentSchema();
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext remoteCopy = remote;

    auto *watcher = new QFutureWatcher<ServiceResult>(this);
    connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, &ServiceProductPanel::onRemoteDataLoaded);
    watcher->setProperty("generation", generation);
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey, tab, remoteCopy, schema]() {
        return ServiceBroker::loadTab(instanceCopy, serverCopy, productKey, tab, remoteCopy, schema);
    }));
}

void ServiceProductPanel::onRemoteDataLoaded()
{
    auto *watcher = static_cast<QFutureWatcher<ServiceResult> *>(sender());
    if (watcher == nullptr || watcher->property("generation").toInt() != m_loadGeneration) {
        if (watcher != nullptr) {
            watcher->deleteLater();
        }
        return;
    }

    const ServiceResult result = watcher->result();
    watcher->deleteLater();
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), result.message);
        PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, 0);
        return;
    }

    applyDetailRows(result.rows, rowColumnsForTab(currentTabKind(), m_product));
}

void ServiceProductPanel::refreshDetailTable()
{
    if (m_currentInstanceIndex < 0) {
        return;
    }
    loadRemoteDetailAsync();
}

void ServiceProductPanel::onDetailAction()
{
    const auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }

    const QString action = button->text();
    const int nodeIndex = nodeTabIndex(m_detailTabs);
    if (m_currentDetailTab == nodeIndex) {
        if (action.contains(QStringLiteral("添加"))) {
            addNode();
            return;
        }
        if (action == QStringLiteral("编辑")) {
            editNode();
            return;
        }
        if (action == QStringLiteral("删除")) {
            deleteNode();
            return;
        }
        if (action == QStringLiteral("刷新")) {
            refreshDetailTable();
            return;
        }
    }

    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        QMessageBox::information(this, QStringLiteral("无法操作"), QStringLiteral("请先配置节点并确保服务器可用。"));
        return;
    }

    const RemoteConnectionContext remote =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache, this);
    const QJsonObject selection = selectedRowData();
    QString input;

    if (action.contains(QStringLiteral("主题添加")) && currentTabKind() == ServiceBroker::TabKind::Topic) {
        input = QInputDialog::getText(this,
                                      QStringLiteral("主题添加"),
                                      QStringLiteral("格式: topic,partitions,replication"),
                                      QLineEdit::Normal,
                                      QStringLiteral("demo-topic,1,1"));
    } else if (action.contains(QStringLiteral("数据写入")) && currentTabKind() == ServiceBroker::TabKind::Topic) {
        if (selection.value(QStringLiteral("name")).toString().isEmpty()) {
            QMessageBox::information(this, QStringLiteral("数据写入"), QStringLiteral("请先在列表中选择目标主题。"));
            return;
        }
        bool ok = false;
        input = QInputDialog::getMultiLineText(this,
                                               QStringLiteral("数据写入 - %1").arg(selection.value(QStringLiteral("name")).toString()),
                                               QStringLiteral("消息内容（每行一条）"),
                                               QString(),
                                               &ok);
        if (!ok || input.isEmpty()) {
            return;
        }
    } else if (action.contains(QStringLiteral("Key 搜索"))) {
        input = QInputDialog::getText(this, QStringLiteral("Key 搜索"), QStringLiteral("匹配模式"), QLineEdit::Normal, QStringLiteral("*"));
    } else if (action == QStringLiteral("SQL")) {
        input = QInputDialog::getMultiLineText(this,
                                               QStringLiteral("SQL 查询"),
                                               QStringLiteral("输入 SQL"),
                                               selection.value(QStringLiteral("name")).toString().isEmpty()
                                                   ? QStringLiteral("SELECT 1")
                                                   : QStringLiteral("SELECT * FROM %1.%2 LIMIT 20")
                                                         .arg(currentSchema(), selection.value(QStringLiteral("name")).toString()));
        if (input.isEmpty()) {
            return;
        }
    }

    const ServiceResult result = ServiceBroker::runAction(instance,
                                                          server,
                                                          serviceProductKindKey(m_product),
                                                          currentTabKind(),
                                                          action,
                                                          selection,
                                                          remote,
                                                          currentSchema(),
                                                          input);

    if (action.contains(QStringLiteral("编辑")) && !selection.isEmpty()) {
        const QString metaKey = metadataKeyForSelection(currentTabKind(), selection, currentSchema());
        const QString current = ServiceBroker::metadataDescription(instance, metaKey);
        const QString updated = QInputDialog::getText(this, QStringLiteral("编辑描述"), QStringLiteral("描述"), QLineEdit::Normal, current);
        if (!updated.isEmpty() || !current.isEmpty()) {
            saveCurrentInstance(ServiceBroker::setMetadataDescription(instance, metaKey, updated));
            refreshDetailTable();
        }
        return;
    }

    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("操作失败"), result.message);
        return;
    }

    if (action.contains(QStringLiteral("Kibana"))) {
        QDesktopServices::openUrl(QUrl(result.message));
        return;
    }

    if (action.contains(QStringLiteral("导出"))) {
        const QString path = QFileDialog::getSaveFileName(this, QStringLiteral("导出"), QString(), QStringLiteral("Text (*.txt);;JSON (*.json)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(result.message.toUtf8());
        }
        return;
    }

    if (action == QStringLiteral("SQL") || action.contains(QStringLiteral("查询")) || action.contains(QStringLiteral("最新"))
        || action.contains(QStringLiteral("搜索")) || action.contains(QStringLiteral("刷新"))) {
        if (!result.rows.isEmpty() || action == QStringLiteral("SQL")) {
            ServiceSqlDialog dialog(this);
            dialog.setSql(input);
            dialog.setResult(result);
            dialog.exec();
        } else {
            ServiceContentDialog dialog(action, this);
            dialog.setContent(result.message);
            dialog.exec();
        }
        if (action.contains(QStringLiteral("刷新")) || action.contains(QStringLiteral("搜索"))) {
            refreshDetailTable();
        }
        return;
    }

    refreshDetailTable();
    QMessageBox::information(this, QStringLiteral("完成"), result.message);
}

void ServiceProductPanel::onDetailRowActivated(int row, int)
{
    if (row < 0) {
        return;
    }
    const QTableWidgetItem *item = m_detailTable->item(row, 0);
    if (item == nullptr) {
        return;
    }
    m_detailTable->selectRow(row);

    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        return;
    }
    const QJsonObject selection = item->data(Qt::UserRole).toJsonObject();
    const RemoteConnectionContext remote =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache, this);

    QString action = QStringLiteral("快速查询");
    if (currentTabKind() == ServiceBroker::TabKind::Key) {
        action = QStringLiteral("Key 搜索");
    } else if (currentTabKind() == ServiceBroker::TabKind::Topic) {
        action = QStringLiteral("最新数据");
    } else if (currentTabKind() == ServiceBroker::TabKind::ConsumerGroup) {
        action = QStringLiteral("消费明细");
    } else if (currentTabKind() == ServiceBroker::TabKind::Node) {
        action = QStringLiteral("参数详情");
    }

    const ServiceResult result = ServiceBroker::runAction(instance,
                                                          server,
                                                          serviceProductKindKey(m_product),
                                                          currentTabKind(),
                                                          action,
                                                          selection,
                                                          remote,
                                                          currentSchema(),
                                                          QString());
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("查看失败"), result.message);
        return;
    }
    if (!result.rows.isEmpty()) {
        ServiceSqlDialog dialog(this);
        dialog.setResult(result);
        dialog.exec();
    } else {
        ServiceContentDialog dialog(action, this);
        dialog.setContent(result.message);
        dialog.exec();
    }
}

QJsonObject ServiceProductPanel::selectedRowData() const
{
    const int row = selectedDetailRow();
    if (row < 0) {
        return {};
    }
    const QTableWidgetItem *item = m_detailTable->item(row, 0);
    return item != nullptr ? item->data(Qt::UserRole).toJsonObject() : QJsonObject{};
}

int ServiceProductPanel::selectedDetailRow() const
{
    const auto rows = m_detailTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}

void ServiceProductPanel::saveCurrentInstance(const QJsonObject &instance)
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size()) {
        return;
    }
    m_instances[m_currentInstanceIndex] = instance;
    persistInstances();
}

void ServiceProductPanel::reloadInstanceList()
{
    loadInstancesFromStore();
    m_instanceTable->setRowCount(m_instances.size());
    for (int row = 0; row < m_instances.size(); ++row) {
        const QJsonObject &instance = m_instances.at(row);
        const QJsonArray nodes = instance.value(QStringLiteral("nodes")).toArray();
        m_instanceTable->setItem(row, 0, new QTableWidgetItem(instance.value(QStringLiteral("name")).toString()));
        m_instanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(nodes.size())));
        m_instanceTable->setItem(row, 2, new QTableWidgetItem(nodes.isEmpty() ? QStringLiteral("未配置节点") : QStringLiteral("待检测")));
        m_instanceTable->setItem(row, 3, new QTableWidgetItem(instance.value(QStringLiteral("id")).toString()));
    }
    m_instanceTable->resizeColumnsToContents();
    PageLayout::updateTableEmptyState(m_instanceTable, m_listEmptyState, m_instances.size());
    if (!m_instances.isEmpty() && m_instanceTable->selectionModel()->selectedRows().isEmpty()) {
        m_instanceTable->selectRow(0);
    }
}

int ServiceProductPanel::selectedInstanceIndex() const
{
    const auto rows = m_instanceTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return -1;
    }
    return rows.first().row();
}

void ServiceProductPanel::addInstance()
{
    ServiceInstanceDialog dialog(m_product, this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonObject instance = dialog.instance();
    instance.insert(QStringLiteral("nodes"), QJsonArray{});
    instance.insert(QStringLiteral("metadata"), QJsonObject{});
    for (const QJsonObject &existing : m_instances) {
        if (existing.value(QStringLiteral("id")).toString() == instance.value(QStringLiteral("id")).toString()) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("实例 ID 已存在。"));
            return;
        }
    }
    m_instances.append(instance);
    persistInstances();
    reloadInstanceList();
}

void ServiceProductPanel::editInstance()
{
    const int index = selectedInstanceIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择实例。"));
        return;
    }

    ServiceInstanceDialog dialog(m_product, this);
    dialog.setInstance(m_instances.at(index), true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonObject updated = dialog.instance();
    updated.insert(QStringLiteral("nodes"), m_instances.at(index).value(QStringLiteral("nodes")).toArray());
    updated.insert(QStringLiteral("metadata"), m_instances.at(index).value(QStringLiteral("metadata")).toObject());
    m_instances[index] = updated;
    persistInstances();
    reloadInstanceList();
}

void ServiceProductPanel::deleteInstance()
{
    const int index = selectedInstanceIndex();
    if (index < 0) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择实例。"));
        return;
    }

    const QString name = m_instances.at(index).value(QStringLiteral("name")).toString();
    if (QMessageBox::question(this, QStringLiteral("确认删除"), QStringLiteral("确定删除实例「%1」？").arg(name))
        != QMessageBox::Yes) {
        return;
    }

    m_instances.removeAt(index);
    persistInstances();
    if (m_currentInstanceIndex == index) {
        m_currentInstanceIndex = -1;
        m_stack->setCurrentIndex(0);
    }
    reloadInstanceList();
}

void ServiceProductPanel::openSelectedInstance()
{
    const int index = selectedInstanceIndex();
    if (index < 0) {
        return;
    }
    m_currentInstanceIndex = index;
    refreshBanner();
    refreshDetailTable();
    m_stack->setCurrentIndex(1);
}

void ServiceProductPanel::backToList()
{
    m_currentInstanceIndex = -1;
    m_stack->setCurrentIndex(0);
}

QString ServiceProductPanel::primaryNodeHost() const
{
    if (m_currentInstanceIndex < 0 || m_store == nullptr) {
        return QStringLiteral("-");
    }

    const QJsonArray nodes = nodesForCurrentInstance();
    if (nodes.isEmpty()) {
        return QStringLiteral("未配置节点");
    }

    const QString serverId = nodes.first().toObject().value(QStringLiteral("serverId")).toString();
    QJsonObject server;
    QString error;
    if (m_store->getServer(serverId, &server, &error)) {
        return server.value(QStringLiteral("host")).toString();
    }
    return nodes.first().toObject().value(QStringLiteral("serverLabel")).toString();
}

void ServiceProductPanel::refreshBanner()
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size()) {
        return;
    }

    const QJsonObject &instanceRef = m_instances.at(m_currentInstanceIndex);
    m_bannerTitle->setText(QStringLiteral("%1-%2")
                               .arg(serviceProductKindLabel(m_product), instanceRef.value(QStringLiteral("name")).toString()));
    m_bannerNode->setText(QStringLiteral("节点: %1").arg(primaryNodeHost()));

    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        m_bannerStatus->setText(QStringLiteral("未配置节点"));
        return;
    }

    const RemoteConnectionContext remote =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache, this, true);
    const ServiceResult probe =
        ServiceBroker::testInstance(instance, server, serviceProductKindKey(m_product), remote);
    m_bannerStatus->setText(probe.ok ? QStringLiteral("运行正常") : probe.message);
    m_bannerStatus->setObjectName(probe.ok ? QStringLiteral("serviceInstanceStatusOk")
                                           : QStringLiteral("serviceInstanceStatusBad"));
}

QJsonArray ServiceProductPanel::nodesForCurrentInstance() const
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size()) {
        return {};
    }
    return m_instances.at(m_currentInstanceIndex).value(QStringLiteral("nodes")).toArray();
}

void ServiceProductPanel::saveNodesForCurrentInstance(const QJsonArray &nodes)
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size()) {
        return;
    }
    QJsonObject instance = m_instances.at(m_currentInstanceIndex);
    instance.insert(QStringLiteral("nodes"), nodes);
    m_instances[m_currentInstanceIndex] = instance;
    persistInstances();
    refreshBanner();
    reloadInstanceList();
}

void ServiceProductPanel::refreshNodeTable()
{
    const QJsonArray nodes = nodesForCurrentInstance();
    QVector<QJsonObject> rows;
    for (const QJsonValue &value : nodes) {
        const QJsonObject node = value.toObject();
        if (m_product == ServiceProductKind::Kafka) {
            QString host = node.value(QStringLiteral("serverLabel")).toString();
            const QString serverId = node.value(QStringLiteral("serverId")).toString();
            QJsonObject server;
            QString error;
            if (m_store != nullptr && !serverId.isEmpty() && m_store->getServer(serverId, &server, &error)) {
                host = server.value(QStringLiteral("host")).toString();
            }
            rows.append(QJsonObject{
                {QStringLiteral("ip"), host},
                {QStringLiteral("port"), node.value(QStringLiteral("info")).toString()},
                {QStringLiteral("status"), QStringLiteral("未检测")},
                {QStringLiteral("dataDir"), node.value(QStringLiteral("storagePath")).toString()},
                {QStringLiteral("disk"), QStringLiteral("-")}
            });
        } else {
            rows.append(QJsonObject{
                {QStringLiteral("server"), node.value(QStringLiteral("serverLabel")).toString()},
                {QStringLiteral("info"), node.value(QStringLiteral("info")).toString()},
                {QStringLiteral("installPath"), node.value(QStringLiteral("installPath")).toString()},
                {QStringLiteral("storagePath"), node.value(QStringLiteral("storagePath")).toString()}
            });
        }
    }
    applyDetailRows(rows, rowColumnsForTab(ServiceBroker::TabKind::Node, m_product));
}

int ServiceProductPanel::selectedNodeRow() const
{
    return selectedDetailRow();
}

void ServiceProductPanel::addNode()
{
    if (m_currentInstanceIndex < 0) {
        return;
    }

    const QJsonObject &instance = m_instances.at(m_currentInstanceIndex);
    ServiceNodeDialog dialog(this);
    dialog.setConfigStore(m_store);
    dialog.setCredentials(m_credentials, m_sessionCache);
    dialog.setContext(instance.value(QStringLiteral("name")).toString(), m_product);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonArray nodes = nodesForCurrentInstance();
    nodes.append(dialog.node());
    saveNodesForCurrentInstance(nodes);
    refreshNodeTable();
}

void ServiceProductPanel::editNode()
{
    const int row = selectedNodeRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择节点。"));
        return;
    }

    QJsonArray nodes = nodesForCurrentInstance();
    const QJsonObject existing = nodes.at(row).toObject();
    const QJsonObject &instance = m_instances.at(m_currentInstanceIndex);

    ServiceNodeDialog dialog(this);
    dialog.setConfigStore(m_store);
    dialog.setCredentials(m_credentials, m_sessionCache);
    dialog.setContext(instance.value(QStringLiteral("name")).toString(), m_product);
    dialog.setNode(existing, true);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    nodes.replace(row, dialog.node());
    saveNodesForCurrentInstance(nodes);
    refreshNodeTable();
}

void ServiceProductPanel::deleteNode()
{
    const int row = selectedNodeRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择节点。"));
        return;
    }

    if (QMessageBox::question(this, QStringLiteral("确认删除"), QStringLiteral("确定删除该节点？")) != QMessageBox::Yes) {
        return;
    }

    QJsonArray nodes = nodesForCurrentInstance();
    nodes.removeAt(row);
    saveNodesForCurrentInstance(nodes);
    refreshNodeTable();
}
