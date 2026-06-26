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
#include "qml/LineTabBarController.h"

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
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMouseEvent>
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

class DashboardHeroWidget final : public QWidget
{
public:
    explicit DashboardHeroWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("dashboardHero"));
    }
};

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

QWidget *makeHeroMetric(const QString &label, QLabel **valueLabel)
{
    auto *widget = new QFrame;
    widget->setObjectName(QStringLiteral("dashboardHeroMetric"));
    widget->setFixedHeight(30);
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(PageLayout::Space8, 0, PageLayout::Space8, 0);
    layout->setSpacing(PageLayout::Space4);
    *valueLabel = makeDashboardLabel(QStringLiteral("0"), QStringLiteral("dashboardHeroMetricValue"));
    auto *labelWidget = makeDashboardLabel(label, QStringLiteral("dashboardHeroMetricLabel"));
    layout->addWidget(*valueLabel);
    layout->addWidget(labelWidget);
    return widget;
}

QWidget *makeDashboardHero(QLabel **projectLabel,
                           QLabel **serverLabel,
                           QLabel **failureLabel,
                           QLabel **onlineRateLabel)
{
    Q_UNUSED(projectLabel);
    Q_UNUSED(serverLabel);
    Q_UNUSED(failureLabel);
    Q_UNUSED(onlineRateLabel);

    auto *hero = new DashboardHeroWidget;
    hero->setFixedHeight(96);
    auto *layout = new QHBoxLayout(hero);
    layout->setContentsMargins(PageLayout::Space16, PageLayout::Space12, PageLayout::Space16, PageLayout::Space12);
    layout->setSpacing(PageLayout::Space16);

    auto *textBlock = new QWidget(hero);
    auto *textLayout = new QVBoxLayout(textBlock);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(PageLayout::Space6);
    textLayout->addStretch();
    textLayout->addWidget(makeDashboardLabel(QStringLiteral("DEPLOY CONTROL"), QStringLiteral("dashboardKicker")));
    textLayout->addWidget(makeDashboardLabel(QStringLiteral("部署资源态势"), QStringLiteral("dashboardTitle")));
    textLayout->addWidget(makeDashboardLabel(QStringLiteral("项目、服务器与最近部署聚合在一屏，适合快速判断当前部署面。"), QStringLiteral("dashboardMeta")));
    textLayout->addStretch();

    layout->addWidget(textBlock, 1);
    return hero;
}

QFrame *makeDashboardStatCard(const QString &icon, const QString &title, const QString &subtitle, QLabel **valueLabel)
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("dashboardStatCard"));
    frame->setFixedHeight(88);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(PageLayout::Space12, PageLayout::Space10, PageLayout::Space12, PageLayout::Space10);
    layout->setSpacing(PageLayout::Space6);

    auto *topRow = new QHBoxLayout;
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(PageLayout::Space8);
    auto *iconLabel = makeDashboardLabel(icon, QStringLiteral("dashboardStatIconBox"));
    iconLabel->setAlignment(Qt::AlignCenter);
    QFont iconFont = iconLabel->font();
    iconFont.setPixelSize(14);
    iconFont.setBold(true);
    iconLabel->setFont(iconFont);
    auto *textBlock = new QWidget(frame);
    auto *textLayout = new QVBoxLayout(textBlock);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(0);
    auto *label = makeDashboardLabel(title, QStringLiteral("dashboardStatLabel"));
    *valueLabel = makeDashboardLabel(QStringLiteral("0"), QStringLiteral("dashboardStatValue"));
    textLayout->addWidget(label);
    textLayout->addWidget(*valueLabel);
    topRow->addWidget(iconLabel);
    topRow->addWidget(textBlock, 1);

    auto *meta = makeDashboardLabel(subtitle, QStringLiteral("dashboardMiniLabel"));
    meta->setWordWrap(true);

    layout->addLayout(topRow);
    layout->addWidget(meta);
    layout->addStretch();
    return frame;
}

QFrame *makeResourceStatusItem(const QString &icon, const QString &title, const QString &subtitle)
{
    auto *item = new QFrame;
    item->setObjectName(QStringLiteral("resourceStatusItem"));
    item->setFixedHeight(46);
    auto *layout = new QHBoxLayout(item);
    layout->setContentsMargins(PageLayout::Space10, PageLayout::Space8, PageLayout::Space10, PageLayout::Space8);
    layout->setSpacing(PageLayout::Space12);

    auto *iconLabel = makeDashboardLabel(icon, QStringLiteral("resourceStatusIcon"));
    auto *textBlock = new QWidget(item);
    auto *textLayout = new QVBoxLayout(textBlock);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(4);
    textLayout->addWidget(makeDashboardLabel(title, QStringLiteral("resourceStatusTitle")));
    auto *meta = makeDashboardLabel(subtitle, QStringLiteral("resourceStatusMeta"));
    meta->setWordWrap(true);
    textLayout->addWidget(meta);

    layout->addWidget(iconLabel);
    layout->addWidget(textBlock, 1);
    return item;
}

QFrame *makeDashboardQuickAction(const QString &icon, const QString &label, const QString &subtitle)
{
    auto *widget = new QFrame;
    widget->setObjectName(QStringLiteral("dashboardQuickAction"));
    widget->setFixedHeight(56);
    widget->setCursor(Qt::PointingHandCursor);
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(PageLayout::Space10, PageLayout::Space8, PageLayout::Space10, PageLayout::Space8);
    layout->setSpacing(PageLayout::Space8);

    auto *iconLabel = makeDashboardLabel(icon, QStringLiteral("dashboardQuickIconBox"));
    iconLabel->setAlignment(Qt::AlignCenter);
    QFont iconFont = iconLabel->font();
    iconFont.setPixelSize(14);
    iconFont.setBold(true);
    iconLabel->setFont(iconFont);
    auto *textBlock = new QWidget(widget);
    auto *textLayout = new QVBoxLayout(textBlock);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);
    textLayout->addWidget(makeDashboardLabel(label, QStringLiteral("dashboardQuickTitle")));
    auto *meta = makeDashboardLabel(subtitle, QStringLiteral("dashboardQuickMeta"));
    meta->setWordWrap(true);
    textLayout->addWidget(meta);

    auto *arrow = makeDashboardLabel(QStringLiteral("›"), QStringLiteral("dashboardQuickArrow"));
    arrow->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);
    layout->addWidget(textBlock, 1);
    layout->addWidget(arrow);
    return widget;
}

class DashboardQuickActionFilter final : public QObject
{
public:
    explicit DashboardQuickActionFilter(std::function<void()> handler, QObject *parent)
        : QObject(parent)
        , m_handler(std::move(handler))
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            const auto *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (m_handler) {
                    m_handler();
                }
                return true;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    std::function<void()> m_handler;
};

void wireDashboardQuickAction(QFrame *action, std::function<void()> handler)
{
    if (action == nullptr || !handler) {
        return;
    }
    action->installEventFilter(new DashboardQuickActionFilter(std::move(handler), action));
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
    showModule(moduleIndex);
    if (pageRow >= 0 && moduleIndex >= 0 && moduleIndex < m_modulePages.size()) {
        m_modulePages.at(moduleIndex)->setCurrentIndex(pageRow);
        m_navigation->blockSignals(true);
        m_navigation->setCurrentRow(pageRow);
        m_navigation->blockSignals(false);
    }

    if (dashboardTabIndex >= 0 && m_dashboardLineTab != nullptr && m_dashboardStack != nullptr) {
        m_dashboardLineTab->setCurrentIndex(dashboardTabIndex);
        m_dashboardStack->setCurrentIndex(dashboardTabIndex);
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
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(PageLayout::Space14, PageLayout::Space14, PageLayout::Space14, PageLayout::Space14);
    layout->setSpacing(PageLayout::Space12);

    const QStringList tabLabels = {
        QStringLiteral("总览"),
        QStringLiteral("部署"),
        QStringLiteral("服务器"),
        QStringLiteral("日志")
    };

    m_dashboardStack = new QStackedWidget(page);
    LineTabBarController *tabController = nullptr;
    layout->addWidget(PageLayout::makeLineTabBar(tabLabels, page, &tabController, m_dashboardStack));
    m_dashboardLineTab = tabController;
    layout->addWidget(m_dashboardStack, 1);

    auto *overviewPage = new QWidget;
    auto *overviewLayout = new QVBoxLayout(overviewPage);
    overviewLayout->setContentsMargins(0, 0, 0, 0);
    overviewLayout->setSpacing(PageLayout::Space12);

    overviewLayout->addWidget(makeDashboardHero(
        &m_heroProjects,
        &m_heroServers,
        &m_heroFailures,
        &m_heroOnlineRate));

    auto *metrics = new QGridLayout;
    metrics->setHorizontalSpacing(PageLayout::Space12);
    metrics->setVerticalSpacing(PageLayout::Space12);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("▦"), QStringLiteral("项目"), QStringLiteral("已登记部署单元"), &m_metricProjects), 0, 0);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("▣"), QStringLiteral("服务器"), QStringLiteral("Linux / Windows"), &m_metricServers), 0, 1);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("✓"), QStringLiteral("最近成功"), QStringLiteral("最近部署结果"), &m_metricRecentSuccess), 0, 2);
    metrics->addWidget(makeDashboardStatCard(QStringLiteral("!"), QStringLiteral("待处理失败"), QStringLiteral("需要关注"), &m_metricPendingFailures), 0, 3);
    m_metricRecentSuccess->setText(QStringLiteral("-"));
    m_metricPendingFailures->setText(QStringLiteral("-"));
    overviewLayout->addLayout(metrics);

    auto *overviewRow = new QGridLayout;
    overviewRow->setHorizontalSpacing(PageLayout::Space12);
    overviewRow->setVerticalSpacing(PageLayout::Space12);

    auto *summaryPanel = new QWidget(overviewPage);
    summaryPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    summaryPanel->setFixedHeight(258);
    auto *summaryLayout = new QVBoxLayout(summaryPanel);
    summaryLayout->setContentsMargins(PageLayout::Space14, PageLayout::Space14, PageLayout::Space14, PageLayout::Space14);
    summaryLayout->setSpacing(PageLayout::Space10);
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
    summaryTable->setFixedHeight(198);
    summaryTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    summaryTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    summaryLayout->addWidget(summaryTable, 1);

    auto *healthPanel = new QWidget(overviewPage);
    healthPanel->setObjectName(QStringLiteral("dashboardTablePanel"));
    healthPanel->setFixedHeight(258);
    auto *healthLayout = new QVBoxLayout(healthPanel);
    healthLayout->setContentsMargins(PageLayout::Space14, PageLayout::Space14, PageLayout::Space14, PageLayout::Space14);
    healthLayout->setSpacing(PageLayout::Space10);
    healthLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("快速操作"), healthPanel));
    auto *miniGrid = new QGridLayout;
    miniGrid->setHorizontalSpacing(PageLayout::Space12);
    miniGrid->setVerticalSpacing(PageLayout::Space12);
    auto *deployQuickAction = makeDashboardQuickAction(
        QStringLiteral("↻"),
        QStringLiteral("一键发布"),
        QStringLiteral("快速部署项目到服务器"));
    auto *uploadQuickAction = makeDashboardQuickAction(
        QStringLiteral("⇧"),
        QStringLiteral("上传文件"),
        QStringLiteral("上传本地文件到服务器"));
    auto *logsQuickAction = makeDashboardQuickAction(
        QStringLiteral("≡"),
        QStringLiteral("查看日志"),
        QStringLiteral("查看部署日志与运行记录"));
    auto *monitorQuickAction = makeDashboardQuickAction(
        QStringLiteral("◉"),
        QStringLiteral("实时监控"),
        QStringLiteral("跟踪服务器运行状态"));
    miniGrid->addWidget(deployQuickAction, 0, 0);
    miniGrid->addWidget(uploadQuickAction, 0, 1);
    miniGrid->addWidget(logsQuickAction, 1, 0);
    miniGrid->addWidget(monitorQuickAction, 1, 1);
    constexpr int kDeployModuleIndex = 1;
    wireDashboardQuickAction(deployQuickAction, [this]() { navigateToPage(kDeployModuleIndex, 3); });
    wireDashboardQuickAction(uploadQuickAction, [this]() { navigateToPage(kDeployModuleIndex, 2); });
    wireDashboardQuickAction(logsQuickAction, [this]() { navigateToPage(kDeployModuleIndex, 0, 3); });
    wireDashboardQuickAction(monitorQuickAction, [this]() { navigateToPage(kDeployModuleIndex, 2); });
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
    auto *recentHeader = new QHBoxLayout;
    recentHeader->setContentsMargins(0, 0, 0, 0);
    recentHeader->addWidget(PageLayout::makeSectionLabel(QStringLiteral("最近部署"), recentPanel));
    recentHeader->addStretch();
    auto *dashboardClearButton = new QPushButton(QStringLiteral("清空记录"));
    connect(dashboardClearButton, &QPushButton::clicked, this, &MainWindow::clearDeploymentHistory);
    recentHeader->addWidget(dashboardClearButton);
    recentLayout->addLayout(recentHeader);
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
        QStringLiteral("暂无服务器。请先在「部署工具 → 服务器管理」中登记目标机。")), 1);
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
    auto *logHeader = new QHBoxLayout;
    logHeader->setContentsMargins(0, 0, 0, 0);
    logHeader->addWidget(PageLayout::makeSectionLabel(QStringLiteral("部署日志索引"), logPanel));
    logHeader->addStretch();
    auto *dashboardLogClearButton = new QPushButton(QStringLiteral("清空记录"));
    connect(dashboardLogClearButton, &QPushButton::clicked, this, &MainWindow::clearDeploymentHistory);
    logHeader->addWidget(dashboardLogClearButton);
    logPanelLayout->addLayout(logHeader);
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
    const int projectCount = m_projectManager != nullptr ? m_projectManager->projectCount() : 0;
    const int serverCount = m_serverManager != nullptr ? m_serverManager->serverCount() : 0;

    if (m_metricProjects) {
        m_metricProjects->setText(QString::number(projectCount));
    }
    if (m_metricServers != nullptr) {
        m_metricServers->setText(QString::number(serverCount));
    }
    if (m_heroProjects != nullptr) {
        m_heroProjects->setText(QString::number(projectCount));
    }
    if (m_heroServers != nullptr) {
        m_heroServers->setText(QString::number(serverCount));
    }
    if (m_heroOnlineRate != nullptr) {
        m_heroOnlineRate->setText(QStringLiteral("100%"));
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
        const QString failureText = pendingFailures == 0 ? QStringLiteral("-") : QString::number(pendingFailures);
        m_metricPendingFailures->setText(failureText);
        if (m_heroFailures != nullptr) {
            m_heroFailures->setText(QString::number(pendingFailures));
        }
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
        PageLayout::refreshListingTableColumns(m_dashboardLogTable);
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
        PageLayout::refreshListingTableColumns(m_dashboardServerTable);
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
        PageLayout::refreshListingTableColumns(m_recentTable);
        PageLayout::updateTableEmptyState(m_recentTable, m_recentDeploymentsEmpty, recentCount);
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
