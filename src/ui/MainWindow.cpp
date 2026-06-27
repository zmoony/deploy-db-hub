#include "ui/MainWindow.h"

#include "infra/ConfigStore.h"
#include "infra/DataPaths.h"
#include "infra/AiSettingsStore.h"
#include "infra/AppSettingsStore.h"
#include "adapters/services/JdbcSqlBridge.h"
#include "infra/JdkProfileStore.h"
#include "infra/LocalLogCatalog.h"
#include "infra/RemoteLogPath.h"
#include "adapters/remote/RemoteExecutor.h"
#include "adapters/remote/RemoteFileBrowser.h"
#include "ui/RemoteCredentialResolver.h"
#include "adapters/remote/RemoteMonitor.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "ui/DeployLogPathOptions.h"
#include "ui/DeployWorker.h"
#include "ui/DeploymentLogDialog.h"
#include "ui/DeploymentLogOpener.h"
#include "ui/BigDataManagerWidget.h"
#include "ui/CommonToolsWidget.h"
#include "ui/DatabaseManagerWidget.h"
#include "ui/JdkProfileDialog.h"
#include "ui/PageLayout.h"
#include "ui/ProjectManagerWidget.h"
#include "ui/RemoteFileViewerDialog.h"
#include "ui/RemoteUiHelpers.h"
#include "ui/ServerManagerWidget.h"
#include "infra/AppBranding.h"
#include <QFutureWatcher>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QtConcurrent/QtConcurrent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>

#include <functional>

namespace {

QIcon makeSidebarLineIcon(const QString &label)
{
    const bool active = false;
    Q_UNUSED(active);
    const QColor color(QStringLiteral("#64748B"));
    QPixmap pixmap(18, 18);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    const QRectF r(3.0, 3.0, 12.0, 12.0);
    if (label.contains(QStringLiteral("AI"))) {
        painter.drawRoundedRect(r.adjusted(1, 1, -1, -1), 3, 3);
        painter.drawLine(QPointF(9, 6), QPointF(9, 12));
        painter.drawLine(QPointF(6, 9), QPointF(12, 9));
    } else if (label.contains(QStringLiteral("JSON")) || label.contains(QStringLiteral("文本")) || label.contains(QStringLiteral("HTML"))) {
        painter.drawLine(QPointF(7, 5), QPointF(4, 9));
        painter.drawLine(QPointF(4, 9), QPointF(7, 13));
        painter.drawLine(QPointF(11, 5), QPointF(14, 9));
        painter.drawLine(QPointF(14, 9), QPointF(11, 13));
    } else if (label.contains(QStringLiteral("图片"))) {
        painter.drawRoundedRect(r, 2, 2);
        painter.drawEllipse(QPointF(7, 7), 1.2, 1.2);
        painter.drawLine(QPointF(5, 13), QPointF(8, 10));
        painter.drawLine(QPointF(8, 10), QPointF(11, 13));
        painter.drawLine(QPointF(11, 13), QPointF(14, 9));
    } else if (label.contains(QStringLiteral("HTTP")) || label.contains(QStringLiteral("WebSocket"))) {
        painter.drawEllipse(r);
        painter.drawLine(QPointF(4, 9), QPointF(14, 9));
        painter.drawArc(QRectF(6, 3, 6, 12), 90 * 16, 180 * 16);
        painter.drawArc(QRectF(6, 3, 6, 12), -90 * 16, 180 * 16);
    } else if (label.contains(QStringLiteral("部署")) || label.contains(QStringLiteral("项目")) || label.contains(QStringLiteral("服务器"))) {
        painter.drawRoundedRect(QRectF(4, 5, 10, 8), 2, 2);
        painter.drawLine(QPointF(7, 5), QPointF(7, 4));
        painter.drawLine(QPointF(11, 5), QPointF(11, 4));
        painter.drawLine(QPointF(6, 11), QPointF(12, 11));
    } else if (label.contains(QStringLiteral("仪表盘")) || label.contains(QStringLiteral("历史")) || label.contains(QStringLiteral("日志"))) {
        painter.drawRoundedRect(r, 2, 2);
        painter.drawLine(QPointF(6, 12), QPointF(6, 9));
        painter.drawLine(QPointF(9, 12), QPointF(9, 6));
        painter.drawLine(QPointF(12, 12), QPointF(12, 8));
    } else if (label.contains(QStringLiteral("设置"))) {
        painter.drawEllipse(QPointF(9, 9), 4.5, 4.5);
        painter.drawEllipse(QPointF(9, 9), 1.5, 1.5);
        painter.drawLine(QPointF(9, 2.5), QPointF(9, 4.5));
        painter.drawLine(QPointF(9, 13.5), QPointF(9, 15.5));
        painter.drawLine(QPointF(2.5, 9), QPointF(4.5, 9));
        painter.drawLine(QPointF(13.5, 9), QPointF(15.5, 9));
    } else {
        painter.drawRoundedRect(r, 2, 2);
        painter.drawLine(QPointF(6, 7), QPointF(12, 7));
        painter.drawLine(QPointF(6, 10), QPointF(12, 10));
        painter.drawLine(QPointF(6, 13), QPointF(10, 13));
    }

    painter.end();
    return QIcon(pixmap);
}

QString formatByteSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    }
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

}

#include <utility>

namespace {

QString formatStartedAt(const QJsonObject &record)
{
    const QDateTime started = QDateTime::fromString(record.value(QStringLiteral("startedAt")).toString(), Qt::ISODate);
    if (!started.isValid()) {
        return record.value(QStringLiteral("startedAt")).toString();
    }
    return started.toString(QStringLiteral("yyyy-MM-dd HH:mm"));
}

QString formatDeploymentStatusLabel(const QString &status)
{
    if (status == QStringLiteral("success")) {
        return QStringLiteral("成功");
    }
    if (status == QStringLiteral("failed")) {
        return QStringLiteral("失败");
    }
    if (status == QStringLiteral("rolled_back")) {
        return QStringLiteral("已回滚");
    }
    if (status == QStringLiteral("rollback_failed")) {
        return QStringLiteral("回滚失败");
    }
    if (status == QStringLiteral("canceled")) {
        return QStringLiteral("已取消");
    }
    return status.isEmpty() ? QStringLiteral("-") : status;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_store(std::make_unique<ConfigStore>(DataPaths::databaseFile()))
    , m_credentials(std::make_unique<CredentialStore>(DataPaths::credentialsFile()))
    , m_aiSettings(std::make_unique<AiSettingsStore>(AiSettingsStore::defaultSettingsFile()))
    , m_sessionCache(std::make_unique<CredentialSessionCache>())
{
    setWindowTitle(QStringLiteral("Deploy Hub - 可视化部署工具"));
    AppBranding::applyWindowIcon(this);
    PageLayout::enableResizableWindow(this, false);

    QString error;
    if (!m_store->open(&error)) {
        QMessageBox::critical(this, QStringLiteral("数据库错误"), error);
    }

    auto *root = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, PageLayout::Space24, 0);
    rootLayout->setSpacing(PageLayout::Space24);

    m_navigation = PageLayout::createSidebarNavigationList();
    rootLayout->addWidget(PageLayout::wrapSidebarNavigation(m_navigation, &m_settingsButton));

    auto *content = new QWidget(root);
    content->setObjectName(QStringLiteral("mainWorkspace"));
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, PageLayout::Space8, 0, PageLayout::Space8);
    contentLayout->setSpacing(0);

    m_moduleStack = new QStackedWidget;
    m_moduleStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_commonTools = new CommonToolsWidget(m_aiSettings.get(), m_credentials.get());
    m_projectManager = new ProjectManagerWidget(m_store.get());
    m_serverManager = new ServerManagerWidget(m_store.get());
    m_bigDataManager = new BigDataManagerWidget(m_store.get(), m_credentials.get(), m_sessionCache.get());
    m_databaseManager = new DatabaseManagerWidget(m_store.get(), m_credentials.get(), m_sessionCache.get());

    QList<QWidget *> commonToolPages;
    const int commonToolCount = m_commonTools->toolPageCount();
    for (int i = 0; i < commonToolCount; ++i) {
        commonToolPages.append(m_commonTools->takeToolPage(0));
    }
    addModuleFromPages(QStringLiteral("通用工具"), m_commonTools->toolLabels(), commonToolPages);
    addModule(QStringLiteral("部署工具"), {
        {QStringLiteral("仪表盘"), createDashboardPage()},
        {QStringLiteral("项目管理"), m_projectManager},
        {QStringLiteral("服务器管理"), m_serverManager},
        {QStringLiteral("一键部署"), createDeployPage()},
        {QStringLiteral("历史记录"), createHistoryPage()},
        {QStringLiteral("设置"), createDeploySettingsPage()}
    });
    QList<QWidget *> bigDataPages;
    const int bigDataPageCount = m_bigDataManager->sectionPageCount();
    for (int i = 0; i < bigDataPageCount; ++i) {
        bigDataPages.append(m_bigDataManager->takeSectionPage(0));
    }
    addModuleFromPages(QStringLiteral("大数据"), m_bigDataManager->sectionLabels(), bigDataPages);
    QList<QWidget *> databasePages;
    const int databasePageCount = m_databaseManager->sectionPageCount();
    for (int i = 0; i < databasePageCount; ++i) {
        databasePages.append(m_databaseManager->takeSectionPage(0));
    }
    addModuleFromPages(QStringLiteral("数据库"), m_databaseManager->sectionLabels(), databasePages);

    m_contentStack = new QStackedWidget(content);
    m_contentStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_contentStack->addWidget(m_moduleStack);
    m_settingsPage = PageLayout::wrapScrollableContentPanel(createGlobalSettingsPage());
    m_contentStack->addWidget(m_settingsPage);

    auto *topBar = new QWidget(content);
    topBar->setObjectName(QStringLiteral("topBar"));
    topBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(0, 0, 0, 0);
    topBarLayout->setSpacing(PageLayout::Space16);

    auto *moduleBar = PageLayout::makeTabBar(m_moduleTitles, topBar, &m_moduleTabGroup, nullptr, 1);
    topBarLayout->addWidget(moduleBar, 1, Qt::AlignVCenter);

    contentLayout->addWidget(topBar);
    contentLayout->addWidget(m_contentStack, 1);
    rootLayout->addWidget(content, 1);

    if (m_moduleTabGroup != nullptr) {
        connect(m_moduleTabGroup, &QButtonGroup::idClicked, this, &MainWindow::onModuleChanged);
    }
    connect(m_navigation, &QListWidget::currentRowChanged, this, &MainWindow::onNavigationChanged);
    if (m_settingsButton != nullptr) {
        connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    }
    connect(m_projectManager, &ProjectManagerWidget::projectsChanged, this, &MainWindow::refreshDashboard);
    connect(m_projectManager, &ProjectManagerWidget::projectsChanged, this, &MainWindow::refreshDeploySelectors);
    connect(m_serverManager, &ServerManagerWidget::serversChanged, this, &MainWindow::refreshDashboard);
    connect(m_serverManager, &ServerManagerWidget::serversChanged, this, &MainWindow::refreshDeploySelectors);

    setCentralWidget(root);
    showModule(1);
    statusBar()->showMessage(QStringLiteral("配置目录：%1").arg(DataPaths::configDir()));

    PageLayout::applyMainWindowGeometry(this);

    refreshDashboard();
    refreshDeploySelectors();
    refreshDeploymentTables();
    refreshLocalLogFiles(false);
    refreshDeployLogPath();
}

MainWindow::~MainWindow() = default;

void MainWindow::addModule(const QString &title, const QList<QPair<QString, QWidget *>> &pages)
{
    m_moduleTitles.append(title);
    auto *stack = new QStackedWidget(m_moduleStack);
    QStringList labels;
    for (const auto &page : pages) {
        labels.append(page.first);
        stack->addWidget(PageLayout::wrapModulePage(page.second));
    }
    m_modulePages.append(stack);
    m_moduleNavigationLabels.append(labels);
    m_moduleStack->addWidget(stack);
}

void MainWindow::addModuleFromPages(const QString &title, const QStringList &labels, const QList<QWidget *> &pages)
{
    m_moduleTitles.append(title);
    auto *stack = new QStackedWidget(m_moduleStack);
    QStringList validLabels;
    QList<QWidget *> attachedPages;
    const int count = qMin(labels.size(), pages.size());
    for (int i = 0; i < count; ++i) {
        if (pages.at(i) == nullptr) {
            continue;
        }
        validLabels.append(labels.at(i));
        stack->addWidget(PageLayout::wrapModulePage(pages.at(i)));
        attachedPages.append(pages.at(i));
    }
    m_modulePages.append(stack);
    m_moduleNavigationLabels.append(validLabels);
    m_moduleStack->addWidget(stack);

    // Pages were taken from another QStackedWidget via removeWidget()+setParent(),
    // which leaves them explicitly hidden. Now that they are attached to the main
    // window hierarchy, clear the hidden state so the inner stack can display them.
    for (QWidget *page : attachedPages) {
        page->show();
    }
}

void MainWindow::showModule(int index)
{
    if (index < 0 || index >= m_modulePages.size()) {
        return;
    }
    showMainModuleContent();
    m_moduleStack->setCurrentIndex(index);
    if (m_moduleTabGroup != nullptr) {
        if (QAbstractButton *button = m_moduleTabGroup->button(index)) {
            m_moduleTabGroup->blockSignals(true);
            button->setChecked(true);
            m_moduleTabGroup->blockSignals(false);
        }
    }
    m_navigation->blockSignals(true);
    m_navigation->clear();
    for (const QString &label : m_moduleNavigationLabels.at(index)) {
        auto *item = new QListWidgetItem(makeSidebarLineIcon(label), label, m_navigation);
        item->setSizeHint(QSize(0, 40));
    }
    m_navigation->setCurrentRow(0);
    m_navigation->blockSignals(false);
    m_modulePages.at(index)->setCurrentIndex(0);
}

void MainWindow::showMainModuleContent()
{
    if (m_contentStack != nullptr) {
        m_contentStack->setCurrentIndex(0);
    }
    if (m_settingsButton != nullptr) {
        m_settingsButton->setChecked(false);
    }
    if (m_navigation != nullptr) {
        m_navigation->setEnabled(true);
    }
}

void MainWindow::onModuleChanged(int index)
{
    showModule(index);
}

void MainWindow::onNavigationChanged(int row)
{
    showMainModuleContent();
    const int moduleIndex = m_moduleStack != nullptr ? m_moduleStack->currentIndex() : -1;
    if (moduleIndex < 0 || moduleIndex >= m_modulePages.size() || row < 0) {
        return;
    }
    m_modulePages.at(moduleIndex)->setCurrentIndex(row);
}

void MainWindow::onSettingsClicked()
{
    if (m_contentStack == nullptr || m_settingsButton == nullptr) {
        return;
    }
    if (m_contentStack->currentIndex() == 1) {
        showMainModuleContent();
        return;
    }
    refreshAiSettingsSummary();
    m_contentStack->setCurrentIndex(1);
    m_settingsButton->setChecked(true);
    if (m_navigation != nullptr) {
        m_navigation->setEnabled(false);
    }
}

void MainWindow::openAiConfigPage()
{
    navigateToPage(0, 0);
}

void MainWindow::refreshAiSettingsSummary()
{
    if (m_aiSummaryUrl == nullptr || m_aiSummaryModel == nullptr || m_aiSummaryKeyStatus == nullptr) {
        return;
    }

    AiSettings settings;
    QString error;
    if (m_aiSettings != nullptr) {
        m_aiSettings->load(&settings, &error);
    }

    const QString unset = QStringLiteral("未配置");
    m_aiSummaryUrl->setText(settings.apiBaseUrl.trimmed().isEmpty() ? unset : settings.apiBaseUrl.trimmed());
    m_aiSummaryModel->setText(settings.model.trimmed().isEmpty() ? unset : settings.model.trimmed());

    const bool hasKey = m_credentials != nullptr && m_credentials->has(settings.credentialRef);
    m_aiSummaryKeyStatus->setText(hasKey ? QStringLiteral("已保存（凭据存储）") : unset);
}

void MainWindow::navigateToPage(int moduleIndex, int pageRow, int dashboardTabIndex)
{
    Q_UNUSED(dashboardTabIndex);
    showModule(moduleIndex);
    if (pageRow >= 0 && moduleIndex >= 0 && moduleIndex < m_modulePages.size()) {
        m_modulePages.at(moduleIndex)->setCurrentIndex(pageRow);
        m_navigation->blockSignals(true);
        m_navigation->setCurrentRow(pageRow);
        m_navigation->blockSignals(false);
    }
}

QFrame *MainWindow::metricCard(const QString &title, QLabel **valueLabel) const
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("metricCard"));
    auto *layout = new QVBoxLayout(frame);
    PageLayout::applyMetricCard(layout);
    auto *titleWidget = new QLabel(title);
    titleWidget->setObjectName(QStringLiteral("metricTitle"));
    *valueLabel = new QLabel(QStringLiteral("0"));
    (*valueLabel)->setObjectName(QStringLiteral("metricValue"));
    layout->addWidget(titleWidget);
    layout->addWidget(*valueLabel);
    return frame;
}

QWidget *MainWindow::createDashboardPage()
{
    auto *page = new QWidget;
    page->setProperty("fitFirstScreen", true);
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    auto *dashboardContent = new QFrame;
    dashboardContent->setObjectName(QStringLiteral("dashboardContent"));
    auto *dashboardLayout = new QVBoxLayout(dashboardContent);
    dashboardLayout->setContentsMargins(0, 0, 0, 0);
    dashboardLayout->setSpacing(0);

    auto *tabBar = PageLayout::makeModuleTabBar(
        {QStringLiteral("全部"), QStringLiteral("成功"), QStringLiteral("失败")},
        dashboardContent,
        &m_dashboardFilterGroup,
        nullptr,
        0);
    dashboardLayout->addWidget(tabBar);

    auto *contentPanel = new QFrame;
    contentPanel->setObjectName(QStringLiteral("contentPanel"));
    auto *panelLayout = new QVBoxLayout(contentPanel);
    panelLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    panelLayout->setSpacing(PageLayout::Space12);

    auto *recentTable = createTable(
        {QStringLiteral("时间"), QStringLiteral("项目"), QStringLiteral("服务器"), QStringLiteral("状态"), QStringLiteral("操作")},
        {});
    recentTable->setObjectName(QStringLiteral("dashboardTable"));
    m_recentTable = recentTable;
    PageLayout::configureListingTableWithActionColumn(m_recentTable, 4, 1);

    connect(m_recentTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (m_recentTable == nullptr || row < 0) {
            return;
        }
        const QTableWidgetItem *item = m_recentTable->item(row, 0);
        if (item != nullptr) {
            openDeploymentLog(item->data(Qt::UserRole).toString());
        }
    });

    auto *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    auto *sectionLabel = PageLayout::makeSectionLabel(QStringLiteral("最近部署任务"), contentPanel);
    sectionLabel->setWordWrap(false);
    sectionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    headerLayout->addWidget(sectionLabel);
    headerLayout->addStretch();
    auto *clearButton = new QPushButton(QStringLiteral("清空记录"));
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearDeploymentHistory);
    headerLayout->addWidget(clearButton);

    panelLayout->addLayout(headerLayout);
    panelLayout->addWidget(PageLayout::wrapTableSection(
        recentTable,
        &m_recentDeploymentsEmpty,
        QStringLiteral("暂无部署记录。完成一次部署后，最近动态会显示在这里。")), 1);

    dashboardLayout->addWidget(contentPanel, 1);
    pageLayout->addWidget(dashboardContent);

    if (m_dashboardFilterGroup != nullptr) {
        connect(m_dashboardFilterGroup, &QButtonGroup::idClicked, this, &MainWindow::refreshDashboard);
    }

    return page;
}

QWidget *MainWindow::createDeployPage()
{
    auto *page = new QWidget;
    page->setProperty("fitFirstScreen", true);
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);
    layout->setSpacing(PageLayout::Space16);
auto *body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(PageLayout::Space16);
    layout->addLayout(body, 1);

    auto *configColumn = new QWidget(page);
    configColumn->setObjectName(QStringLiteral("deployConfigColumn"));
    configColumn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    configColumn->setMinimumWidth(420);
    configColumn->setMaximumWidth(560);
    auto *configLayout = new QVBoxLayout(configColumn);
    configLayout->setContentsMargins(0, 0, 0, 0);
    configLayout->setSpacing(PageLayout::Space16);

    auto *formBox = new QGroupBox(QStringLiteral("部署配置"));
    formBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *form = new QFormLayout(formBox);
    PageLayout::applyForm(form);
    form->setContentsMargins(0, PageLayout::Space8, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_deployProject = new QComboBox;
    m_deployServer = new QComboBox;
    m_jdkSelector = new QComboBox;
    m_manageJdkButton = new QPushButton(QStringLiteral("管理 JDK"));
    PageLayout::configureFormInput(m_deployProject);
    PageLayout::configureFormInput(m_deployServer);
    PageLayout::configureFormInput(m_jdkSelector);
    m_deployProject->setMinimumWidth(360);
    m_deployServer->setMinimumWidth(360);
    m_jdkSelector->setMinimumWidth(260);
    form->addRow(QStringLiteral("项目"), m_deployProject);
    form->addRow(QStringLiteral("服务器"), m_deployServer);
    auto *jdkRow = new QWidget;
    auto *jdkRowLayout = new QHBoxLayout(jdkRow);
    jdkRowLayout->setContentsMargins(0, 0, 0, 0);
    jdkRowLayout->setSpacing(PageLayout::Space8);
    jdkRowLayout->addWidget(m_jdkSelector, 1);
    jdkRowLayout->addWidget(m_manageJdkButton);
    connect(m_manageJdkButton, &QPushButton::clicked, this, &MainWindow::manageJdkProfiles);
    form->addRow(QStringLiteral("JDK"), jdkRow);
    configLayout->addWidget(formBox);

    auto *statusBox = new QGroupBox(QStringLiteral("服务状态"));
    statusBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *statusForm = new QFormLayout(statusBox);
    PageLayout::applyForm(statusForm);
    statusForm->setContentsMargins(0, PageLayout::Space8, 0, 0);
    statusForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    statusForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_serviceStatusLabel = new QLabel(QStringLiteral("未检测"));
    m_serviceStatusLabel->setObjectName(QStringLiteral("serviceStatusBadge"));
    m_refreshServiceStatusButton = new QPushButton(QStringLiteral("刷新状态"));
    connect(m_refreshServiceStatusButton, &QPushButton::clicked, this, [this]() {
        refreshServiceStatus(true);
    });
    auto *statusRow = new QWidget;
    auto *statusRowLayout = new QHBoxLayout(statusRow);
    statusRowLayout->setContentsMargins(0, 0, 0, 0);
    statusRowLayout->setSpacing(PageLayout::Space8);
    statusRowLayout->addWidget(m_serviceStatusLabel, 1);
    statusRowLayout->addWidget(m_refreshServiceStatusButton);
    statusForm->addRow(QStringLiteral("运行状态"), statusRow);
    configLayout->addWidget(statusBox);

    auto *logBox = new QGroupBox(QStringLiteral("应用日志"));
    logBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *logForm = new QFormLayout(logBox);
    PageLayout::applyForm(logForm);
    logForm->setContentsMargins(0, PageLayout::Space8, 0, 0);
    logForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    logForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_logPathInput = new QComboBox;
    m_logPathInput->setEditable(true);
    m_logPathInput->setInsertPolicy(QComboBox::NoInsert);
    m_logPathInput->setProperty("manualEdit", true);
    m_logPathInput->setMinimumWidth(360);
    PageLayout::configureFormInput(m_logPathInput);
    if (QLineEdit *logPathEditor = m_logPathInput->lineEdit()) {
        logPathEditor->setReadOnly(false);
        logPathEditor->setPlaceholderText(
            QStringLiteral("选择或输入远程日志文件路径，例如 /home/app/logs/app.log"));
    }
    m_refreshLogListButton = new QPushButton(QStringLiteral("刷新列表"));
    m_viewLogButton = new QPushButton(QStringLiteral("一键查看"));
    m_viewLogButton->setObjectName(QStringLiteral("primaryButton"));
    connect(m_refreshLogListButton, &QPushButton::clicked, this, [this]() {
        refreshLocalLogFiles(true);
    });
    connect(m_viewLogButton, &QPushButton::clicked, this, &MainWindow::viewDeploymentLog);
    auto *logFieldRow = new QWidget;
    auto *logRowLayout = new QHBoxLayout(logFieldRow);
    logRowLayout->setContentsMargins(0, 0, 0, 0);
    logRowLayout->setSpacing(PageLayout::Space8);
    logRowLayout->addWidget(m_logPathInput, 1);
    logRowLayout->addWidget(m_refreshLogListButton);
    logRowLayout->addWidget(m_viewLogButton);
    logForm->addRow(QStringLiteral("远程日志"), logFieldRow);
    configLayout->addWidget(logBox);
    configLayout->addStretch(1);

    connect(m_deployProject, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::refreshDeployLogPath);
    connect(m_deployProject, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        refreshServiceStatus(false);
    });
    connect(m_deployServer, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() {
        refreshServiceStatus(false);
    });

    refreshLocalLogFiles(false);
    refreshJdkProfiles();

    auto *execBox = new QFrame(page);
    execBox->setObjectName(QStringLiteral("deployExecBox"));
    execBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto *execLayout = new QVBoxLayout(execBox);
    execLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    execLayout->setSpacing(PageLayout::Space16);

    auto *execHeader = new QHBoxLayout;
    execHeader->setContentsMargins(0, 0, 0, 0);
    execHeader->setSpacing(PageLayout::Space12);
    auto *execTitle = new QLabel(QStringLiteral("部署执行"));
    execTitle->setObjectName(QStringLiteral("deployOutputLabel"));
    execHeader->addWidget(execTitle);
    execHeader->addStretch(1);
    m_deployButton = new QPushButton(QStringLiteral("开始部署"));
    m_deployButton->setObjectName(QStringLiteral("primaryButton"));
    connect(m_deployButton, &QPushButton::clicked, this, &MainWindow::startDeployment);
    execHeader->addWidget(m_deployButton);
    execLayout->addLayout(execHeader);

    m_progress = new QProgressBar;
    m_progress->setObjectName(QStringLiteral("deployProgress"));
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    execLayout->addWidget(m_progress);

    auto *outputLabel = new QLabel(QStringLiteral("部署输出"));
    outputLabel->setObjectName(QStringLiteral("deployOutputLabel"));
    execLayout->addWidget(outputLabel);

    m_log = new QPlainTextEdit;
    m_log->setObjectName(QStringLiteral("deployLog"));
    m_log->setReadOnly(true);
    execLayout->addWidget(m_log, 1);

    body->addWidget(configColumn);
    body->addWidget(execBox, 1);
    return page;
}

QWidget *MainWindow::createHistoryPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);
auto *historyTable = createTable(
        {QStringLiteral("部署 ID"), QStringLiteral("项目"), QStringLiteral("状态"), QStringLiteral("版本"), QStringLiteral("日志")},
        {});
    m_historyTable = historyTable;
    connect(historyTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (m_historyTable == nullptr || row < 0) {
            return;
        }
        const QTableWidgetItem *logItem = m_historyTable->item(row, 4);
        if (logItem == nullptr) {
            return;
        }
        openDeploymentLog(logItem->text());
    });

    m_historyViewLogButton = new QPushButton(QStringLiteral("查看日志"));
    m_historyViewLogButton->setObjectName(QStringLiteral("primaryButton"));
    connect(m_historyViewLogButton, &QPushButton::clicked, this, &MainWindow::viewHistoryLog);
    m_clearHistoryButton = new QPushButton(QStringLiteral("清空记录"));
    connect(m_clearHistoryButton, &QPushButton::clicked, this, &MainWindow::clearDeploymentHistory);
    auto *historyToolbar = new QHBoxLayout;
    historyToolbar->setContentsMargins(0, 0, 0, PageLayout::Space8);
    historyToolbar->addStretch();
    historyToolbar->addWidget(m_clearHistoryButton);
    historyToolbar->addWidget(m_historyViewLogButton);

    layout->addLayout(historyToolbar);
    layout->addWidget(PageLayout::wrapTableSection(
        historyTable,
        &m_historyEmpty,
        QStringLiteral("暂无历史记录。执行部署后，记录会出现在此列表。")), 1);
    return page;
}

QWidget *MainWindow::createDeploySettingsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);
AppSettings settings;
    QString settingsError;
    AppSettingsStore(AppSettingsStore::defaultSettingsFile()).load(&settings, &settingsError);

    auto *settingsBox = new QGroupBox(QStringLiteral("全局设置"));
    settingsBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *form = new QFormLayout(settingsBox);
    PageLayout::applyForm(form);
    form->setContentsMargins(0, PageLayout::Space8, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto makePathRow = [this](QLineEdit **input, const QString &value, const QString &placeholder) {
        auto *row = new QWidget;
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(PageLayout::Space8);
        *input = new QLineEdit;
        (*input)->setText(value);
        (*input)->setPlaceholderText(placeholder);
        PageLayout::configureFormInput(*input);
        auto *browse = new QPushButton(QStringLiteral("浏览..."));
        connect(browse, &QPushButton::clicked, this, [input, this]() {
            const QString selected = QFileDialog::getExistingDirectory(this, QStringLiteral("选择目录"), (*input)->text());
            if (!selected.isEmpty()) {
                (*input)->setText(QDir::fromNativeSeparators(selected));
            }
        });
        rowLayout->addWidget(*input, 1);
        rowLayout->addWidget(browse);
        return row;
    };

    form->addRow(QStringLiteral("配置目录"), makePathRow(&m_configDirInput,
                                                     settings.configDirOverride,
                                                     DataPaths::configDir()));
    form->addRow(QStringLiteral("Maven 目录"), makePathRow(&m_mavenHomeInput,
                                                      settings.mavenHome,
                                                      QStringLiteral("D:/install/apache-maven")));
    form->addRow(QStringLiteral("Maven 仓库"), makePathRow(&m_mavenRepoInput,
                                                      settings.mavenRepository,
                                                      QStringLiteral("D:/m2/repository")));
    auto *saveButton = new QPushButton(QStringLiteral("保存设置"));
    saveButton->setObjectName(QStringLiteral("primaryButton"));
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveSettings);
    form->addRow(QString(), saveButton);
    layout->addWidget(settingsBox);

    layout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("当前路径"), page));
    layout->addWidget(createTable(
        {QStringLiteral("设置项"), QStringLiteral("当前值")},
        {
            {QStringLiteral("配置目录"), DataPaths::configDir()},
            {QStringLiteral("工作区"), DataPaths::workspaceDir()},
            {QStringLiteral("数据库"), DataPaths::databaseFile()},
            {QStringLiteral("日志目录"), DataPaths::logsDir()},
            {QStringLiteral("设置文件"), AppSettingsStore::defaultSettingsFile()}
        }));
    layout->addStretch();
    return page;
}

QWidget *MainWindow::createGlobalSettingsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);
AppSettings settings;
    QString settingsError;
    AppSettingsStore(AppSettingsStore::defaultSettingsFile()).load(&settings, &settingsError);

    auto makeFileRow = [this](QLineEdit **input, const QString &value, const QString &placeholder) {
        auto *row = new QWidget;
        PageLayout::configureHorizontalFormRow(row);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(PageLayout::Space8);
        *input = new QLineEdit;
        (*input)->setText(value);
        (*input)->setPlaceholderText(placeholder);
        PageLayout::configureFormInput(*input);
        auto *browse = new QPushButton(QStringLiteral("浏览..."));
        browse->setMinimumWidth(72);
        browse->setFixedHeight(PageLayout::DialogFieldHeight);
        browse->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        connect(browse, &QPushButton::clicked, this, [input, this]() {
            const QString selected = QFileDialog::getOpenFileName(this,
                                                                  QStringLiteral("选择 JDBC 驱动 JAR"),
                                                                  (*input)->text(),
                                                                  QStringLiteral("JAR 文件 (*.jar)"));
            if (!selected.isEmpty()) {
                (*input)->setText(QDir::fromNativeSeparators(selected));
            }
        });
        rowLayout->addWidget(*input, 1);
        rowLayout->addWidget(browse);
        return row;
    };

    auto *driverBox = new QGroupBox(QStringLiteral("数据库驱动 (JDBC)"));
    driverBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *driverLayout = new QVBoxLayout(driverBox);
    driverLayout->setContentsMargins(0, PageLayout::Space8, 0, 0);
    driverLayout->setSpacing(PageLayout::Space8);

    auto *driverForm = new QFormLayout;
    PageLayout::applyInlineForm(driverForm);
    driverForm->setVerticalSpacing(PageLayout::Space8);

    driverForm->addRow(QStringLiteral("PostgreSQL 驱动 JAR"),
                       makeFileRow(&m_postgresDriverJarInput,
                                   settings.postgresDriverJar,
                                   QStringLiteral("例如 postgresql-42.7.3.jar")));
    driverForm->addRow(QStringLiteral("Oracle 驱动 JAR"),
                       makeFileRow(&m_oracleDriverJarInput,
                                   settings.oracleDriverJar,
                                   QStringLiteral("例如 ojdbc8.jar")));

    auto *driverActions = new QWidget(driverBox);
    auto *driverActionsLayout = new QHBoxLayout(driverActions);
    driverActionsLayout->setContentsMargins(0, 0, 0, 0);
    driverActionsLayout->setSpacing(PageLayout::Space8);
    auto *probeButton = new QPushButton(QStringLiteral("检测驱动"), driverActions);
    probeButton->setFixedHeight(PageLayout::DialogFieldHeight);
    probeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(probeButton, &QPushButton::clicked, this, [this]() {
        AppSettings settings;
        AppSettingsStore store(AppSettingsStore::defaultSettingsFile());
        store.load(&settings, nullptr);
        if (m_postgresDriverJarInput != nullptr) {
            settings.postgresDriverJar = QDir::fromNativeSeparators(m_postgresDriverJarInput->text().trimmed());
        }
        if (m_oracleDriverJarInput != nullptr) {
            settings.oracleDriverJar = QDir::fromNativeSeparators(m_oracleDriverJarInput->text().trimmed());
        }
        store.save(settings, nullptr);
        const SqlDriverProbeResult probe = JdbcSqlBridge::probe();
        if (m_driverProbeLabel != nullptr) {
            m_driverProbeLabel->setText(probe.message);
        }
    });
    auto *saveJdbcButton = new QPushButton(QStringLiteral("保存驱动设置"), driverActions);
    saveJdbcButton->setObjectName(QStringLiteral("primaryButton"));
    saveJdbcButton->setFixedHeight(PageLayout::DialogFieldHeight);
    connect(saveJdbcButton, &QPushButton::clicked, this, &MainWindow::saveGlobalJdbcSettings);
    m_driverProbeLabel = new QLabel(QStringLiteral("JDBC 使用部署工具中配置的 JDK；选择驱动 JAR 后点击检测。"), driverActions);
    m_driverProbeLabel->setObjectName(QStringLiteral("mutedText"));
    m_driverProbeLabel->setWordWrap(true);
    driverActionsLayout->addWidget(probeButton);
    driverActionsLayout->addWidget(saveJdbcButton);
    driverActionsLayout->addWidget(m_driverProbeLabel, 1);
    driverLayout->addLayout(driverForm);
    driverLayout->addWidget(driverActions);
    layout->addWidget(driverBox);

    auto *aiBox = new QGroupBox(QStringLiteral("AI 辅助"));
    aiBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *aiLayout = new QVBoxLayout(aiBox);
    aiLayout->setContentsMargins(0, PageLayout::Space8, 0, 0);
    aiLayout->setSpacing(PageLayout::Space8);

    auto *aiHint = new QLabel(
        QStringLiteral("以下为只读摘要；编辑 API URL、Key、模型请前往「通用工具 → AI 配置」。"),
        aiBox);
    aiHint->setObjectName(QStringLiteral("mutedText"));
    aiHint->setWordWrap(true);
    aiLayout->addWidget(aiHint);

    auto *aiForm = new QFormLayout;
    PageLayout::applyInlineForm(aiForm);
    aiForm->setVerticalSpacing(PageLayout::Space8);

    m_aiSummaryUrl = new QLineEdit(aiBox);
    m_aiSummaryUrl->setReadOnly(true);
    PageLayout::configureFormInput(m_aiSummaryUrl);
    aiForm->addRow(QStringLiteral("API Base URL"), m_aiSummaryUrl);

    m_aiSummaryModel = new QLineEdit(aiBox);
    m_aiSummaryModel->setReadOnly(true);
    PageLayout::configureFormInput(m_aiSummaryModel);
    aiForm->addRow(QStringLiteral("模型"), m_aiSummaryModel);

    m_aiSummaryKeyStatus = new QLabel(QStringLiteral("未配置"), aiBox);
    m_aiSummaryKeyStatus->setObjectName(QStringLiteral("mutedText"));
    aiForm->addRow(QStringLiteral("API Key"), m_aiSummaryKeyStatus);

    aiForm->addRow(QStringLiteral("配置文件"),
                   new QLabel(AiSettingsStore::defaultSettingsFile(), aiBox));

    aiLayout->addLayout(aiForm);

    auto *aiActions = new QWidget(aiBox);
    auto *aiActionsLayout = new QHBoxLayout(aiActions);
    aiActionsLayout->setContentsMargins(0, 0, 0, 0);
    aiActionsLayout->setSpacing(PageLayout::Space8);
    auto *openAiConfigButton = new QPushButton(QStringLiteral("打开 AI 配置"), aiActions);
    openAiConfigButton->setObjectName(QStringLiteral("primaryButton"));
    openAiConfigButton->setFixedHeight(PageLayout::DialogFieldHeight);
    connect(openAiConfigButton, &QPushButton::clicked, this, &MainWindow::openAiConfigPage);
    aiActionsLayout->addWidget(openAiConfigButton);
    aiActionsLayout->addStretch();
    aiLayout->addWidget(aiActions);
    layout->addWidget(aiBox);

    refreshAiSettingsSummary();
    layout->addStretch();
    return page;
}

QTableWidget *MainWindow::createTable(const QStringList &headers, const QList<QStringList> &rows)
{
    auto *table = new QTableWidget(rows.size(), headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    PageLayout::configureDataTable(table);
    PageLayout::configureListingTable(table);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setDefaultSectionSize(40);
    for (int row = 0; row < rows.size(); ++row) {
        const auto values = rows.at(row);
        for (int column = 0; column < headers.size(); ++column) {
            table->setItem(row, column, new QTableWidgetItem(values.value(column)));
        }
    }
    PageLayout::refreshListingTableColumns(table);
    return table;
}

void MainWindow::refreshDashboard()
{
    refreshDashboardTabData();
}

void MainWindow::refreshDashboardTabData(const QVector<StoredRecord> *deployments)
{
    QVector<StoredRecord> loadedDeployments;
    const QVector<StoredRecord> *deploymentSource = deployments;
    if (deploymentSource == nullptr) {
        QString error;
        if (!m_store->listDeployments(&loadedDeployments, &error)) {
            statusBar()->showMessage(QStringLiteral("部署数据加载失败：%1").arg(error));
            return;
        }
        deploymentSource = &loadedDeployments;
    }

    if (m_recentTable == nullptr) {
        return;
    }

    int filterId = 0;
    if (m_dashboardFilterGroup != nullptr) {
        filterId = m_dashboardFilterGroup->checkedId();
    }

    QVector<StoredRecord> filtered;
    filtered.reserve(deploymentSource->size());
    for (const StoredRecord &record : *deploymentSource) {
        const QString status = record.config.value(QStringLiteral("status")).toString();
        if (filterId == 1 && status != QStringLiteral("success")) {
            continue;
        }
        if (filterId == 2 && status != QStringLiteral("failed") && status != QStringLiteral("rollback_failed")) {
            continue;
        }
        filtered.append(record);
    }

    const int rowCount = filtered.size();
    m_recentTable->setRowCount(rowCount);
    for (int row = 0; row < rowCount; ++row) {
        const QJsonObject &record = filtered.at(row).config;
        const QString startedAt = formatStartedAt(record);
        const QString projectId = record.value(QStringLiteral("projectId")).toString();
        const QString serverId = record.value(QStringLiteral("serverId")).toString();
        const QString status = record.value(QStringLiteral("status")).toString();
        const QString logPath = record.value(QStringLiteral("logPath")).toString();

        auto *timeItem = new QTableWidgetItem(startedAt);
        timeItem->setData(Qt::UserRole, logPath);
        m_recentTable->setItem(row, 0, timeItem);
        m_recentTable->setItem(row, 1, new QTableWidgetItem(projectId));
        m_recentTable->setItem(row, 2, new QTableWidgetItem(serverId));
        m_recentTable->setItem(row, 3, new QTableWidgetItem(formatDeploymentStatusLabel(status)));

        auto *logButton = PageLayout::makeTableActionButton(QStringLiteral("日志"), QStringLiteral("tableActionView"), m_recentTable);
        connect(logButton, &QPushButton::clicked, this, [this, logPath]() {
            openDeploymentLog(logPath);
        });
        m_recentTable->setCellWidget(row, 4, logButton);
    }

    PageLayout::refreshListingTableColumns(m_recentTable, 1);
    PageLayout::updateTableEmptyState(m_recentTable, m_recentDeploymentsEmpty, rowCount);
}

void MainWindow::refreshDeploySelectors()
{
    m_deployProject->clear();
    m_deployServer->clear();

    for (const QString &id : m_projectManager->projectIds()) {
        m_deployProject->addItem(id, id);
    }
    for (const QString &id : m_serverManager->serverIds()) {
        m_deployServer->addItem(id, id);
    }
    refreshLocalLogFiles(false);
    refreshDeployLogPath();
    refreshJdkProfiles();
}

void MainWindow::refreshJdkProfiles()
{
    if (m_jdkSelector == nullptr) {
        return;
    }

    QString current = m_jdkSelector->currentData().toString();
    if (current.isEmpty()) {
        current = QStringLiteral("system");
    }
    m_jdkSelector->clear();
    const JdkProfile system = JdkProfileStore::systemProfile();
    m_jdkSelector->addItem(system.version, system.id);

    QVector<JdkProfile> profiles;
    QString error;
    JdkProfileStore store(DataPaths::jdkProfilesFile());
    if (store.load(&profiles, &error)) {
        for (const JdkProfile &profile : profiles) {
            m_jdkSelector->addItem(QStringLiteral("%1  ·  %2").arg(profile.version, profile.path), profile.id);
        }
    }
    const int index = m_jdkSelector->findData(current);
    m_jdkSelector->setCurrentIndex(index >= 0 ? index : 0);
}

void MainWindow::manageJdkProfiles()
{
    JdkProfileDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        refreshJdkProfiles();
    }
}

void MainWindow::saveSettings()
{
    AppSettings settings;
    QString loadError;
    AppSettingsStore store(AppSettingsStore::defaultSettingsFile());
    store.load(&settings, &loadError);

    if (m_configDirInput != nullptr) {
        settings.configDirOverride = QDir::fromNativeSeparators(m_configDirInput->text().trimmed());
    }
    if (m_mavenHomeInput != nullptr) {
        settings.mavenHome = QDir::fromNativeSeparators(m_mavenHomeInput->text().trimmed());
    }
    if (m_mavenRepoInput != nullptr) {
        settings.mavenRepository = QDir::fromNativeSeparators(m_mavenRepoInput->text().trimmed());
    }

    if (!settings.mavenHome.isEmpty()) {
#ifdef Q_OS_WIN
        const QString mvnPath = QDir(settings.mavenHome).filePath(QStringLiteral("bin/mvn.cmd"));
#else
        const QString mvnPath = QDir(settings.mavenHome).filePath(QStringLiteral("bin/mvn"));
#endif
        if (!QFileInfo::exists(mvnPath)) {
            QMessageBox::warning(this,
                                 QStringLiteral("Maven 路径无效"),
                                 QStringLiteral("未找到 %1。\n请填写 Maven 安装根目录，例如 D:/install/apache-maven-3.9.9")
                                     .arg(mvnPath));
            return;
        }
    }

    QString error;
    if (!store.save(settings, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("已保存"),
                             QStringLiteral("Maven 设置已写入 %1，下一次部署自动生效；配置目录变更需要重启应用。")
                                 .arg(AppSettingsStore::defaultSettingsFile()));
}

void MainWindow::saveGlobalJdbcSettings()
{
    AppSettings settings;
    QString loadError;
    AppSettingsStore store(AppSettingsStore::defaultSettingsFile());
    store.load(&settings, &loadError);

    if (m_postgresDriverJarInput != nullptr) {
        settings.postgresDriverJar = QDir::fromNativeSeparators(m_postgresDriverJarInput->text().trimmed());
    }
    if (m_oracleDriverJarInput != nullptr) {
        settings.oracleDriverJar = QDir::fromNativeSeparators(m_oracleDriverJarInput->text().trimmed());
    }

    QString error;
    if (!store.save(settings, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("已保存"),
                             QStringLiteral("数据库驱动设置已写入 %1。")
                                 .arg(AppSettingsStore::defaultSettingsFile()));
}

void MainWindow::refreshDeploymentTables()
{
    QVector<StoredRecord> deployments;
    QString error;
    if (!m_store->listDeployments(&deployments, &error)) {
        statusBar()->showMessage(QStringLiteral("部署数据加载失败：%1").arg(error));
        return;
    }

    if (m_historyTable != nullptr) {
        m_historyTable->setRowCount(deployments.size());
        for (int row = 0; row < deployments.size(); ++row) {
            const QJsonObject &record = deployments.at(row).config;
            const QString logPath = record.value(QStringLiteral("logPath")).toString();
            m_historyTable->setItem(row, 0, new QTableWidgetItem(record.value(QStringLiteral("id")).toString()));
            m_historyTable->setItem(row, 1, new QTableWidgetItem(record.value(QStringLiteral("projectId")).toString()));
            m_historyTable->setItem(row, 2, new QTableWidgetItem(record.value(QStringLiteral("status")).toString()));
            m_historyTable->setItem(row, 3, new QTableWidgetItem(record.value(QStringLiteral("version")).toString()));
            auto *logItem = new QTableWidgetItem(logPath);
            logItem->setData(Qt::UserRole, logPath);
            m_historyTable->setItem(row, 4, logItem);
        }
        PageLayout::refreshListingTableColumns(m_historyTable);
        PageLayout::updateTableEmptyState(m_historyTable, m_historyEmpty, deployments.size());
    }

    refreshDashboardTabData(&deployments);
}

void MainWindow::applyRemoteLogPathOptions(const QStringList &options, const QString &preferred)
{
    if (m_logPathInput == nullptr) {
        return;
    }

    QStringList merged = options;
    if (!preferred.isEmpty() && !merged.contains(preferred)) {
        merged.prepend(preferred);
    }

    m_logPathInput->clear();
    m_logPathInput->addItems(merged);
    m_logPathInput->setProperty("manualEdit", true);
    m_logPathInput->setInsertPolicy(QComboBox::NoInsert);
    if (QLineEdit *editor = m_logPathInput->lineEdit()) {
        editor->setReadOnly(false);
    }

    const QString pick = preferred.isEmpty() && !merged.isEmpty() ? merged.first() : preferred;
    if (pick.isEmpty()) {
        return;
    }

    const int index = m_logPathInput->findText(pick);
    if (index >= 0) {
        m_logPathInput->setCurrentIndex(index);
    } else {
        m_logPathInput->setEditText(pick);
    }
}

void MainWindow::refreshLocalLogFiles()
{
    refreshLocalLogFiles(true);
}

void MainWindow::refreshLocalLogFiles(bool allowRemotePrompt)
{
    if (m_logPathInput == nullptr) {
        return;
    }

    const QString current = m_logPathInput->currentText().trimmed();
    QString preferred = current;
    QStringList options;
    if (m_deployProject != nullptr && m_deployProject->count() > 0) {
        QJsonObject project;
        QString error;
        if (m_store->getProject(m_deployProject->currentData().toString(), &project, &error)) {
            const QString configured = defaultRemoteLogPath(project);
            if (current.isEmpty() && !configured.isEmpty()) {
                preferred = configured;
            }
            if (!configured.isEmpty()) {
                options.append(configured);
            }
        }
    }

    applyRemoteLogPathOptions(options, preferred);
    refreshRemoteLogPathOptions(allowRemotePrompt);
}

void MainWindow::refreshRemoteLogPathOptions()
{
    refreshRemoteLogPathOptions(true);
}

void MainWindow::refreshRemoteLogPathOptions(bool allowPrompt)
{
    if (m_logPathInput == nullptr || m_deployProject == nullptr || m_deployProject->count() == 0
        || m_deployServer == nullptr || m_deployServer->count() == 0) {
        return;
    }

    const QString current = m_logPathInput->currentText().trimmed();
    const int generation = ++m_remoteLogRefreshGeneration;

    QJsonObject project;
    QJsonObject server;
    QString error;
    if (!m_store->getProject(m_deployProject->currentData().toString(), &project, &error)
        || !m_store->getServer(m_deployServer->currentData().toString(), &server, &error)) {
        return;
    }

    const QString configuredDefault = defaultRemoteLogPath(project);
    if (!allowPrompt) {
        return;
    }

    RemoteConnectionContext context =
        RemoteCredentialResolver::resolve(server, m_credentials.get(), m_sessionCache.get(), this, allowPrompt);
    const QJsonObject auth = server.value(QStringLiteral("auth")).toObject();
    if (auth.value(QStringLiteral("mode")).toString() != QStringLiteral("ssh-key")
        && context.password.isEmpty()) {
        return;
    }

    auto *watcher = new QFutureWatcher<QStringList>(this);
    connect(watcher, &QFutureWatcher<QStringList>::finished, this, [this, watcher, generation, current, configuredDefault]() {
        watcher->deleteLater();
        if (generation != m_remoteLogRefreshGeneration) {
            return;
        }

        const QStringList remoteOptions = watcher->result();
        if (remoteOptions.isEmpty()) {
            return;
        }

        const QString typed = m_logPathInput->currentText().trimmed();
        QString preferred = typed.isEmpty() ? current : typed;
        if (preferred.isEmpty() && !configuredDefault.isEmpty()) {
            preferred = configuredDefault;
        }
        applyRemoteLogPathOptions(remoteOptions, preferred);
    });

    const QJsonObject projectCopy = project;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext contextCopy = context;
    watcher->setFuture(QtConcurrent::run([projectCopy, serverCopy, contextCopy]() {
        auto executor = createRemoteExecutor(contextCopy);
        if (!executor) {
            return QStringList{};
        }

        const QString base = remoteBaseDirFromProject(projectCopy);
        const QString discoverCommand = remoteDiscoverLogDirectoriesCommand(serverCopy, base);
        if (discoverCommand.isEmpty()) {
            return QStringList{};
        }

        const RemoteCommandResult dirResult = executor->execute(discoverCommand, 20);
        QStringList directories = mergeRemoteLogDirectories(
            projectCopy,
            dirResult.ok ? parseDiscoveredLogDirectories(dirResult.stdoutText) : QStringList{});
        if (directories.isEmpty()) {
            directories = candidateRemoteLogDirectories(projectCopy);
        }

        const QString discoverFilesCommand = remoteDiscoverLogFilesCommand(serverCopy, directories);
        if (discoverFilesCommand.isEmpty()) {
            const QString configured = defaultRemoteLogPath(projectCopy);
            return configured.isEmpty() ? QStringList{} : QStringList{configured};
        }

        const RemoteCommandResult fileResult = executor->execute(discoverFilesCommand, 30);
        QStringList files = fileResult.ok ? parseDiscoveredLogFiles(fileResult.stdoutText) : QStringList{};
        const QString configured = defaultRemoteLogPath(projectCopy);
        if (!configured.isEmpty() && !files.contains(configured)) {
            files.prepend(configured);
        }
        return files;
    }));
}

void MainWindow::refreshDeployLogPath()
{
    if (m_logPathInput == nullptr || m_deployProject == nullptr || m_deployProject->count() == 0) {
        return;
    }

    const QString projectId = m_deployProject->currentData().toString();
    QJsonObject project;
    QString error;
    if (!m_store->getProject(projectId, &project, &error)) {
        return;
    }

    const QString logPath = defaultRemoteLogPath(project);
    if (logPath.isEmpty()) {
        return;
    }

    const int index = m_logPathInput->findText(logPath);
    if (index >= 0) {
        m_logPathInput->setCurrentIndex(index);
    } else {
        m_logPathInput->setEditText(logPath);
    }
}

void MainWindow::refreshServiceStatus()
{
    refreshServiceStatus(true);
}

void MainWindow::refreshServiceStatus(bool allowPrompt)
{
    if (m_serviceStatusLabel == nullptr || m_deployProject == nullptr || m_deployServer == nullptr) {
        return;
    }
    if (m_deployProject->count() == 0 || m_deployServer->count() == 0) {
        m_serviceStatusLabel->setText(QStringLiteral("请先配置项目与服务器"));
        return;
    }
    if (!allowPrompt) {
        m_serviceStatusLabel->setText(QStringLiteral("未检测"));
        return;
    }

    const QString projectId = m_deployProject->currentData().toString();
    const QString serverId = m_deployServer->currentData().toString();
    QJsonObject project;
    QJsonObject server;
    QString error;
    if (!m_store->getProject(projectId, &project, &error) || !m_store->getServer(serverId, &server, &error)) {
        m_serviceStatusLabel->setText(QStringLiteral("配置加载失败"));
        return;
    }

    const RemoteConnectionContext context =
        RemoteCredentialResolver::resolve(server, m_credentials.get(), m_sessionCache.get(), this, allowPrompt);
    const QJsonObject auth = server.value(QStringLiteral("auth")).toObject();
    if (auth.value(QStringLiteral("mode")).toString() != QStringLiteral("ssh-key")
        && context.password.isEmpty()) {
        m_serviceStatusLabel->setText(QStringLiteral("未获取到服务器密码"));
        return;
    }
    const auto monitor = createRemoteMonitor(context);
    const ServiceStatusResult status = monitor->queryServiceStatus(server, project);
    if (!status.ok) {
        m_serviceStatusLabel->setText(status.error);
        return;
    }

    m_serviceStatusLabel->setText(QStringLiteral("%1 - %2")
                                      .arg(serviceRunStateLabel(status.state), status.message));
}

void MainWindow::openDeploymentLog(const QString &relativePath)
{
    const QString trimmed = relativePath.trimmed();
    if (trimmed.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("查看日志"),
                                 QStringLiteral("请选择或输入日志路径，例如 /home/app/logs/*.log 或 /home/app/logs/*.txt"));
        return;
    }

    QJsonObject server;
    if (isRemoteDeployLogPath(trimmed) && m_deployServer != nullptr && m_deployServer->count() > 0) {
        QString error;
        if (!m_store->getServer(m_deployServer->currentData().toString(), &server, &error)) {
            QMessageBox::warning(this, QStringLiteral("无法打开远程日志"), error);
            return;
        }
    }

    DeploymentLogOpener::open(this, m_credentials.get(), m_sessionCache.get(), server, trimmed, m_aiSettings.get());
}

void MainWindow::openRemoteDeploymentLog(const QString &remotePath)
{
    if (m_deployServer == nullptr || m_deployServer->count() == 0) {
        QMessageBox::information(this,
                                 QStringLiteral("查看日志"),
                                 QStringLiteral("请先选择服务器。"));
        return;
    }

    QJsonObject server;
    QString error;
    if (!m_store->getServer(m_deployServer->currentData().toString(), &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("无法打开远程日志"), error);
        return;
    }

    DeploymentLogOpener::open(this, m_credentials.get(), m_sessionCache.get(), server, remotePath);
}

QString MainWindow::selectedHistoryLogPath() const
{
    if (m_historyTable == nullptr) {
        return {};
    }
    const QList<QTableWidgetItem *> selected = m_historyTable->selectedItems();
    if (selected.isEmpty()) {
        return {};
    }
    const int row = selected.first()->row();
    const QTableWidgetItem *logItem = m_historyTable->item(row, 4);
    return logItem != nullptr ? logItem->text() : QString();
}

void MainWindow::viewDeploymentLog()
{
    if (m_logPathInput == nullptr) {
        return;
    }
    openDeploymentLog(m_logPathInput->currentText());
}

void MainWindow::viewHistoryLog()
{
    const QString logPath = selectedHistoryLogPath();
    if (logPath.isEmpty()) {
        QMessageBox::information(this,
                                 QStringLiteral("查看日志"),
                                 QStringLiteral("请先在历史记录中选择一条部署。"));
        return;
    }
    openDeploymentLog(logPath);
}

void MainWindow::clearDeploymentHistory()
{
    if (m_deployRunning) {
        QMessageBox::information(this,
                                 QStringLiteral("无法清空"),
                                 QStringLiteral("部署进行中，请等待完成后再清空记录。"));
        return;
    }

    QVector<StoredRecord> deployments;
    QString error;
    if (!m_store->listDeployments(&deployments, &error)) {
        QMessageBox::warning(this, QStringLiteral("清空失败"), error);
        return;
    }

    const int recordCount = deployments.size();
    const int logFileCount = LocalLogCatalog::listRelativeLogFiles().size();
    if (recordCount == 0 && logFileCount == 0) {
        QMessageBox::information(this,
                                 QStringLiteral("无需清空"),
                                 QStringLiteral("当前没有部署记录或本地部署日志。"));
        return;
    }

    const auto answer = QMessageBox::question(
        this,
        QStringLiteral("确认清空"),
        QStringLiteral("确定清空全部 %1 条部署记录，并删除本地 %2 个部署日志文件？\n\n"
                       "此操作不可恢复，不会删除远端服务器上的文件。")
            .arg(recordCount)
            .arg(logFileCount),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        return;
    }

    int removedRecords = 0;
    if (!m_store->clearAllDeployments(&removedRecords, &error)) {
        QMessageBox::warning(this, QStringLiteral("清空失败"), error);
        return;
    }

    const LocalLogCleanupResult logResult = LocalLogCatalog::clearAll();
    if (m_log != nullptr) {
        m_log->clear();
    }

    refreshDeploymentTables();
    refreshDashboard();

    QString summary = QStringLiteral("已清空 %1 条部署记录，删除 %2 个日志文件，释放约 %3")
                          .arg(removedRecords)
                          .arg(logResult.removedFiles)
                          .arg(formatByteSize(logResult.freedBytes));
    if (logResult.failedFiles > 0) {
        summary += QStringLiteral("（%1 个日志文件删除失败）").arg(logResult.failedFiles);
    }
    statusBar()->showMessage(summary, 8000);
    QMessageBox::information(this, QStringLiteral("清空完成"), summary);
}

void MainWindow::startDeployment()
{
    if (m_deployRunning) {
        return;
    }

    if (m_deployProject->count() == 0 || m_deployServer->count() == 0) {
        QMessageBox::information(this,
                                 QStringLiteral("无法部署"),
                                 QStringLiteral("请先在「部署工具 → 项目管理」和「部署工具 → 服务器管理」中完成配置。"));
        return;
    }

    const QString projectId = m_deployProject->currentData().toString();
    const QString serverId = m_deployServer->currentData().toString();

    QJsonObject server;
    QString loadError;
    if (!m_store->getServer(serverId, &server, &loadError)) {
        QMessageBox::warning(this, QStringLiteral("无法部署"), loadError);
        return;
    }

    RemoteConnectionContext connectionContext =
        RemoteCredentialResolver::resolve(server, m_credentials.get(), m_sessionCache.get(), this);
    const QString authMode = server.value(QStringLiteral("auth")).toObject().value(QStringLiteral("mode")).toString();
    if (authMode != QStringLiteral("ssh-key") && connectionContext.password.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("无法部署"),
                             QStringLiteral("未获取到服务器密码，请先在「部署工具 → 服务器管理」中保存密码。"));
        return;
    }

    m_deployRunning = true;
    m_log->clear();
    m_progress->setValue(0);
    m_deployButton->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("部署进行中…"));

    auto *thread = new QThread;
    QString selectedJdkId = m_jdkSelector != nullptr
        ? m_jdkSelector->currentData().toString()
        : QString();
    if (selectedJdkId.isEmpty()) {
        selectedJdkId = QStringLiteral("system");
    }
    auto *worker = new DeployWorker(DataPaths::databaseFile(), selectedJdkId, connectionContext);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, [worker, projectId, serverId]() {
        worker->run(projectId, serverId);
    });
    connect(worker, &DeployWorker::logLine, this, &MainWindow::appendLog, Qt::QueuedConnection);
    connect(worker, &DeployWorker::progress, m_progress, &QProgressBar::setValue, Qt::QueuedConnection);
    connect(worker, &DeployWorker::deploymentLogReady, this, [this](const QString &logRelativePath) {
        if (logRelativePath.isEmpty()) {
            return;
        }
        appendLog(QStringLiteral("LOG"),
                  QStringLiteral("Deploy Hub 完整日志：%1（可在 config/%1 查看）").arg(logRelativePath));
    }, Qt::QueuedConnection);
    connect(worker, &DeployWorker::finished, this, &MainWindow::onDeploymentFinished, Qt::QueuedConnection);
    connect(worker, &DeployWorker::finished, thread, &QThread::quit, Qt::QueuedConnection);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void MainWindow::onDeploymentFinished(bool ok, const QString &summary, const QString &logRelativePath)
{
    m_deployRunning = false;
    m_deployButton->setEnabled(true);
    statusBar()->showMessage(ok ? QStringLiteral("部署完成") : QStringLiteral("部署失败"));

    if (!logRelativePath.isEmpty()) {
        appendLog(QStringLiteral("LOG"),
                  QStringLiteral("Deploy Hub 完整日志：%1").arg(logRelativePath));
    }
    appendLog(QStringLiteral("RESULT"), ok ? QStringLiteral("部署成功") : QStringLiteral("部署失败"));

    refreshDeploymentTables();
    refreshServiceStatus();

    if (!ok) {
        QMessageBox::warning(this, QStringLiteral("部署失败"), summary);
    }
}

void MainWindow::appendLog(const QString &stage, const QString &message)
{
    m_log->appendPlainText(QStringLiteral("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), stage, message));
}
