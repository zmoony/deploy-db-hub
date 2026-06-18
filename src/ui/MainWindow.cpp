#include "ui/MainWindow.h"

#include "infra/ConfigStore.h"
#include "infra/DataPaths.h"
#include "infra/AppSettingsStore.h"
#include "infra/JdkProfileStore.h"
#include "infra/LocalLogCatalog.h"
#include "adapters/remote/RemoteFileBrowser.h"
#include "ui/RemoteCredentialResolver.h"
#include "adapters/remote/RemoteMonitor.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "ui/DeployLogPathOptions.h"
#include "ui/DeployWorker.h"
#include "ui/DeploymentLogDialog.h"
#include "ui/JdkProfileDialog.h"
#include "ui/PageLayout.h"
#include "ui/ProjectManagerWidget.h"
#include "ui/RemoteFileViewerDialog.h"
#include "ui/RemoteUiHelpers.h"
#include "ui/ServerManagerWidget.h"
#include "infra/AppBranding.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>

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

QString formatDuration(const QJsonObject &record)
{
    const QJsonValue finishedValue = record.value(QStringLiteral("finishedAt"));
    if (finishedValue.isNull()) {
        return QStringLiteral("-");
    }
    const QDateTime started = QDateTime::fromString(record.value(QStringLiteral("startedAt")).toString(), Qt::ISODate);
    const QDateTime finished = QDateTime::fromString(finishedValue.toString(), Qt::ISODate);
    if (!started.isValid() || !finished.isValid()) {
        return QStringLiteral("-");
    }
    return QStringLiteral("%1s").arg(started.secsTo(finished));
}

QString formatServerOsLabel(const QJsonObject &server)
{
    const QString os = server.value(QStringLiteral("os")).toString();
    if (os == QStringLiteral("windows")) {
        return QStringLiteral("Windows WinRM");
    }
    if (os == QStringLiteral("linux")) {
        return QStringLiteral("Linux SSH");
    }
    return os.isEmpty() ? QStringLiteral("-") : os;
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

bool isPendingFailureStatus(const QString &status)
{
    return status == QStringLiteral("failed") || status == QStringLiteral("rollback_failed");
}

QLabel *makeDashboardLabel(const QString &text, const QString &objectName)
{
    auto *label = new QLabel(text);
    label->setObjectName(objectName);
    return label;
}

QWidget *makeDashboardHero()
{
    auto *hero = new QWidget;
    hero->setObjectName(QStringLiteral("dashboardHero"));
    auto *layout = new QVBoxLayout(hero);
    layout->setContentsMargins(PageLayout::Space24, PageLayout::Space24, PageLayout::Space24, PageLayout::Space24);
    layout->setSpacing(PageLayout::Space8);
    layout->addWidget(makeDashboardLabel(QStringLiteral("DEPLOY CONTROL"), QStringLiteral("dashboardKicker")));
    layout->addWidget(makeDashboardLabel(QStringLiteral("部署资源态势"), QStringLiteral("dashboardTitle")));
    layout->addWidget(makeDashboardLabel(
        QStringLiteral("项目、服务器与最近部署聚合在一屏，适合快速判断当前部署面。"),
        QStringLiteral("dashboardMeta")));
    return hero;
}

QFrame *makeDashboardStatCard(const QString &title, const QString &subtitle, QLabel **valueLabel)
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("dashboardStatCard"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    layout->setSpacing(PageLayout::Space8);

    auto *label = makeDashboardLabel(title, QStringLiteral("dashboardStatLabel"));
    *valueLabel = makeDashboardLabel(QStringLiteral("0"), QStringLiteral("dashboardStatValue"));
    auto *meta = makeDashboardLabel(subtitle, QStringLiteral("dashboardMiniLabel"));
    meta->setWordWrap(true);

    layout->addWidget(label);
    layout->addWidget(*valueLabel);
    layout->addWidget(meta);
    layout->addStretch();
    return frame;
}

QWidget *makeDashboardMiniMetric(const QString &label, const QString &value)
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space8);
    layout->addWidget(makeDashboardLabel(value, QStringLiteral("dashboardMiniValue")));
    layout->addWidget(makeDashboardLabel(label, QStringLiteral("dashboardMiniLabel")));
    return widget;
}

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_store(std::make_unique<ConfigStore>(DataPaths::databaseFile()))
    , m_credentials(std::make_unique<CredentialStore>(DataPaths::credentialsFile()))
    , m_sessionCache(std::make_unique<CredentialSessionCache>())
{
    setWindowTitle(QStringLiteral("Deploy Hub - 可视化部署工具"));
    AppBranding::applyWindowIcon(this);
    PageLayout::enableResizableWindow(this);
    PageLayout::fitWindowToScreen(this, PageLayout::MainWindowDefaultWidth, PageLayout::MainWindowDefaultHeight);

    QString error;
    if (!m_store->open(&error)) {
        QMessageBox::critical(this, QStringLiteral("数据库错误"), error);
    }

    auto *root = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    rootLayout->setSpacing(PageLayout::Space12);

    m_navigation = new QListWidget(root);
    m_navigation->setObjectName(QStringLiteral("sidebarNav"));
    m_navigation->setFixedWidth(180);
    m_navigation->addItems({
        QStringLiteral("仪表盘"),
        QStringLiteral("项目管理"),
        QStringLiteral("服务器管理"),
        QStringLiteral("一键部署"),
        QStringLiteral("历史记录"),
        QStringLiteral("设置")
    });
    m_navigation->setCurrentRow(0);

    m_pages = new QStackedWidget(root);
    m_pages->addWidget(PageLayout::wrapContentPanel(createDashboardPage()));
    m_projectManager = new ProjectManagerWidget(m_store.get());
    m_serverManager = new ServerManagerWidget(m_store.get());
    m_pages->addWidget(PageLayout::wrapContentPanel(m_projectManager));
    m_pages->addWidget(PageLayout::wrapContentPanel(m_serverManager));
    m_pages->addWidget(PageLayout::wrapContentPanel(createDeployPage()));
    m_pages->addWidget(PageLayout::wrapContentPanel(createHistoryPage()));
    m_pages->addWidget(PageLayout::wrapContentPanel(createSettingsPage()));

    connect(m_navigation, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(m_projectManager, &ProjectManagerWidget::projectsChanged, this, &MainWindow::refreshDashboard);
    connect(m_projectManager, &ProjectManagerWidget::projectsChanged, this, &MainWindow::refreshDeploySelectors);
    connect(m_serverManager, &ServerManagerWidget::serversChanged, this, &MainWindow::refreshDashboard);
    connect(m_serverManager, &ServerManagerWidget::serversChanged, this, &MainWindow::refreshDeploySelectors);

    rootLayout->addWidget(m_navigation);
    rootLayout->addWidget(m_pages, 1);
    setCentralWidget(root);
    statusBar()->showMessage(QStringLiteral("配置目录：%1").arg(DataPaths::configDir()));

    refreshDashboard();
    refreshDeploySelectors();
    refreshDeploymentTables();
    refreshLocalLogFiles();
    refreshDeployLogPath();
    refreshServiceStatus();
}

MainWindow::~MainWindow() = default;

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
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);
    layout->setSpacing(PageLayout::Space16);

    auto *tabBar = new QWidget(page);
    tabBar->setObjectName(QStringLiteral("dashboardTabBar"));
    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(4, 4, 4, 4);
    tabLayout->setSpacing(PageLayout::Space8);

    auto *tabGroup = new QButtonGroup(page);
    tabGroup->setExclusive(true);
    const QStringList tabLabels = {
        QStringLiteral("总览"),
        QStringLiteral("部署"),
        QStringLiteral("服务器"),
        QStringLiteral("日志")
    };
    for (int i = 0; i < tabLabels.size(); ++i) {
        auto *button = new QPushButton(tabLabels.at(i), tabBar);
        button->setObjectName(QStringLiteral("tabButton"));
        button->setCheckable(true);
        button->setChecked(i == 0);
        tabGroup->addButton(button, i);
        tabLayout->addWidget(button);
    }
    tabLayout->addStretch();
    layout->addWidget(tabBar);

    m_dashboardStack = new QStackedWidget(page);
    layout->addWidget(m_dashboardStack, 1);
    connect(tabGroup, &QButtonGroup::idClicked, m_dashboardStack, &QStackedWidget::setCurrentIndex);

    auto *overviewPage = new QWidget;
    auto *overviewLayout = new QVBoxLayout(overviewPage);
    overviewLayout->setContentsMargins(0, 0, 0, 0);
    overviewLayout->setSpacing(PageLayout::Space16);

    overviewLayout->addWidget(makeDashboardHero());

    auto *metrics = new QGridLayout;
    metrics->setHorizontalSpacing(PageLayout::Space16);
    metrics->setVerticalSpacing(PageLayout::Space16);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("项目"), QStringLiteral("已登记部署单元"), &m_metricProjects), 0, 0);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("服务器"), QStringLiteral("Linux / Windows 目标机"), &m_metricServers), 0, 1);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("最近成功"), QStringLiteral("最近部署结果"), &m_metricRecentSuccess), 0, 2);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("待处理失败"), QStringLiteral("需要关注的失败项"), &m_metricPendingFailures), 0, 3);
    m_metricRecentSuccess->setText(QStringLiteral("-"));
    m_metricPendingFailures->setText(QStringLiteral("-"));
    overviewLayout->addLayout(metrics);

    auto *overviewRow = new QGridLayout;
    overviewRow->setHorizontalSpacing(PageLayout::Space16);
    overviewRow->setVerticalSpacing(PageLayout::Space16);

    auto *summaryPanel = new QWidget(overviewPage);
    summaryPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    auto *summaryLayout = new QVBoxLayout(summaryPanel);
    summaryLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    summaryLayout->setSpacing(PageLayout::Space12);
    summaryLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("资源概览"), summaryPanel));
    auto *summaryTable = createTable(
        {QStringLiteral("模块"), QStringLiteral("当前状态")},
        {
            {QStringLiteral("项目配置"), QStringLiteral("读取本地 SQLite 配置")},
            {QStringLiteral("远程通道"), QStringLiteral("Linux OpenSSH / Windows winrs")},
            {QStringLiteral("凭据"), QStringLiteral("本地加密存储 + 会话缓存")},
            {QStringLiteral("部署日志"), QStringLiteral("config/logs 可追溯")}
        });
    summaryTable->setObjectName(QStringLiteral("dashboardTable"));
    summaryTable->setMinimumHeight(180);
    summaryLayout->addWidget(summaryTable, 1);

    auto *healthPanel = new QWidget(overviewPage);
    healthPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    auto *healthLayout = new QVBoxLayout(healthPanel);
    healthLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    healthLayout->setSpacing(PageLayout::Space12);
    healthLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("操作面板"), healthPanel));
    auto *miniGrid = new QGridLayout;
    miniGrid->setHorizontalSpacing(PageLayout::Space16);
    miniGrid->setVerticalSpacing(PageLayout::Space16);
    miniGrid->addWidget(makeDashboardMiniMetric(QStringLiteral("单任务锁"), QStringLiteral("ON")), 0, 0);
    miniGrid->addWidget(makeDashboardMiniMetric(QStringLiteral("日志索引"), QStringLiteral("LOCAL")), 0, 1);
    miniGrid->addWidget(makeDashboardMiniMetric(QStringLiteral("远程文件"), QStringLiteral("SFTP")), 1, 0);
    miniGrid->addWidget(makeDashboardMiniMetric(QStringLiteral("服务监控"), QStringLiteral("LIVE")), 1, 1);
    healthLayout->addLayout(miniGrid);
    healthLayout->addStretch();

    overviewRow->addWidget(summaryPanel, 0, 0);
    overviewRow->addWidget(healthPanel, 0, 1);
    overviewRow->setColumnStretch(0, 3);
    overviewRow->setColumnStretch(1, 2);
    overviewLayout->addLayout(overviewRow, 1);
    m_dashboardStack->addWidget(overviewPage);

    auto *deployTabPage = new QWidget;
    auto *deployTabLayout = new QVBoxLayout(deployTabPage);
    deployTabLayout->setContentsMargins(0, 0, 0, 0);
    deployTabLayout->setSpacing(PageLayout::Space12);
    auto *recentTable = createTable(
        {QStringLiteral("时间"), QStringLiteral("项目"), QStringLiteral("服务器"), QStringLiteral("状态"), QStringLiteral("耗时")},
        {});
    recentTable->setObjectName(QStringLiteral("dashboardTable"));
    m_recentTable = recentTable;
    auto *recentPanel = new QWidget(deployTabPage);
    recentPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    auto *recentLayout = new QVBoxLayout(recentPanel);
    recentLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    recentLayout->setSpacing(PageLayout::Space12);
    recentLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("最近部署"), recentPanel));
    recentLayout->addWidget(PageLayout::wrapTableSection(
        recentTable,
        &m_recentDeploymentsEmpty,
        QStringLiteral("暂无部署记录。完成一次部署后，最近动态会显示在这里。")), 1);
    deployTabLayout->addWidget(recentPanel, 1);
    m_dashboardStack->addWidget(deployTabPage);

    auto *serverTabPage = new QWidget;
    auto *serverTabLayout = new QVBoxLayout(serverTabPage);
    serverTabLayout->setContentsMargins(0, 0, 0, 0);
    serverTabLayout->setSpacing(PageLayout::Space12);
    m_dashboardServerTable = createTable(
        {QStringLiteral("名称"), QStringLiteral("主机"), QStringLiteral("平台"), QStringLiteral("用户名")},
        {});
    m_dashboardServerTable->setObjectName(QStringLiteral("dashboardTable"));
    auto *serverPanel = new QWidget(serverTabPage);
    serverPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    auto *serverPanelLayout = new QVBoxLayout(serverPanel);
    serverPanelLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    serverPanelLayout->setSpacing(PageLayout::Space12);
    serverPanelLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("已登记服务器"), serverPanel));
    serverPanelLayout->addWidget(PageLayout::wrapTableSection(
        m_dashboardServerTable,
        &m_dashboardServersEmpty,
        QStringLiteral("暂无服务器。请先在「服务器管理」中登记目标机。")), 1);
    serverTabLayout->addWidget(serverPanel, 1);
    m_dashboardStack->addWidget(serverTabPage);

    auto *logTabPage = new QWidget;
    auto *logTabLayout = new QVBoxLayout(logTabPage);
    logTabLayout->setContentsMargins(0, 0, 0, 0);
    logTabLayout->setSpacing(PageLayout::Space12);
    m_dashboardLogTable = createTable(
        {QStringLiteral("时间"), QStringLiteral("项目"), QStringLiteral("状态"), QStringLiteral("日志路径")},
        {});
    m_dashboardLogTable->setObjectName(QStringLiteral("dashboardTable"));
    connect(m_dashboardLogTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (m_dashboardLogTable == nullptr || row < 0) {
            return;
        }
        const QTableWidgetItem *logItem = m_dashboardLogTable->item(row, 3);
        if (logItem != nullptr) {
            openDeploymentLog(logItem->text());
        }
    });
    auto *logPanel = new QWidget(logTabPage);
    logPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    auto *logPanelLayout = new QVBoxLayout(logPanel);
    logPanelLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    logPanelLayout->setSpacing(PageLayout::Space12);
    logPanelLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("部署日志索引"), logPanel));
    logPanelLayout->addWidget(PageLayout::wrapTableSection(
        m_dashboardLogTable,
        &m_dashboardLogsEmpty,
        QStringLiteral("暂无部署日志。完成一次部署后，日志索引会显示在这里。")), 1);
    logTabLayout->addWidget(logPanel, 1);
    m_dashboardStack->addWidget(logTabPage);

    return page;
}

QWidget *MainWindow::createDeployPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);
    layout->setSpacing(PageLayout::Space16);

    layout->addWidget(PageLayout::makeHeaderBlock(
        QStringLiteral("一键部署"),
        QStringLiteral("选择已配置的项目与服务器，执行本地构建、上传与 restart。"),
        page));

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
    layout->addWidget(formBox);

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
    connect(m_refreshServiceStatusButton, &QPushButton::clicked, this, &MainWindow::refreshServiceStatus);
    auto *statusRow = new QWidget;
    auto *statusRowLayout = new QHBoxLayout(statusRow);
    statusRowLayout->setContentsMargins(0, 0, 0, 0);
    statusRowLayout->setSpacing(PageLayout::Space8);
    statusRowLayout->addWidget(m_serviceStatusLabel, 1);
    statusRowLayout->addWidget(m_refreshServiceStatusButton);
    statusForm->addRow(QStringLiteral("运行状态"), statusRow);
    layout->addWidget(statusBox);

    auto *logBox = new QGroupBox(QStringLiteral("应用日志"));
    logBox->setObjectName(QStringLiteral("deployConfigBox"));
    auto *logForm = new QFormLayout(logBox);
    PageLayout::applyForm(logForm);
    logForm->setContentsMargins(0, PageLayout::Space8, 0, 0);
    logForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    logForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_logPathInput = new QComboBox;
    m_logPathInput->setEditable(true);
    m_logPathInput->setMinimumWidth(360);
    PageLayout::configureFormInput(m_logPathInput);
    m_refreshLogListButton = new QPushButton(QStringLiteral("刷新列表"));
    m_viewLogButton = new QPushButton(QStringLiteral("一键查看"));
    m_viewLogButton->setObjectName(QStringLiteral("primaryButton"));
    connect(m_refreshLogListButton, &QPushButton::clicked, this, &MainWindow::refreshLocalLogFiles);
    connect(m_viewLogButton, &QPushButton::clicked, this, &MainWindow::viewDeploymentLog);
    auto *logFieldRow = new QWidget;
    auto *logRowLayout = new QHBoxLayout(logFieldRow);
    logRowLayout->setContentsMargins(0, 0, 0, 0);
    logRowLayout->setSpacing(PageLayout::Space8);
    logRowLayout->addWidget(m_logPathInput, 1);
    logRowLayout->addWidget(m_refreshLogListButton);
    logRowLayout->addWidget(m_viewLogButton);
    logForm->addRow(QStringLiteral("远程日志"), logFieldRow);
    layout->addWidget(logBox);

    connect(m_deployProject, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::refreshDeployLogPath);
    connect(m_deployProject, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::refreshServiceStatus);
    connect(m_deployServer, qOverload<int>(&QComboBox::currentIndexChanged), this, &MainWindow::refreshServiceStatus);

    refreshLocalLogFiles();
    refreshJdkProfiles();

    auto *execBox = new QFrame(page);
    execBox->setObjectName(QStringLiteral("deployExecBox"));
    auto *execLayout = new QVBoxLayout(execBox);
    execLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    execLayout->setSpacing(PageLayout::Space16);

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

    m_deployButton = new QPushButton(QStringLiteral("开始部署"));
    m_deployButton->setObjectName(QStringLiteral("primaryButton"));
    connect(m_deployButton, &QPushButton::clicked, this, &MainWindow::startDeployment);
    execLayout->addWidget(m_deployButton);

    layout->addWidget(execBox, 1);
    return page;
}

QWidget *MainWindow::createHistoryPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);

    layout->addWidget(PageLayout::makeHeaderBlock(
        QStringLiteral("历史记录"),
        QStringLiteral("查看每次部署的状态、版本与日志路径。"),
        page));

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
    auto *historyToolbar = new QHBoxLayout;
    historyToolbar->setContentsMargins(0, 0, 0, PageLayout::Space8);
    historyToolbar->addStretch();
    historyToolbar->addWidget(m_historyViewLogButton);

    layout->addLayout(historyToolbar);
    layout->addWidget(PageLayout::wrapTableSection(
        historyTable,
        &m_historyEmpty,
        QStringLiteral("暂无历史记录。执行部署后，记录会出现在此列表。")), 1);
    return page;
}

QWidget *MainWindow::createSettingsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    PageLayout::applyPage(layout);

    layout->addWidget(PageLayout::makeHeaderBlock(
        QStringLiteral("设置"),
        QStringLiteral("配置 Maven、本地仓库和数据目录。配置目录默认仍为程序目录下的 config。"),
        page));

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

QTableWidget *MainWindow::createTable(const QStringList &headers, const QList<QStringList> &rows)
{
    auto *table = new QTableWidget(rows.size(), headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->setVisible(false);
    PageLayout::configureDataTable(table);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setDefaultSectionSize(40);
    for (int row = 0; row < rows.size(); ++row) {
        const auto values = rows.at(row);
        for (int column = 0; column < headers.size(); ++column) {
            table->setItem(row, column, new QTableWidgetItem(values.value(column)));
        }
    }
    table->resizeColumnsToContents();
    return table;
}

void MainWindow::refreshDashboard()
{
    if (m_metricProjects) {
        m_metricProjects->setText(QString::number(m_projectManager->projectCount()));
    }
    if (m_metricServers) {
        m_metricServers->setText(QString::number(m_serverManager->serverCount()));
    }
    refreshDashboardTabData();
}

void MainWindow::applyDeploymentMetrics(const QVector<StoredRecord> &deployments)
{
    if (m_metricRecentSuccess != nullptr) {
        if (deployments.isEmpty()) {
            m_metricRecentSuccess->setText(QStringLiteral("-"));
        } else {
            const QString status = deployments.first().config.value(QStringLiteral("status")).toString();
            m_metricRecentSuccess->setText(formatDeploymentStatusLabel(status));
        }
    }

    if (m_metricPendingFailures != nullptr) {
        int pendingFailures = 0;
        for (const StoredRecord &record : deployments) {
            if (isPendingFailureStatus(record.config.value(QStringLiteral("status")).toString())) {
                ++pendingFailures;
            }
        }
        m_metricPendingFailures->setText(pendingFailures == 0 ? QStringLiteral("-") : QString::number(pendingFailures));
    }
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

    applyDeploymentMetrics(*deploymentSource);

    if (m_dashboardLogTable != nullptr) {
        const int logCount = qMin(20, deploymentSource->size());
        m_dashboardLogTable->setRowCount(logCount);
        for (int row = 0; row < logCount; ++row) {
            const QJsonObject &record = deploymentSource->at(row).config;
            m_dashboardLogTable->setItem(row, 0, new QTableWidgetItem(formatStartedAt(record)));
            m_dashboardLogTable->setItem(row, 1, new QTableWidgetItem(record.value(QStringLiteral("projectId")).toString()));
            m_dashboardLogTable->setItem(row, 2, new QTableWidgetItem(record.value(QStringLiteral("status")).toString()));
            m_dashboardLogTable->setItem(row, 3, new QTableWidgetItem(record.value(QStringLiteral("logPath")).toString()));
        }
        m_dashboardLogTable->resizeColumnsToContents();
        PageLayout::updateTableEmptyState(m_dashboardLogTable, m_dashboardLogsEmpty, logCount);
    }

    if (m_dashboardServerTable != nullptr) {
        QVector<StoredRecord> servers;
        QString error;
        if (!m_store->listServers(&servers, &error)) {
            statusBar()->showMessage(QStringLiteral("服务器数据加载失败：%1").arg(error));
            return;
        }

        m_dashboardServerTable->setRowCount(servers.size());
        for (int row = 0; row < servers.size(); ++row) {
            const QJsonObject &server = servers.at(row).config;
            m_dashboardServerTable->setItem(row, 0, new QTableWidgetItem(server.value(QStringLiteral("name")).toString()));
            m_dashboardServerTable->setItem(row, 1, new QTableWidgetItem(server.value(QStringLiteral("host")).toString()));
            m_dashboardServerTable->setItem(row, 2, new QTableWidgetItem(formatServerOsLabel(server)));
            m_dashboardServerTable->setItem(row, 3, new QTableWidgetItem(server.value(QStringLiteral("username")).toString()));
        }
        m_dashboardServerTable->resizeColumnsToContents();
        PageLayout::updateTableEmptyState(m_dashboardServerTable, m_dashboardServersEmpty, servers.size());
    }
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
    refreshLocalLogFiles();
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
    if (!AppSettingsStore(AppSettingsStore::defaultSettingsFile()).save(settings, &error)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), error);
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("已保存"),
                             QStringLiteral("Maven 设置已写入 %1，下一次部署自动生效；配置目录变更需要重启应用。")
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
        m_historyTable->resizeColumnsToContents();
        PageLayout::updateTableEmptyState(m_historyTable, m_historyEmpty, deployments.size());
    }

    if (m_recentTable != nullptr) {
        const int recentCount = qMin(5, deployments.size());
        m_recentTable->setRowCount(recentCount);
        for (int row = 0; row < recentCount; ++row) {
            const QJsonObject &record = deployments.at(row).config;
            m_recentTable->setItem(row, 0, new QTableWidgetItem(formatStartedAt(record)));
            m_recentTable->setItem(row, 1, new QTableWidgetItem(record.value(QStringLiteral("projectId")).toString()));
            m_recentTable->setItem(row, 2, new QTableWidgetItem(record.value(QStringLiteral("serverId")).toString()));
            m_recentTable->setItem(row, 3, new QTableWidgetItem(record.value(QStringLiteral("status")).toString()));
            m_recentTable->setItem(row, 4, new QTableWidgetItem(formatDuration(record)));
        }
        m_recentTable->resizeColumnsToContents();
        PageLayout::updateTableEmptyState(m_recentTable, m_recentDeploymentsEmpty, recentCount);
    }

    refreshDashboardTabData(&deployments);
}

void MainWindow::refreshLocalLogFiles()
{
    if (m_logPathInput == nullptr) {
        return;
    }

    const QString current = m_logPathInput->currentText().trimmed();
    m_logPathInput->clear();
    QStringList options;
    if (m_deployProject != nullptr && m_deployProject->count() > 0) {
        QJsonObject project;
        QString error;
        if (m_store->getProject(m_deployProject->currentData().toString(), &project, &error)) {
            options.append(deployLogPathOptionsForProject(project));
        }
    }
    options.removeDuplicates();
    m_logPathInput->addItems(options);
    if (!options.isEmpty()) {
        const int index = current.isEmpty() ? 0 : m_logPathInput->findText(current);
        m_logPathInput->setCurrentIndex(index >= 0 ? index : 0);
    } else if (!current.isEmpty()) {
        m_logPathInput->setEditText(current);
    }
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

    const QStringList options = deployLogPathOptionsForProject(project);
    if (options.isEmpty()) {
        return;
    }

    const QString logPath = options.first();
    const int index = m_logPathInput->findText(logPath);
    if (index >= 0) {
        m_logPathInput->setCurrentIndex(index);
    } else {
        m_logPathInput->setEditText(logPath);
    }
}

void MainWindow::refreshServiceStatus()
{
    if (m_serviceStatusLabel == nullptr || m_deployProject == nullptr || m_deployServer == nullptr) {
        return;
    }
    if (m_deployProject->count() == 0 || m_deployServer->count() == 0) {
        m_serviceStatusLabel->setText(QStringLiteral("请先配置项目与服务器"));
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
        RemoteCredentialResolver::resolve(server, m_credentials.get(), m_sessionCache.get(), this, true);
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
                                 QStringLiteral("请选择或输入日志文件，例如 logs/deploy-xxx.log 或 /home/app/logs/*.log"));
        return;
    }
    if (isRemoteDeployLogPath(trimmed)) {
        openRemoteDeploymentLog(trimmed);
        return;
    }
    if (!LocalLogCatalog::isValidRelativePath(trimmed)) {
        QMessageBox::warning(this,
                             QStringLiteral("路径无效"),
                             QStringLiteral("日志路径格式应为 logs/<filename>.log 或远程绝对路径。"));
        return;
    }

    QString loadError;
    if (!DeploymentLogDialog::canOpen(trimmed)) {
        DeploymentLogDialog::loadContent(trimmed, &loadError);
        QMessageBox::warning(this, QStringLiteral("无法打开日志"), loadError);
        return;
    }

    DeploymentLogDialog dialog(trimmed, this);
    dialog.exec();
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

    RemoteConnectionContext context =
        RemoteCredentialResolver::resolve(server, m_credentials.get(), m_sessionCache.get(), this);
    const QJsonObject auth = server.value(QStringLiteral("auth")).toObject();
    if (auth.value(QStringLiteral("mode")).toString() != QStringLiteral("ssh-key")
        && context.password.isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("无法打开远程日志"),
                             QStringLiteral("未获取到服务器密码，请先在服务器管理中保存密码。"));
        return;
    }

    auto browser = createRemoteLogFileBrowser(context, makeHostKeyPromptHandler(this));
    if (!browser) {
        QMessageBox::warning(this,
                             QStringLiteral("无法打开远程日志"),
                             QStringLiteral("远程日志查看当前仅支持 Linux SSH 或 Windows WinRM 服务器。"));
        return;
    }

    RemoteFileViewerDialog dialog(std::move(browser), QDir::fromNativeSeparators(remotePath.trimmed()), this);
    dialog.exec();
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

void MainWindow::startDeployment()
{
    if (m_deployRunning) {
        return;
    }

    if (m_deployProject->count() == 0 || m_deployServer->count() == 0) {
        QMessageBox::information(this,
                                 QStringLiteral("无法部署"),
                                 QStringLiteral("请先在「项目管理」和「服务器管理」中完成配置。"));
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
                             QStringLiteral("未获取到服务器密码，请先在服务器管理中保存密码。"));
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
