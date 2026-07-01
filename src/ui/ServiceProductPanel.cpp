#include "ui/ServiceProductPanel.h"

#include "adapters/services/SqlServiceClient.h"
#include "ui/ServiceNodeTypes.h"
#include "infra/ServiceNodeConnection.h"
#include "infra/RemoteOutputCleaner.h"
#include "infra/CredentialStore.h"
#include "infra/ConfigStore.h"
#include "infra/DataPaths.h"
#include "infra/ServiceInstanceStore.h"
#include "ui/PageLayout.h"
#include "qml/LineTabBarController.h"
#include "ui/RemoteCredentialResolver.h"
#include "ui/ServiceContentDialog.h"
#include "ui/ServiceDatabaseConnectionDialog.h"
#include "ui/ServiceElasticsearchQueryDialog.h"
#include "ui/ServiceElasticsearchIndexStructureDialog.h"
#include "ui/ServiceRedisSearchDialog.h"
#include "ui/ServiceSqlDialog.h"
#include <QJsonArray>
#include <QJsonDocument>
#include "ui/ServiceInstanceDialog.h"
#include "ui/ServiceNodeDialog.h"
#include "ui/ServiceRedisConnectionDialog.h"
#include "ui/ServiceSqlWorkbenchWidget.h"

#include <QComboBox>
#include <QDialog>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QDesktopServices>
#include <QInputDialog>
#include <QLineEdit>
#include <QUrl>
#include <QSet>
#include <QSizePolicy>
#include <QtConcurrent/QtConcurrent>

#include <QAbstractButton>
#include <QApplication>
#include <QColor>
#include <QStyle>
#include <QButtonGroup>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

constexpr int kServiceToolbarButtonWidth = 96;
constexpr int kServiceToolbarButtonHeight = 32;

void configureServiceToolbarButton(QPushButton *button)
{
    if (button == nullptr) {
        return;
    }
    button->setProperty("serviceToolbar", true);
    button->setFixedSize(kServiceToolbarButtonWidth, kServiceToolbarButtonHeight);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setStyle(button->style());
    button->style()->unpolish(button);
    button->style()->polish(button);
}

QVector<ServiceProductPanel::DetailTabSpec> detailTabsFor(ServiceProductKind product)
{
    switch (product) {
    case ServiceProductKind::Kafka:
        return {
            {QStringLiteral("主题管理(Topic)"),
             {QStringLiteral("主题名称"), QStringLiteral("描述"), QStringLiteral("分区数量"),
              QStringLiteral("副本数量"), QStringLiteral("消费者组"), QStringLiteral("近7日"),
              QStringLiteral("昨日"), QStringLiteral("今日"), QStringLiteral("数据总量"), QStringLiteral("操作")},
             {QStringLiteral("主题添加"), QStringLiteral("主题导出"), QStringLiteral("刷新")}},
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
                 {QStringLiteral("Key 搜索"), QStringLiteral("Key 写入"), QStringLiteral("刷新")}}
            };
    case ServiceProductKind::Elasticsearch:
        return {
            {QStringLiteral("索引管理(Index)"),
             {QStringLiteral("索引名称"), QStringLiteral("描述"), QStringLiteral("状态"),
              QStringLiteral("文档总量"), QStringLiteral("磁盘总量"), QStringLiteral("操作")},
             {QStringLiteral("Kibana"), QStringLiteral("索引统计"), QStringLiteral("索引导出"), QStringLiteral("刷新")}},
            {QStringLiteral("节点管理(Node)"),
             {QStringLiteral("服务器"), QStringLiteral("信息"), QStringLiteral("安装路径"),
              QStringLiteral("存储路径"), QStringLiteral("操作")},
             {QStringLiteral("添加节点"), QStringLiteral("编辑"), QStringLiteral("删除"), QStringLiteral("刷新")}}
        };
    case ServiceProductKind::Oracle:
    case ServiceProductKind::PostgreSQL:
        return {
            {QStringLiteral("SQL"), {}, {}}
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
        if (product == ServiceProductKind::Redis) {
            return {QStringLiteral("server"), QStringLiteral("info"), QStringLiteral("installPath"),
                    QStringLiteral("storagePath"), QStringLiteral("status"), QStringLiteral("version"),
                    QStringLiteral("memory")};
        }
        if (product == ServiceProductKind::Elasticsearch) {
            return {QStringLiteral("server"), QStringLiteral("info"), QStringLiteral("installPath"),
                    QStringLiteral("storagePath"), QStringLiteral("status"), QStringLiteral("cluster")};
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
    case ServiceBroker::TabKind::Sql:
        return {};
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

QString statusTagObjectName(const QString &status)
{
    const QString normalized = status.trimmed().toLower();
    if (normalized == QStringLiteral("red")) {
        return QStringLiteral("healthTagRed");
    }
    if (normalized == QStringLiteral("yellow")) {
        return QStringLiteral("healthTagYellow");
    }
    return QStringLiteral("healthTagGreen");
}

QString displayElasticsearchIndexName(const QJsonObject &row, bool expanded)
{
    const QString rowKind = row.value(QStringLiteral("rowKind")).toString();
    const QString name = row.value(QStringLiteral("name")).toString();
    if (rowKind == QStringLiteral("group")) {
        return QStringLiteral("%1 %2").arg(expanded ? QStringLiteral("▼") : QStringLiteral("▶"), name);
    }
    if (rowKind == QStringLiteral("child")) {
        return QStringLiteral("    %1").arg(name);
    }
    return name;
}

bool isElasticsearchChildVisible(const QJsonObject &row, const QSet<QString> &expandedGroups)
{
    if (row.value(QStringLiteral("rowKind")).toString() != QStringLiteral("child")) {
        return true;
    }
    return expandedGroups.contains(row.value(QStringLiteral("groupKey")).toString());
}

bool isDatabaseConnectionProduct(ServiceProductKind product)
{
    return product == ServiceProductKind::Oracle || product == ServiceProductKind::PostgreSQL;
}

bool isRedisConnectionProduct(ServiceProductKind product)
{
    return product == ServiceProductKind::Redis;
}

bool isStandaloneConnectionProduct(ServiceProductKind product)
{
    return isDatabaseConnectionProduct(product) || isRedisConnectionProduct(product);
}

RemoteConnectionContext resolveRemoteCredentials(ServiceProductKind product,
                                                 const QJsonObject &server,
                                                 CredentialStore *credentials,
                                                 CredentialSessionCache *sessionCache,
                                                 QWidget *parent,
                                                 bool allowPrompt = true)
{
    if (ServiceNodeConnection::usesDirectConnect(serviceProductKindKey(product))) {
        return {};
    }
    return RemoteCredentialResolver::resolve(server, credentials, sessionCache, parent, allowPrompt);
}

QString redisConnectionSummary(const QJsonObject &instance)
{
    const QJsonObject node = ServiceNodeConnection::primaryNode(instance);
    if (node.isEmpty()) {
        return QStringLiteral("-");
    }
    const QString host = node.value(QStringLiteral("customHost")).toString().trimmed();
    return host.isEmpty() ? node.value(QStringLiteral("serverLabel")).toString().section(QLatin1Char(':'), 0, 0).trimmed()
                          : host;
}

QString redisDbSummary(const QJsonObject &instance)
{
    const QJsonObject node = ServiceNodeConnection::primaryNode(instance);
    if (node.isEmpty()) {
        return QStringLiteral("-");
    }
    return QStringLiteral("DB%1").arg(node.value(QStringLiteral("redisDb")).toInt(0));
}

QString databaseConnectionSummary(const QJsonObject &instance, ServiceProductKind product)
{
    const QJsonObject node = ServiceNodeConnection::primaryNode(instance);
    if (node.isEmpty()) {
        return QStringLiteral("-");
    }
    const QString host = node.value(QStringLiteral("customHost")).toString().trimmed();
    return host.isEmpty() ? node.value(QStringLiteral("serverLabel")).toString() : host;
}

QString databaseNameSummary(const QJsonObject &instance, ServiceProductKind product)
{
    const QJsonObject node = ServiceNodeConnection::primaryNode(instance);
    if (node.isEmpty()) {
        return QStringLiteral("-");
    }
    ServiceConnectionFields fields;
    if (!ServiceNodeConnection::decodeInfo(node.value(QStringLiteral("info")).toString(),
                                           serviceProductKindKey(product),
                                           &fields)) {
        return QStringLiteral("-");
    }
    return fields.database.isEmpty() ? QStringLiteral("-") : fields.database;
}

constexpr int kEsIndexColName = 0;
constexpr int kEsIndexColDescription = 1;
constexpr int kEsIndexColStatus = 2;
constexpr int kEsIndexColDocs = 3;
constexpr int kEsIndexColDisk = 4;
constexpr int kEsIndexColOperation = 5;

void ensureElasticsearchIndexTableLayout(QTableWidget *table)
{
    if (table == nullptr) {
        return;
    }
    if (table->columnCount() < kEsIndexColOperation + 1) {
        table->setColumnCount(kEsIndexColOperation + 1);
        table->setHorizontalHeaderLabels({QStringLiteral("索引名称"),
                                          QStringLiteral("描述"),
                                          QStringLiteral("状态"),
                                          QStringLiteral("文档总量"),
                                          QStringLiteral("磁盘总量"),
                                          QStringLiteral("操作")});
    }
    PageLayout::configureListingTableWithActionColumn(table, kEsIndexColOperation, kEsIndexColDescription);
    table->horizontalHeader()->setSectionResizeMode(kEsIndexColStatus, QHeaderView::Fixed);
    table->setColumnWidth(kEsIndexColStatus, 96);
    table->setColumnWidth(kEsIndexColOperation, qMax(table->columnWidth(kEsIndexColOperation), 108));
    table->verticalHeader()->setDefaultSectionSize(44);
}

void clearDetailTable(QTableWidget *table)
{
    if (table == nullptr) {
        return;
    }
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int column = 0; column < table->columnCount(); ++column) {
            table->setCellWidget(row, column, nullptr);
        }
    }
    table->clear();
    table->setRowCount(0);
}

QVector<QJsonObject> parseJsonObjectArray(const QString &json)
{
    QVector<QJsonObject> rows;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isArray()) {
        return rows;
    }
    for (const QJsonValue &value : document.array()) {
        if (value.isObject()) {
            rows.append(value.toObject());
        }
    }
    return rows;
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
    if (m_schemaCombo == nullptr) {
        return m_activeSchema;
    }
    if (m_product == ServiceProductKind::Redis && currentTabKind() == ServiceBroker::TabKind::Key) {
        return QString::number(m_schemaCombo->currentIndex());
    }
    return m_schemaCombo->currentText().trimmed();
}

bool ServiceProductPanel::currentInstanceContext(QJsonObject *instance, QJsonObject *server) const
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size() || m_store == nullptr) {
        return false;
    }
    *instance = m_instances.at(m_currentInstanceIndex);
    const QJsonObject node = ServiceNodeConnection::primaryNode(*instance);
    QString error;
    return ServiceNodeConnection::resolveServerForNode(node, m_store, server, &error);
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
    auto *addButton = new QPushButton(isStandaloneConnectionProduct(m_product)
                                          ? QStringLiteral("新建连接")
                                          : QStringLiteral("新建实例"));
    addButton->setObjectName(QStringLiteral("primaryButton"));
    auto *editButton = new QPushButton(isStandaloneConnectionProduct(m_product)
                                           ? QStringLiteral("编辑连接")
                                           : QStringLiteral("编辑"));
    auto *deleteButton = new QPushButton(QStringLiteral("删除"));
    deleteButton->setObjectName(QStringLiteral("dangerButton"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    configureServiceToolbarButton(addButton);
    configureServiceToolbarButton(editButton);
    configureServiceToolbarButton(deleteButton);
    configureServiceToolbarButton(refreshButton);
    toolbarLayout->addWidget(addButton);
    toolbarLayout->addWidget(editButton);
    toolbarLayout->addWidget(deleteButton);
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    m_instanceTable = new QTableWidget(0, 4, page);
    if (isStandaloneConnectionProduct(m_product)) {
        if (isRedisConnectionProduct(m_product)) {
            m_instanceTable->setHorizontalHeaderLabels({
                QStringLiteral("连接名称"),
                QStringLiteral("地址"),
                QStringLiteral("数据库"),
                QStringLiteral("状态")
            });
        } else {
            m_instanceTable->setHorizontalHeaderLabels({
                QStringLiteral("连接名称"),
                QStringLiteral("地址"),
                QStringLiteral("数据库"),
                QStringLiteral("状态")
            });
        }
    } else {
        m_instanceTable->setHorizontalHeaderLabels({
            QStringLiteral("实例名称"),
            QStringLiteral("节点数"),
            QStringLiteral("状态"),
            QStringLiteral("实例 ID")
        });
    }
    m_instanceTable->verticalHeader()->setVisible(false);
    m_instanceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_instanceTable->setAlternatingRowColors(true);
    PageLayout::configureListingTable(m_instanceTable);
    m_instanceTable->verticalHeader()->setDefaultSectionSize(40);
    layout->addWidget(PageLayout::wrapTableSection(
        m_instanceTable,
        &m_listEmptyState,
        isStandaloneConnectionProduct(m_product)
            ? QStringLiteral("暂无 %1 连接。点击「新建连接」填写地址、端口与账号。")
                  .arg(serviceProductKindLabel(m_product))
            : QStringLiteral("暂无 %1 实例。点击「新建实例」开始配置。").arg(serviceProductKindLabel(m_product))), 1);

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

    m_backNavButton = new QPushButton(banner);
    m_backNavButton->setObjectName(QStringLiteral("backNavButton"));
    m_backNavButton->setToolTip(QStringLiteral("返回实例列表"));
    m_backNavButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
    m_backNavButton->setIconSize(QSize(18, 18));
    m_backNavButton->setFixedSize(PageLayout::DialogFieldHeight, PageLayout::DialogFieldHeight);
    m_backNavButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_backNavButton->setCursor(Qt::PointingHandCursor);
    m_backNavButton->setAutoDefault(false);
    m_backNavButton->setDefault(false);

    m_bannerTitle = new QLabel(banner);
    m_bannerTitle->setObjectName(QStringLiteral("serviceInstanceTitle"));
    m_bannerTitle->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    m_bannerStatus = new QLabel(QStringLiteral("检测中"), banner);
    m_bannerStatus->setObjectName(QStringLiteral("serviceInstanceStatusOk"));
    m_bannerStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_bannerStatus->setMinimumWidth(0);

    m_bannerNode = new QLabel(banner);
    m_bannerNode->setObjectName(QStringLiteral("serviceInstanceNode"));
    m_bannerNode->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    layout->addWidget(m_backNavButton, 0, Qt::AlignVCenter);
    layout->addWidget(m_bannerTitle, 0, Qt::AlignVCenter);
    layout->addWidget(m_bannerStatus, 1, Qt::AlignVCenter);
    layout->addWidget(m_bannerNode, 0, Qt::AlignVCenter);
    return banner;
}

QWidget *ServiceProductPanel::buildDetailPage()
{
    m_detailPage = new QWidget;
    auto *layout = new QVBoxLayout(m_detailPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space16);

    layout->addWidget(makeInstanceBanner());
    connect(m_backNavButton, &QPushButton::clicked, this, &ServiceProductPanel::backToList);

    m_detailTabStack = new QStackedWidget(m_detailPage);
    for (const DetailTabSpec &spec : m_detailTabs) {
        m_detailTabStack->addWidget(new QWidget);
    }

    LineTabBarController *tabController = nullptr;
    QStringList tabLabels;
    for (const DetailTabSpec &spec : m_detailTabs) {
        tabLabels.append(spec.title);
    }

    auto *tabRow = new QWidget(m_detailPage);
    auto *tabRowLayout = new QHBoxLayout(tabRow);
    tabRowLayout->setContentsMargins(0, 0, 0, 0);
    tabRowLayout->setSpacing(PageLayout::Space8);
    tabRowLayout->addWidget(PageLayout::makeLineTabBar(tabLabels, m_detailPage, &tabController, m_detailTabStack));
    m_detailTabController = tabController;
    connect(m_detailTabController, &LineTabBarController::tabActivated, this, &ServiceProductPanel::onDetailTabChanged);

    if (isDatabaseConnectionProduct(m_product)) {
        tabRowLayout->addStretch();
        tabRowLayout->addWidget(new QLabel(QStringLiteral("模式"), tabRow));
        m_schemaCombo = new QComboBox(tabRow);
        PageLayout::configureFormInput(m_schemaCombo);
        m_schemaCombo->setMinimumWidth(140);
        tabRowLayout->addWidget(m_schemaCombo);
        m_schemaRefreshButton = new QPushButton(QStringLiteral("刷新"), tabRow);
        m_schemaRefreshButton->setObjectName(QStringLiteral("secondaryButton"));
        m_schemaRefreshButton->setMinimumHeight(32);
        tabRowLayout->addWidget(m_schemaRefreshButton);
        connect(m_schemaCombo, &QComboBox::currentTextChanged, this, [this](const QString &schema) {
            m_activeSchema = schema.trimmed();
            if (m_sqlWorkbench != nullptr) {
                m_sqlWorkbench->setCurrentSchema(m_activeSchema);
            }
            refreshSqlWorkbench();
        });
        connect(m_schemaRefreshButton, &QPushButton::clicked, this, [this]() {
            updateSchemaCombo();
            refreshSqlWorkbench();
        });
    }
    layout->addWidget(tabRow);

    m_detailToolbar = new QWidget(m_detailPage);
    m_detailToolbarLayout = new QHBoxLayout(m_detailToolbar);
    m_detailToolbarLayout->setContentsMargins(0, 0, 0, 0);
    m_detailToolbarLayout->setSpacing(PageLayout::Space8);
    layout->addWidget(m_detailToolbar);

    m_detailContentStack = new QStackedWidget(m_detailPage);
    m_detailTablePage = new QWidget(m_detailContentStack);
    auto *tablePageLayout = new QVBoxLayout(m_detailTablePage);
    tablePageLayout->setContentsMargins(0, 0, 0, 0);
    tablePageLayout->setSpacing(0);

    m_detailTable = new QTableWidget(m_detailTablePage);
    m_detailTable->verticalHeader()->setVisible(false);
    m_detailTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_detailTable->setAlternatingRowColors(true);
    m_detailTable->verticalHeader()->setDefaultSectionSize(40);
    connect(m_detailTable, &QTableWidget::cellDoubleClicked, this, &ServiceProductPanel::onDetailRowActivated);
    connect(m_detailTable, &QTableWidget::cellClicked, this, &ServiceProductPanel::onDetailCellClicked);
    tablePageLayout->addWidget(PageLayout::wrapTableSection(
        m_detailTable,
        &m_detailEmptyState,
        QStringLiteral("连接远端后将在此展示数据。")), 1);
    m_detailContentStack->addWidget(m_detailTablePage);

    if (isDatabaseConnectionProduct(m_product)) {
        m_sqlWorkbench = new ServiceSqlWorkbenchWidget(m_detailContentStack);
        m_detailContentStack->addWidget(m_sqlWorkbench);
        connect(m_sqlWorkbench, &ServiceSqlWorkbenchWidget::executeRequested, this, &ServiceProductPanel::executeSqlQuery);
        connect(m_sqlWorkbench, &ServiceSqlWorkbenchWidget::refreshTablesRequested, this, [this]() {
            refreshSqlWorkbench();
        });
        connect(m_sqlWorkbench, &ServiceSqlWorkbenchWidget::showTableStructureRequested, this,
                &ServiceProductPanel::showTableStructure);
        connect(m_sqlWorkbench, &ServiceSqlWorkbenchWidget::deleteTableRequested, this,
                &ServiceProductPanel::deleteTable);
        connect(m_sqlWorkbench, &ServiceSqlWorkbenchWidget::tableSelected, this, [this](const QString &tableName) {
            if (tableName.isEmpty()) {
                return;
            }
            m_sqlWorkbench->setSqlText(SqlServiceClient::defaultSelectSql(serviceProductKindKey(m_product),
                                                                          currentSchema(),
                                                                          tableName));
        });
    }

    layout->addWidget(m_detailContentStack, 1);

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
        m_schemaCombo->addItem(QStringLiteral("public"));
        m_schemaCombo->setToolTip(loaded.message);
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

    const DetailTabSpec &spec = m_detailTabs.at(index);
    for (const QString &action : spec.toolbarActions) {
        auto *button = new QPushButton(action, m_detailToolbar);
        if (action.contains(QStringLiteral("添加")) || action == QStringLiteral("SQL")
            || action.contains(QStringLiteral("主题添加"))) {
            button->setObjectName(QStringLiteral("primaryButton"));
        }
        configureServiceToolbarButton(button);
        m_detailToolbarLayout->addWidget(button);
        connect(button, &QPushButton::clicked, this, &ServiceProductPanel::onDetailAction);
    }

    if (m_product == ServiceProductKind::Redis && currentTabKind() == ServiceBroker::TabKind::Key) {
        m_schemaCombo = new QComboBox(m_detailToolbar);
        PageLayout::configureFormInput(m_schemaCombo);
        m_schemaCombo->setMinimumWidth(88);
        for (int db = 0; db < 16; ++db) {
            m_schemaCombo->addItem(QStringLiteral("DB%1").arg(db));
        }
        if (m_currentInstanceIndex >= 0 && m_currentInstanceIndex < m_instances.size()) {
            const QJsonObject node = ServiceNodeConnection::primaryNode(m_instances.at(m_currentInstanceIndex));
            m_schemaCombo->setCurrentIndex(qBound(0, node.value(QStringLiteral("redisDb")).toInt(0), 15));
        }
        m_detailToolbarLayout->addWidget(new QLabel(QStringLiteral("数据库")));
        m_detailToolbarLayout->addWidget(m_schemaCombo);
        connect(m_schemaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            m_detailTabCache.remove(m_currentDetailTab);
            refreshDetailTable();
        });
    }

    m_detailToolbarLayout->addStretch();

    const bool sqlTab = currentTabKind() == ServiceBroker::TabKind::Sql;
    m_detailToolbar->setVisible(!sqlTab);
    if (m_detailContentStack != nullptr) {
        if (sqlTab && m_sqlWorkbench != nullptr) {
            m_detailContentStack->setCurrentWidget(m_sqlWorkbench);
            refreshSqlWorkbench();
        } else {
            m_detailContentStack->setCurrentWidget(m_detailTablePage);
        }
    }

    if (sqlTab) {
        if (isDatabaseConnectionProduct(m_product) && m_sqlWorkbench != nullptr) {
            m_sqlWorkbench->setCurrentSchema(currentSchema());
        }
        return;
    }

    clearDetailTable(m_detailTable);
    m_kafkaGroupPartitions.clear();

    m_detailTable->setColumnCount(spec.tableHeaders.size());
    m_detailTable->setHorizontalHeaderLabels(spec.tableHeaders);
    if (spec.tableHeaders.last() == QStringLiteral("操作")) {
        int stretchColumn = 1;
        if (m_product == ServiceProductKind::Kafka && spec.title.contains(QStringLiteral("Consumer"))) {
            stretchColumn = 0;
        } else if (m_product == ServiceProductKind::Elasticsearch && spec.title.contains(QStringLiteral("Index"))) {
            ensureElasticsearchIndexTableLayout(m_detailTable);
            stretchColumn = -1;
        }
        if (stretchColumn >= 0) {
            PageLayout::configureListingTableWithActionColumn(m_detailTable,
                                                              spec.tableHeaders.size() - 1,
                                                              stretchColumn);
        }
    } else {
        PageLayout::configureListingTable(m_detailTable);
    }
    restoreDetailTabFromCache(index);
}

void ServiceProductPanel::restoreDetailTabFromCache(int index)
{
    if (m_detailTabCache.contains(index)) {
        applyDetailRows(m_detailTabCache.value(index), rowColumnsForTab(currentTabKind(), m_product));
        return;
    }
    m_detailTable->setRowCount(0);
    m_detailEmptyState->setText(QStringLiteral("点击「刷新」加载当前页数据。"));
    PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, 0);
}

void ServiceProductPanel::showDetailLoading(const QString &message)
{
    m_detailTable->setRowCount(0);
    m_detailEmptyState->setText(message);
    PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, 0);
}

void ServiceProductPanel::applyDetailRows(const QVector<QJsonObject> &rows, const QStringList &columns)
{
    if (m_product == ServiceProductKind::Elasticsearch && currentTabKind() == ServiceBroker::TabKind::Index) {
        m_esIndexRowsCache = rows;
        applyElasticsearchIndexRows();
        return;
    }

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
            setOperationCell(row, data);
        }
    }
    PageLayout::refreshListingTableColumns(m_detailTable);
    if (m_detailTable->columnCount() > 0
        && m_detailTable->horizontalHeaderItem(m_detailTable->columnCount() - 1) != nullptr
        && m_detailTable->horizontalHeaderItem(m_detailTable->columnCount() - 1)->text()
               == QStringLiteral("操作")) {
        PageLayout::ensureTableActionColumnWidth(m_detailTable, m_detailTable->columnCount() - 1);
    }
    PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, rows.size());
}

void ServiceProductPanel::applyElasticsearchIndexRows()
{
    ensureElasticsearchIndexTableLayout(m_detailTable);

    QVector<QJsonObject> visibleRows;
    visibleRows.reserve(m_esIndexRowsCache.size());
    for (const QJsonObject &row : m_esIndexRowsCache) {
        if (isElasticsearchChildVisible(row, m_expandedEsIndexGroups)) {
            visibleRows.append(row);
        }
    }

    m_detailTable->setRowCount(visibleRows.size());
    for (int row = 0; row < visibleRows.size(); ++row) {
        const QJsonObject data = visibleRows.at(row);
        const QString rowKind = data.value(QStringLiteral("rowKind")).toString();
        const bool expanded = m_expandedEsIndexGroups.contains(data.value(QStringLiteral("groupKey")).toString());

        auto setTextCell = [&](int column, const QString &text, bool primary = false) {
            auto *item = new QTableWidgetItem(text);
            if (primary) {
                item->setData(Qt::UserRole, data);
                if (rowKind == QStringLiteral("group")) {
                    item->setForeground(QColor(QStringLiteral("#6366F1")));
                    QFont font = item->font();
                    font.setBold(true);
                    item->setFont(font);
                } else if (rowKind == QStringLiteral("child")) {
                    item->setForeground(QColor(QStringLiteral("#64748B")));
                }
            }
            m_detailTable->setItem(row, column, item);
        };

        setTextCell(kEsIndexColName,
                    displayElasticsearchIndexName(data, expanded),
                    true);
        setTextCell(kEsIndexColDescription, data.value(QStringLiteral("description")).toString());
        setTextCell(kEsIndexColDocs, data.value(QStringLiteral("docs")).toString());
        setTextCell(kEsIndexColDisk, data.value(QStringLiteral("disk")).toString());

        setElasticsearchStatusCell(row, data.value(QStringLiteral("status")).toString());
        setOperationCell(row, data);
    }

    PageLayout::refreshListingTableColumns(m_detailTable, kEsIndexColDescription);
    PageLayout::ensureTableActionColumnWidth(m_detailTable, kEsIndexColOperation);
    m_detailTable->setColumnWidth(kEsIndexColStatus, 96);
    PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, visibleRows.size());
}

void ServiceProductPanel::toggleElasticsearchIndexGroup(const QString &groupKey)
{
    if (groupKey.isEmpty()) {
        return;
    }
    if (m_expandedEsIndexGroups.contains(groupKey)) {
        m_expandedEsIndexGroups.remove(groupKey);
    } else {
        m_expandedEsIndexGroups.insert(groupKey);
    }
    applyElasticsearchIndexRows();
}

void ServiceProductPanel::setElasticsearchStatusCell(int row, const QString &status)
{
    const QString text = status.isEmpty() ? QStringLiteral("-") : status;
    auto *panel = new QWidget(m_detailTable);
    auto *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(PageLayout::Space4, 0, PageLayout::Space4, 0);
    layout->setSpacing(0);

    auto *tag = new QLabel(text, panel);
    tag->setObjectName(statusTagObjectName(status));
    tag->setAlignment(Qt::AlignCenter);
    tag->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    const QFontMetrics fm(tag->font());
    const int tagWidth = qMax(fm.horizontalAdvance(text) + 22, 56);
    const int tagHeight = fm.height() + 10;
    tag->setFixedSize(tagWidth, tagHeight);
    layout->addWidget(tag, 0, Qt::AlignCenter);
    m_detailTable->setCellWidget(row, kEsIndexColStatus, panel);
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
    if (tab == ServiceBroker::TabKind::Sql) {
        refreshSqlWorkbench();
        return;
    }
    if (tab == ServiceBroker::TabKind::Node && m_product != ServiceProductKind::Kafka
        && m_product != ServiceProductKind::Redis && m_product != ServiceProductKind::Elasticsearch) {
        refreshNodeTable();
        return;
    }

    const int generation = ++m_loadGeneration;
    const int detailTab = m_currentDetailTab;
    showDetailLoading();
    const RemoteConnectionContext remote =
        resolveRemoteCredentials(m_product, server, m_credentials, m_sessionCache, this, true);
    const QString productKey = serviceProductKindKey(m_product);
    const QString schema = currentSchema();
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext remoteCopy = remote;

    auto *watcher = new QFutureWatcher<ServiceResult>(this);
    connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, &ServiceProductPanel::onRemoteDataLoaded);
    watcher->setProperty("generation", generation);
    watcher->setProperty("detailTab", detailTab);
    const bool kafkaTopicQuick =
        productKey == QStringLiteral("kafka") && tab == ServiceBroker::TabKind::Topic;
    watcher->setProperty("kafkaTopicQuick", kafkaTopicQuick);
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey, tab, remoteCopy, schema, kafkaTopicQuick]() {
        if (kafkaTopicQuick) {
            return ServiceBroker::loadKafkaTopicsQuick(instanceCopy, serverCopy, remoteCopy);
        }
        return ServiceBroker::loadTab(instanceCopy, serverCopy, productKey, tab, remoteCopy, schema);
    }));
}

void ServiceProductPanel::startKafkaTopicStatsLoad(int generation, int detailTab, const QVector<QJsonObject> &knownTopics)
{
    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        return;
    }

    const RemoteConnectionContext remote =
        resolveRemoteCredentials(m_product, server, m_credentials, m_sessionCache, this, true);
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext remoteCopy = remote;
    const QVector<QJsonObject> knownTopicsCopy = knownTopics;

    auto *watcher = new QFutureWatcher<ServiceResult>(this);
    connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, &ServiceProductPanel::onRemoteDataLoaded);
    watcher->setProperty("generation", generation);
    watcher->setProperty("detailTab", detailTab);
    watcher->setProperty("kafkaTopicStats", true);
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, remoteCopy, knownTopicsCopy]() {
        return ServiceBroker::loadKafkaTopicStats(instanceCopy, serverCopy, remoteCopy, knownTopicsCopy);
    }));
}

void ServiceProductPanel::mergeKafkaTopicStats(const QVector<QJsonObject> &rows)
{
    QHash<QString, QJsonObject> byName;
    for (const QJsonObject &row : rows) {
        byName.insert(row.value(QStringLiteral("name")).toString(), row);
    }

    const auto setText = [this](int row, int column, const QString &text) {
        if (QTableWidgetItem *item = m_detailTable->item(row, column)) {
            item->setText(text);
        }
    };

    for (int row = 0; row < m_detailTable->rowCount(); ++row) {
        const QTableWidgetItem *nameItem = m_detailTable->item(row, 0);
        if (nameItem == nullptr) {
            continue;
        }
        const QJsonObject stats = byName.value(nameItem->text());
        if (stats.isEmpty()) {
            continue;
        }
        setText(row, 4, stats.value(QStringLiteral("groups")).toString());
        setText(row, 5, stats.value(QStringLiteral("week")).toString());
        setText(row, 6, stats.value(QStringLiteral("yesterday")).toString());
        setText(row, 7, stats.value(QStringLiteral("today")).toString());
        setText(row, 8, stats.value(QStringLiteral("total")).toString());
        if (QTableWidgetItem *item = m_detailTable->item(row, 0)) {
            item->setData(Qt::UserRole, stats);
        }
    }
    m_detailTabCache.insert(m_currentDetailTab, rows);
}

void ServiceProductPanel::onRemoteDataLoaded()
{
    auto *watcher = static_cast<QFutureWatcher<ServiceResult> *>(sender());
    if (watcher == nullptr) {
        return;
    }

    const bool kafkaTopicStats = watcher->property("kafkaTopicStats").toBool();
    if (kafkaTopicStats) {
        if (watcher->property("generation").toInt() == m_loadGeneration
            && watcher->property("detailTab").toInt() == m_currentDetailTab) {
            const ServiceResult result = watcher->result();
            if (result.ok) {
                mergeKafkaTopicStats(result.rows);
            }
        }
        watcher->deleteLater();
        return;
    }

    if (watcher->property("generation").toInt() != m_loadGeneration
        || watcher->property("detailTab").toInt() != m_currentDetailTab) {
        watcher->deleteLater();
        return;
    }

    const ServiceResult result = watcher->result();
    const bool kafkaTopicQuick = watcher->property("kafkaTopicQuick").toBool();
    const int generation = watcher->property("generation").toInt();
    const int detailTab = watcher->property("detailTab").toInt();
    watcher->deleteLater();
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), result.message);
        m_detailEmptyState->setText(QStringLiteral("加载失败，请点击「刷新」重试。"));
        PageLayout::updateTableEmptyState(m_detailTable, m_detailEmptyState, 0);
        return;
    }

    const int loadedTab = m_currentDetailTab;
    applyDetailRows(result.rows, rowColumnsForTab(currentTabKind(), m_product));
    m_detailTabCache.insert(loadedTab, result.rows);
    if (m_product == ServiceProductKind::Kafka && currentTabKind() == ServiceBroker::TabKind::ConsumerGroup) {
        m_kafkaGroupPartitions = parseJsonObjectArray(result.text);
        if (m_kafkaGroupPartitions.isEmpty()) {
            m_kafkaGroupPartitions = result.rows;
        }
    }
    if (kafkaTopicQuick) {
        startKafkaTopicStatsLoad(generation, detailTab, result.rows);
    }
}

void ServiceProductPanel::setOperationCell(int row, const QJsonObject &rowData)
{
    int column = m_detailTable->columnCount() - 1;
    if (m_product == ServiceProductKind::Elasticsearch && currentTabKind() == ServiceBroker::TabKind::Index) {
        column = kEsIndexColOperation;
    }
    if (column < 0) {
        return;
    }

    if (m_product == ServiceProductKind::Kafka && currentTabKind() == ServiceBroker::TabKind::Topic) {
        auto *panel = new QWidget(m_detailTable);
        auto *layout = new QHBoxLayout(panel);
        layout->setContentsMargins(PageLayout::Space4, 2, PageLayout::Space4, 2);
        layout->setSpacing(PageLayout::Space8);

        const auto addAction = [this, panel, layout, column](const QString &label, const QString &objectName) {
            QPushButton *button = PageLayout::makeTableActionButton(label, objectName, panel);
            connect(button, &QPushButton::clicked, this, [this, panel, column, label]() {
                for (int tableRow = 0; tableRow < m_detailTable->rowCount(); ++tableRow) {
                    if (m_detailTable->cellWidget(tableRow, column) == panel) {
                        runDetailRowAction(tableRow, label);
                        return;
                    }
                }
            });
            layout->addWidget(button);
        };

        addAction(QStringLiteral("查看"), QStringLiteral("tableActionView"));
        addAction(QStringLiteral("写入"), QStringLiteral("tableActionWrite"));
        addAction(QStringLiteral("删除"), QStringLiteral("tableActionDelete"));
        layout->addStretch();
        m_detailTable->setCellWidget(row, column, panel);
        Q_UNUSED(rowData);
        return;
    }

    if (m_product == ServiceProductKind::Elasticsearch && currentTabKind() == ServiceBroker::TabKind::Index) {
        auto *panel = new QWidget(m_detailTable);
        auto *layout = new QHBoxLayout(panel);
        layout->setContentsMargins(PageLayout::Space4, 2, PageLayout::Space4, 2);
        layout->setSpacing(PageLayout::Space8);
        QPushButton *queryButton = PageLayout::makeTableActionButton(QStringLiteral("查询"),
                                                                     QStringLiteral("tableActionView"),
                                                                     panel);
        connect(queryButton, &QPushButton::clicked, this, [this, panel, column]() {
            for (int tableRow = 0; tableRow < m_detailTable->rowCount(); ++tableRow) {
                if (m_detailTable->cellWidget(tableRow, column) == panel) {
                    runDetailRowAction(tableRow, QStringLiteral("查询"));
                    return;
                }
            }
        });
        QPushButton *structureButton = PageLayout::makeTableActionButton(QStringLiteral("表结构"),
                                                                         QStringLiteral("tableActionView"),
                                                                         panel);
        connect(structureButton, &QPushButton::clicked, this, [this, panel, column]() {
            for (int tableRow = 0; tableRow < m_detailTable->rowCount(); ++tableRow) {
                if (m_detailTable->cellWidget(tableRow, column) == panel) {
                    runDetailRowAction(tableRow, QStringLiteral("查看表结构"));
                    return;
                }
            }
        });
        layout->addWidget(queryButton);
        layout->addWidget(structureButton);
        layout->addStretch();
        m_detailTable->setCellWidget(row, column, panel);
        Q_UNUSED(rowData);
        return;
    }

    if (m_product == ServiceProductKind::Redis && currentTabKind() == ServiceBroker::TabKind::Key) {
        auto *panel = new QWidget(m_detailTable);
        auto *layout = new QHBoxLayout(panel);
        layout->setContentsMargins(PageLayout::Space4, 2, PageLayout::Space4, 2);
        layout->setSpacing(PageLayout::Space8);
        const auto addAction = [this, panel, layout, column](const QString &label, const QString &objectName) {
            QPushButton *button = PageLayout::makeTableActionButton(label, objectName, panel);
            connect(button, &QPushButton::clicked, this, [this, panel, column, label]() {
                for (int tableRow = 0; tableRow < m_detailTable->rowCount(); ++tableRow) {
                    if (m_detailTable->cellWidget(tableRow, column) == panel) {
                        runDetailRowAction(tableRow, label);
                        return;
                    }
                }
            });
            layout->addWidget(button);
        };
        addAction(QStringLiteral("查看"), QStringLiteral("tableActionView"));
        addAction(QStringLiteral("删除"), QStringLiteral("tableActionDelete"));
        layout->addStretch();
        m_detailTable->setCellWidget(row, column, panel);
        Q_UNUSED(rowData);
        return;
    }

    if (currentTabKind() == ServiceBroker::TabKind::ConsumerGroup
        || currentTabKind() == ServiceBroker::TabKind::Node) {
        auto *panel = new QWidget(m_detailTable);
        auto *layout = new QHBoxLayout(panel);
        layout->setContentsMargins(PageLayout::Space4, 2, PageLayout::Space4, 2);
        layout->setSpacing(PageLayout::Space8);
        QPushButton *button =
            PageLayout::makeTableActionButton(QStringLiteral("查看"), QStringLiteral("tableActionView"), panel);
        connect(button, &QPushButton::clicked, this, [this, panel, column]() {
            for (int tableRow = 0; tableRow < m_detailTable->rowCount(); ++tableRow) {
                if (m_detailTable->cellWidget(tableRow, column) == panel) {
                    runDetailRowAction(tableRow, QStringLiteral("查看"));
                    return;
                }
            }
        });
        layout->addWidget(button);
        layout->addStretch();
        m_detailTable->setCellWidget(row, column, panel);
        Q_UNUSED(rowData);
        return;
    }

    m_detailTable->setItem(row, column, new QTableWidgetItem(QStringLiteral("查看")));
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
    if (action == QStringLiteral("刷新")) {
        refreshDetailTable();
        return;
    }

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
        resolveRemoteCredentials(m_product, server, m_credentials, m_sessionCache, this);
    const QJsonObject selection = selectedRowData();
    QString input;

    if (action.contains(QStringLiteral("主题添加")) && currentTabKind() == ServiceBroker::TabKind::Topic) {
        input = QInputDialog::getText(this,
                                      QStringLiteral("主题添加"),
                                      QStringLiteral("格式: topic,partitions,replication"),
                                      QLineEdit::Normal,
                                      QStringLiteral("demo-topic,1,1"));
    } else if (action.contains(QStringLiteral("Key 搜索"))) {
        ServiceRedisSearchDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        input = dialog.pattern() + QLatin1Char('\t') + dialog.typeFilter();
    } else if (action.contains(QStringLiteral("Key 写入"))) {
        input = QInputDialog::getText(this,
                                      QStringLiteral("Key 写入"),
                                      QStringLiteral("格式: key=value"),
                                      QLineEdit::Normal,
                                      selection.value(QStringLiteral("key")).toString());
        if (input.isEmpty()) {
            return;
        }
    } else if (action == QStringLiteral("SQL")) {
        if (m_detailTabController != nullptr) {
            for (int i = 0; i < m_detailTabs.size(); ++i) {
                if (ServiceBroker::tabKindFromTitle(m_detailTabs.at(i).title) == ServiceBroker::TabKind::Sql) {
                    m_detailTabController->setCurrentIndex(i);
                    onDetailTabChanged(i);
                    return;
                }
            }
        }
        return;
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
        || action.contains(QStringLiteral("搜索"))) {
        if (!result.rows.isEmpty() && action != QStringLiteral("SQL")) {
            ServiceSqlDialog dialog(this);
            dialog.setSql(input);
            dialog.setResult(result);
            dialog.exec();
        } else if (!result.ok) {
            QMessageBox::warning(this, QStringLiteral("操作失败"), result.message);
        }
        if (action.contains(QStringLiteral("刷新")) || action.contains(QStringLiteral("搜索"))) {
            refreshDetailTable();
        }
        return;
    }

    refreshDetailTable();
    QMessageBox::information(this, QStringLiteral("完成"), result.message);
}

void ServiceProductPanel::runDetailRowAction(int row, const QString &action)
{
    if (row < 0 || row >= m_detailTable->rowCount()) {
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
        QMessageBox::information(this, QStringLiteral("无法操作"), QStringLiteral("请先配置节点并确保服务器可用。"));
        return;
    }

    const QJsonObject selection = item->data(Qt::UserRole).toJsonObject();
    const RemoteConnectionContext remote =
        resolveRemoteCredentials(m_product, server, m_credentials, m_sessionCache, this);

    QString brokerAction = action;
    if (action == QStringLiteral("查看")) {
        if (currentTabKind() == ServiceBroker::TabKind::Topic) {
            brokerAction = QStringLiteral("最新数据");
        } else if (currentTabKind() == ServiceBroker::TabKind::ConsumerGroup) {
            brokerAction = QStringLiteral("消费明细");
        } else if (currentTabKind() == ServiceBroker::TabKind::Table) {
            brokerAction = QStringLiteral("样例数据");
        } else if (currentTabKind() == ServiceBroker::TabKind::Key) {
            brokerAction = QStringLiteral("查看");
        } else if (currentTabKind() == ServiceBroker::TabKind::Node) {
            brokerAction = QStringLiteral("参数详情");
        }
    }

    if (action == QStringLiteral("查询")
        && m_product == ServiceProductKind::Elasticsearch
        && currentTabKind() == ServiceBroker::TabKind::Index) {
        ServiceEndpoint endpoint;
        QString error;
        if (!ServiceNodeConnection::resolvePrimaryNode(instance, server, serviceProductKindKey(m_product), &endpoint, &error)) {
            QMessageBox::warning(this, QStringLiteral("无法查询"), error);
            return;
        }
        const QString indexName = selection.value(QStringLiteral("queryIndex")).toString().isEmpty()
            ? selection.value(QStringLiteral("name")).toString()
            : selection.value(QStringLiteral("queryIndex")).toString();
        ServiceElasticsearchQueryDialog dialog(this);
        dialog.setEndpoint(endpoint);
        dialog.setIndexName(indexName);
        dialog.exec();
        return;
    }

    if (action == QStringLiteral("查看表结构")
        && m_product == ServiceProductKind::Elasticsearch
        && currentTabKind() == ServiceBroker::TabKind::Index) {
        const QString rowKind = selection.value(QStringLiteral("rowKind")).toString();
        if (rowKind == QStringLiteral("alias")) {
            QMessageBox::information(this,
                                     QStringLiteral("无法查看表结构"),
                                     QStringLiteral("别名（alias）没有独立的表结构，请展开所属的模板组或直接打开具体索引。"));
            return;
        }
        ServiceEndpoint endpoint;
        QString error;
        if (!ServiceNodeConnection::resolvePrimaryNode(instance, server, serviceProductKindKey(m_product), &endpoint, &error)) {
            QMessageBox::warning(this, QStringLiteral("无法查看"), error);
            return;
        }
        const QString indexName = selection.value(QStringLiteral("queryIndex")).toString().isEmpty()
            ? selection.value(QStringLiteral("name")).toString()
            : selection.value(QStringLiteral("queryIndex")).toString();
        ServiceElasticsearchIndexStructureDialog dialog(this);
        dialog.setEndpoint(endpoint);
        dialog.setIndexName(indexName);
        dialog.exec();
        return;
    }

    if (action == QStringLiteral("查看")
        && m_product == ServiceProductKind::Kafka
        && currentTabKind() == ServiceBroker::TabKind::ConsumerGroup) {
        const QString group = selection.value(QStringLiteral("group")).toString();
        ServiceResult detail{true, {}, {}, {}};
        for (const QJsonObject &row : m_kafkaGroupPartitions) {
            if (row.value(QStringLiteral("group")).toString() == group) {
                detail.rows.append(row);
            }
        }
        if (detail.rows.isEmpty()) {
            QMessageBox::information(this,
                                     QStringLiteral("消费明细"),
                                     QStringLiteral("未找到 %1 的分区消费数据，请先刷新列表。").arg(group));
            return;
        }
        ServiceSqlDialog dialog(this);
        dialog.setTableResult(detail,
                              {QStringLiteral("group"),
                               QStringLiteral("topic"),
                               QStringLiteral("partition"),
                               QStringLiteral("current"),
                               QStringLiteral("logEnd"),
                               QStringLiteral("lag")});
        dialog.exec();
        return;
    }

    QString input;
    if (action == QStringLiteral("写入") && currentTabKind() == ServiceBroker::TabKind::Topic) {
        const QString topic = selection.value(QStringLiteral("name")).toString();
        bool ok = false;
        input = QInputDialog::getMultiLineText(this,
                                               QStringLiteral("数据写入 - %1").arg(topic),
                                               QStringLiteral("消息内容（每行一条）"),
                                               QString(),
                                               &ok);
        if (!ok || input.isEmpty()) {
            return;
        }
    }

    if (action == QStringLiteral("删除")) {
        QString target = selection.value(QStringLiteral("name")).toString();
        if (target.isEmpty()) {
            target = selection.value(QStringLiteral("key")).toString();
        }
        if (target.isEmpty()) {
            return;
        }
        QString confirmMessage = QStringLiteral("确定删除 %1 吗？").arg(target);
        const QString type = selection.value(QStringLiteral("type")).toString();
        if (!type.isEmpty()) {
            confirmMessage = QStringLiteral("确定删除 %1（类型：%2）吗？").arg(target, type);
        }
        const auto answer = QMessageBox::question(this,
                                                  QStringLiteral("确认删除"),
                                                  confirmMessage);
        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    const QString productKey = serviceProductKindKey(m_product);
    const ServiceBroker::TabKind tab = currentTabKind();
    const QString schema = currentSchema();
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext remoteCopy = remote;
    const QJsonObject selectionCopy = selection;

    const auto showViewResult = [this, action, brokerAction, tab](const ServiceResult &result) {
        if (!result.ok) {
            QMessageBox::warning(this, QStringLiteral("操作失败"), result.message);
            return;
        }
        if (action != QStringLiteral("查看")) {
            return;
        }
        if (!result.rows.isEmpty()) {
            ServiceSqlDialog dialog(this);
            if (tab == ServiceBroker::TabKind::ConsumerGroup) {
                dialog.setTableResult(result,
                                        {QStringLiteral("group"),
                                         QStringLiteral("topic"),
                                         QStringLiteral("partition"),
                                         QStringLiteral("current"),
                                         QStringLiteral("logEnd"),
                                         QStringLiteral("lag")});
            } else {
                dialog.setResult(result);
            }
            dialog.exec();
        } else {
            ServiceContentDialog dialog(brokerAction, this);
            dialog.setContent(result.message);
            dialog.exec();
        }
    };

    if (action == QStringLiteral("查看")) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        auto *watcher = new QFutureWatcher<ServiceResult>(this);
        connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, [this, watcher, showViewResult]() {
            QApplication::restoreOverrideCursor();
            showViewResult(watcher->result());
            watcher->deleteLater();
        });
        watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey, tab, brokerAction, selectionCopy, remoteCopy, schema, input]() {
            return ServiceBroker::runAction(instanceCopy,
                                            serverCopy,
                                            productKey,
                                            tab,
                                            brokerAction,
                                            selectionCopy,
                                            remoteCopy,
                                            schema,
                                            input);
        }));
        return;
    }

    const ServiceResult result = ServiceBroker::runAction(instance,
                                                          server,
                                                          productKey,
                                                          tab,
                                                          brokerAction,
                                                          selection,
                                                          remote,
                                                          schema,
                                                          input);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("操作失败"), result.message);
        return;
    }

    if (action == QStringLiteral("查看")) {
        showViewResult(result);
        return;
    }

    if (action == QStringLiteral("写入") || action == QStringLiteral("删除")) {
        refreshDetailTable();
        QMessageBox::information(this, QStringLiteral("完成"), result.message);
        return;
    }
}

void ServiceProductPanel::onDetailRowActivated(int row, int column)
{
    if (row < 0) {
        return;
    }
    if (m_product == ServiceProductKind::Kafka && currentTabKind() == ServiceBroker::TabKind::Topic
        && column == m_detailTable->columnCount() - 1) {
        return;
    }
    if (m_product == ServiceProductKind::Elasticsearch && currentTabKind() == ServiceBroker::TabKind::Index) {
        if (column == m_detailTable->columnCount() - 1) {
            return;
        }
        runDetailRowAction(row, QStringLiteral("查询"));
        return;
    }
    runDetailRowAction(row, QStringLiteral("查看"));
}

void ServiceProductPanel::onDetailCellClicked(int row, int column)
{
    if (row < 0 || column != 0) {
        return;
    }
    if (m_product != ServiceProductKind::Elasticsearch || currentTabKind() != ServiceBroker::TabKind::Index) {
        return;
    }
    const QTableWidgetItem *item = m_detailTable->item(row, 0);
    if (item == nullptr) {
        return;
    }
    const QJsonObject rowData = item->data(Qt::UserRole).toJsonObject();
    if (rowData.value(QStringLiteral("rowKind")).toString() != QStringLiteral("group")) {
        return;
    }
    toggleElasticsearchIndexGroup(rowData.value(QStringLiteral("groupKey")).toString());
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
        if (isStandaloneConnectionProduct(m_product)) {
            m_instanceTable->setItem(row, 0, new QTableWidgetItem(instance.value(QStringLiteral("name")).toString()));
            if (isRedisConnectionProduct(m_product)) {
                m_instanceTable->setItem(row, 1, new QTableWidgetItem(redisConnectionSummary(instance)));
                m_instanceTable->setItem(row, 2, new QTableWidgetItem(redisDbSummary(instance)));
            } else {
                m_instanceTable->setItem(row, 1,
                                         new QTableWidgetItem(databaseConnectionSummary(instance, m_product)));
                m_instanceTable->setItem(row, 2, new QTableWidgetItem(databaseNameSummary(instance, m_product)));
            }
            m_instanceTable->setItem(row,
                                     3,
                                     new QTableWidgetItem(nodes.isEmpty() ? QStringLiteral("未配置") : QStringLiteral("已配置")));
        } else {
            m_instanceTable->setItem(row, 0, new QTableWidgetItem(instance.value(QStringLiteral("name")).toString()));
            m_instanceTable->setItem(row, 1, new QTableWidgetItem(QString::number(nodes.size())));
            m_instanceTable->setItem(row, 2, new QTableWidgetItem(nodes.isEmpty() ? QStringLiteral("未配置节点") : QStringLiteral("待检测")));
            m_instanceTable->setItem(row, 3, new QTableWidgetItem(instance.value(QStringLiteral("id")).toString()));
        }
    }
    PageLayout::refreshListingTableColumns(m_instanceTable);
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
    if (isRedisConnectionProduct(m_product)) {
        ServiceRedisConnectionDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        QJsonObject instance = dialog.instance();
        instance.insert(QStringLiteral("metadata"), QJsonObject{});
        for (const QJsonObject &existing : m_instances) {
            if (existing.value(QStringLiteral("id")).toString() == instance.value(QStringLiteral("id")).toString()) {
                QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("连接 ID 已存在，请修改连接名称。"));
                return;
            }
        }
        m_instances.append(instance);
        persistInstances();
        reloadInstanceList();
        return;
    }

    if (isDatabaseConnectionProduct(m_product)) {
        ServiceDatabaseConnectionDialog dialog(m_product, this);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        QJsonObject instance = dialog.instance();
        instance.insert(QStringLiteral("metadata"), QJsonObject{});
        for (const QJsonObject &existing : m_instances) {
            if (existing.value(QStringLiteral("id")).toString() == instance.value(QStringLiteral("id")).toString()) {
                QMessageBox::warning(this, QStringLiteral("保存失败"), QStringLiteral("连接 ID 已存在，请修改连接名称。"));
                return;
            }
        }
        m_instances.append(instance);
        persistInstances();
        reloadInstanceList();
        return;
    }

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
        QMessageBox::information(this,
                                 QStringLiteral("未选择"),
                                 isStandaloneConnectionProduct(m_product) ? QStringLiteral("请先选择连接。")
                                                                      : QStringLiteral("请先选择实例。"));
        return;
    }

    if (isRedisConnectionProduct(m_product)) {
        ServiceRedisConnectionDialog dialog(this);
        dialog.setInstance(m_instances.at(index), true);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        QJsonObject updated = dialog.instance();
        updated.insert(QStringLiteral("metadata"), m_instances.at(index).value(QStringLiteral("metadata")).toObject());
        m_instances[index] = updated;
        persistInstances();
        reloadInstanceList();
        return;
    }

    if (isDatabaseConnectionProduct(m_product)) {
        ServiceDatabaseConnectionDialog dialog(m_product, this);
        dialog.setInstance(m_instances.at(index), true);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        QJsonObject updated = dialog.instance();
        updated.insert(QStringLiteral("metadata"), m_instances.at(index).value(QStringLiteral("metadata")).toObject());
        m_instances[index] = updated;
        persistInstances();
        reloadInstanceList();
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

    int targetTab = m_currentDetailTab;
    if (isStandaloneConnectionProduct(m_product)) {
        targetTab = 0;
    } else if (nodesForCurrentInstance().isEmpty()) {
        const int nodeIndex = nodeTabIndex(m_detailTabs);
        if (nodeIndex >= 0) {
            targetTab = nodeIndex;
        }
    }
    if (targetTab < 0 || targetTab >= m_detailTabs.size()) {
        targetTab = 0;
    }
    if (m_detailTabController != nullptr) {
        m_detailTabController->setCurrentIndex(targetTab);
    }
    if (m_detailTabStack != nullptr) {
        m_detailTabStack->setCurrentIndex(targetTab);
    }

    m_stack->setCurrentIndex(1);
    m_detailTabCache.clear();
    onDetailTabChanged(targetTab);
    refreshBanner();
    if (isDatabaseConnectionProduct(m_product)) {
        updateSchemaCombo();
        if (m_sqlWorkbench != nullptr) {
            m_sqlWorkbench->setCurrentSchema(currentSchema());
        }
        refreshSqlWorkbench();
        return;
    }
    showDetailLoading();
    refreshDetailTable();
}

void ServiceProductPanel::backToList()
{
    ++m_bannerGeneration;
    ++m_loadGeneration;
    m_currentInstanceIndex = -1;
    m_detailTabCache.clear();
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

    return ServiceNodeConnection::hostFromNodeLabel(nodes.first().toObject());
}

void ServiceProductPanel::refreshBanner()
{
    if (m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size()) {
        return;
    }

    const QJsonObject &instanceRef = m_instances.at(m_currentInstanceIndex);
    m_bannerTitle->setText(isStandaloneConnectionProduct(m_product)
                               ? instanceRef.value(QStringLiteral("name")).toString()
                               : QStringLiteral("%1-%2")
                                     .arg(serviceProductKindLabel(m_product),
                                          instanceRef.value(QStringLiteral("name")).toString()));

    if (isRedisConnectionProduct(m_product)) {
        const QJsonObject node = ServiceNodeConnection::primaryNode(instanceRef);
        ServiceConnectionFields fields;
        ServiceNodeConnection::decodeInfo(node.value(QStringLiteral("info")).toString(),
                                          serviceProductKindKey(m_product),
                                          &fields);
        const QString host = redisConnectionSummary(instanceRef);
        m_bannerNode->setText(QStringLiteral("%1:%2 / DB%3")
                                  .arg(host,
                                       QString::number(fields.port),
                                       QString::number(node.value(QStringLiteral("redisDb")).toInt(0))));
    } else if (isDatabaseConnectionProduct(m_product)) {
        const QJsonObject node = ServiceNodeConnection::primaryNode(instanceRef);
        ServiceConnectionFields fields;
        ServiceNodeConnection::decodeInfo(node.value(QStringLiteral("info")).toString(),
                                          serviceProductKindKey(m_product),
                                          &fields);
        const QString host = databaseConnectionSummary(instanceRef, m_product);
        m_bannerNode->setText(QStringLiteral("%1:%2 / %3").arg(host, QString::number(fields.port), fields.database));
    } else {
        m_bannerNode->setText(QStringLiteral("节点: %1").arg(primaryNodeHost()));
    }

    const auto setStatus = [this](const QString &text, bool ok) {
        m_bannerStatus->setToolTip(text);
        const QString display = text.size() > 56
                                    ? QStringLiteral("%1…").arg(text.left(55))
                                    : text;
        m_bannerStatus->setText(display);
        m_bannerStatus->setObjectName(ok ? QStringLiteral("serviceInstanceStatusOk")
                                         : QStringLiteral("serviceInstanceStatusBad"));
        m_bannerStatus->style()->unpolish(m_bannerStatus);
        m_bannerStatus->style()->polish(m_bannerStatus);
    };

    // Invalidate any in-flight probe from a previous instance/open.
    ++m_bannerGeneration;

    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        setStatus(QStringLiteral("未配置节点"), false);
        return;
    }

    setStatus(QStringLiteral("检测中…"), true);

    // Probe the instance in a background thread so entering the detail view never
    // freezes the UI (the back button must stay responsive). Avoid a modal
    // password prompt here; the probe degrades gracefully when no credential is
    // cached, and data loading will prompt when actually needed.
    const int generation = m_bannerGeneration;
    const RemoteConnectionContext remote =
        resolveRemoteCredentials(m_product, server, m_credentials, m_sessionCache, this, false);
    const QString productKey = serviceProductKindKey(m_product);
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext remoteCopy = remote;

    auto *watcher = new QFutureWatcher<ServiceResult>(this);
    watcher->setProperty("bannerGeneration", generation);
    connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, [this, watcher, setStatus]() {
        const int gen = watcher->property("bannerGeneration").toInt();
        const ServiceResult probe = watcher->result();
        watcher->deleteLater();
        if (gen != m_bannerGeneration) {
            return;
        }
        const QString statusText = probe.ok
                                       ? QStringLiteral("运行正常")
                                       : RemoteOutputCleaner::normalizeRemoteError(probe.message);
        setStatus(statusText.isEmpty() ? QStringLiteral("连接失败") : statusText, probe.ok);
    });
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey, remoteCopy]() {
        return ServiceBroker::testInstance(instanceCopy, serverCopy, productKey, remoteCopy);
    }));
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
            const QString host = ServiceNodeConnection::hostFromNodeLabel(node);
            rows.append(QJsonObject{
                {QStringLiteral("ip"), host},
                {QStringLiteral("port"), node.value(QStringLiteral("info")).toString()},
                {QStringLiteral("status"), QStringLiteral("未检测")},
                {QStringLiteral("dataDir"), node.value(QStringLiteral("storagePath")).toString()},
                {QStringLiteral("disk"), QStringLiteral("-")}
            });
        } else {
            const QString connectionMode = node.value(QStringLiteral("connectionMode")).toString();
            const bool direct = connectionMode == QStringLiteral("direct")
                || serviceProductUsesDirectConnect(m_product);
            QString infoLabel = node.value(QStringLiteral("info")).toString();
            if (direct) {
                ServiceConnectionFields fields;
                if (ServiceNodeConnection::decodeInfo(infoLabel, serviceProductKindKey(m_product), &fields)) {
                    infoLabel = fields.username.isEmpty()
                                    ? QStringLiteral("端口 %1").arg(fields.port)
                                    : QStringLiteral("%1 @ 端口 %2").arg(fields.username).arg(fields.port);
                }
            }
            rows.append(QJsonObject{
                {QStringLiteral("server"), node.value(QStringLiteral("serverLabel")).toString()},
                {QStringLiteral("info"), infoLabel},
                {QStringLiteral("installPath"),
                 direct ? QStringLiteral("-") : node.value(QStringLiteral("installPath")).toString()},
                {QStringLiteral("storagePath"),
                 direct ? QStringLiteral("-") : node.value(QStringLiteral("storagePath")).toString()}
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

void ServiceProductPanel::updateSqlConnectionSummary()
{
    if (m_sqlWorkbench == nullptr || m_currentInstanceIndex < 0 || m_currentInstanceIndex >= m_instances.size()) {
        return;
    }

    const QJsonObject &instance = m_instances.at(m_currentInstanceIndex);
    const QJsonObject node = ServiceNodeConnection::primaryNode(instance);
    ServiceConnectionFields fields;
    ServiceNodeConnection::decodeInfo(node.value(QStringLiteral("info")).toString(),
                                      serviceProductKindKey(m_product),
                                      &fields);
    const QString host = databaseConnectionSummary(instance, m_product);
    m_sqlWorkbench->setConnectionSummary(QStringLiteral("%1:%2 / %3")
                                             .arg(host, QString::number(fields.port), fields.database));
}

void ServiceProductPanel::refreshSqlWorkbench()
{
    if (m_sqlWorkbench == nullptr) {
        return;
    }

    updateSqlConnectionSummary();

    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        m_sqlWorkbench->setTables({});
        return;
    }

    const QString productKey = serviceProductKindKey(m_product);
    const QString schema = currentSchema();
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;

    auto *watcher = new QFutureWatcher<ServiceResult>(this);
    connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, [this, watcher]() {
        const ServiceResult result = watcher->result();
        watcher->deleteLater();
        if (!result.ok || m_sqlWorkbench == nullptr) {
            return;
        }
        QStringList tables;
        for (const QJsonObject &row : result.rows) {
            tables.append(row.value(QStringLiteral("name")).toString());
        }
        m_sqlWorkbench->setTables(tables);
    });
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey, schema]() {
        return ServiceBroker::loadTab(instanceCopy, serverCopy, productKey, ServiceBroker::TabKind::Table, {}, schema);
    }));
}

void ServiceProductPanel::executeSqlQuery(const QString &sql)
{
    if (m_sqlWorkbench == nullptr || sql.trimmed().isEmpty()) {
        return;
    }

    QJsonObject instance;
    QJsonObject server;
    if (!currentInstanceContext(&instance, &server)) {
        QMessageBox::information(this, QStringLiteral("无法执行"), QStringLiteral("请先配置数据库连接。"));
        return;
    }

    m_sqlWorkbench->setExecuting(true);
    const QString productKey = serviceProductKindKey(m_product);
    const QString schema = currentSchema();
    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const QString sqlCopy = SqlServiceClient::withSchemaContext(productKey, schema, sql);

    auto *watcher = new QFutureWatcher<QPair<ServiceResult, qint64>>(this);
    connect(watcher, &QFutureWatcher<QPair<ServiceResult, qint64>>::finished, this, [this, watcher]() {
        if (m_sqlWorkbench != nullptr) {
            m_sqlWorkbench->setExecuting(false);
            const QPair<ServiceResult, qint64> payload = watcher->result();
            m_sqlWorkbench->showResult(payload.first, payload.second);
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey, sqlCopy]() {
        QElapsedTimer timer;
        timer.start();
        ServiceResult result = ServiceBroker::runAction(instanceCopy,
                                                        serverCopy,
                                                        productKey,
                                                        ServiceBroker::TabKind::Sql,
                                                        QStringLiteral("SQL"),
                                                        {},
                                                        {},
                                                        {},
                                                        sqlCopy);
        return qMakePair(result, timer.elapsed());
    }));
}

void ServiceProductPanel::showTableStructure(const QString &tableName)
{
    if (tableName.isEmpty()) {
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
        QMessageBox::warning(this, QStringLiteral("无法查看表结构"), error);
        return;
    }

    const ServiceResult result =
        SqlServiceClient::describeTable(endpoint, serviceProductKindKey(m_product), currentSchema(), tableName);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("无法查看表结构"), result.message);
        return;
    }

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("表结构 - %1").arg(tableName));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    auto *layout = new QVBoxLayout(dialog);
    PageLayout::applyDialog(layout);

    auto *table = new QTableWidget(dialog);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({QStringLiteral("序号"),
                                      QStringLiteral("字段"),
                                      QStringLiteral("类型"),
                                      QStringLiteral("可空"),
                                      QStringLiteral("默认值")});
    table->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); ++row) {
        const QJsonObject item = result.rows.at(row);
        table->setItem(row, 0, new QTableWidgetItem(item.value(QStringLiteral("ordinal")).toString()));
        table->setItem(row, 1, new QTableWidgetItem(item.value(QStringLiteral("column_name")).toString()));
        table->setItem(row, 2, new QTableWidgetItem(item.value(QStringLiteral("data_type")).toString()));
        table->setItem(row, 3, new QTableWidgetItem(item.value(QStringLiteral("nullable")).toString()));
        table->setItem(row, 4, new QTableWidgetItem(item.value(QStringLiteral("data_default")).toString()));
    }
    PageLayout::configureListingTable(table);
    PageLayout::refreshListingTableColumns(table);
    layout->addWidget(table, 1);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"), dialog);
    connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeButton, 0, Qt::AlignRight);

    PageLayout::applyModalDialog(dialog);
    dialog->resize(760, 480);
    dialog->open();
}

void ServiceProductPanel::deleteTable(const QString &tableName)
{
    if (tableName.isEmpty()) {
        return;
    }
    if (QMessageBox::question(this,
                              QStringLiteral("确认删除"),
                              QStringLiteral("确定删除表 %1？此操作不可恢复。").arg(tableName))
        != QMessageBox::Yes) {
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
        QMessageBox::warning(this, QStringLiteral("删除失败"), error);
        return;
    }

    const ServiceResult result =
        SqlServiceClient::dropTable(endpoint, serviceProductKindKey(m_product), currentSchema(), tableName);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), result.message);
        return;
    }

    QMessageBox::information(this, QStringLiteral("删除成功"), result.message.isEmpty() ? QStringLiteral("表已删除") : result.message);
    refreshSqlWorkbench();
}
