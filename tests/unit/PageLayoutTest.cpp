#include "PageLayoutTest.h"

#include "ui/PageLayout.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QScreen>
#include <QStringList>
#include <QTextStream>
#include <QTest>
#include <QWidget>

namespace {

QString findRepositoryRoot()
{
    QDir dir(QDir::currentPath());
    while (!dir.isRoot()) {
        if (QFile::exists(dir.filePath(QStringLiteral("CMakeLists.txt")))
            && QFile::exists(dir.filePath(QStringLiteral("src/ui/PageLayout.cpp")))) {
            return dir.absolutePath();
        }
        dir.cdUp();
    }
    return QDir::currentPath();
}

QString readSourceFile(const QString &relativePath)
{
    QFile file(QDir(findRepositoryRoot()).filePath(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QTextStream stream(&file);
    return stream.readAll();
}

QString extractAiConfigPageSource(const QString &source)
{
    const int start = source.indexOf(QStringLiteral("AiConfigToolPage::AiConfigToolPage("));
    if (start < 0) {
        return QString();
    }
    const int end = source.indexOf(QStringLiteral("auto *testClient = new OpenAiChatClient(this)"), start);
    if (end < 0) {
        return QString();
    }
    return source.mid(start, end - start);
}

} // namespace

void PageLayoutTest::fitWindowToScreenCentersWindow()
{
    QWidget window;
    PageLayout::fitWindowToScreen(&window, PageLayout::MainWindowDefaultWidth, PageLayout::MainWindowDefaultHeight);

    QScreen *screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    const QRect available = screen->availableGeometry();
    QCOMPARE(window.height(), qMin(PageLayout::MainWindowDefaultHeight, static_cast<int>(available.height() * 0.88)));

    const QPoint expectedTopLeft(
        available.x() + (available.width() - window.width()) / 2,
        available.y() + (available.height() - window.height()) / 2);
    QCOMPARE(window.frameGeometry().topLeft(), expectedTopLeft);
}

void PageLayoutTest::pageTransferDoesNotShowDetachedPages()
{
    const QStringList sourceFiles = {
        QStringLiteral("src/ui/PageLayout.cpp"),
        QStringLiteral("src/ui/CommonToolsWidget.cpp"),
        QStringLiteral("src/ui/BigDataManagerWidget.cpp"),
        QStringLiteral("src/ui/DatabaseManagerWidget.cpp")
    };

    for (const QString &sourceFile : sourceFiles) {
        const QString source = readSourceFile(sourceFile);
        QVERIFY2(!source.isEmpty(), qPrintable(QStringLiteral("Unable to read %1").arg(sourceFile)));
        QVERIFY2(!source.contains(QStringLiteral("page->show();")),
                 qPrintable(QStringLiteral("%1 must not show transferred pages before the main window owns them")
                                .arg(sourceFile)));
    }
}

void PageLayoutTest::desktopShellTokensMatchReferenceLayout()
{
    QCOMPARE(PageLayout::SidebarNavWidth, 236);
    QCOMPARE(PageLayout::DialogFieldHeight, 40);
    QCOMPARE(PageLayout::Space24, 24);
}

void PageLayoutTest::sidebarNavigationRemainsModuleScoped()
{
    const QString source = readSourceFile(QStringLiteral("src/ui/MainWindow.cpp"));
    QVERIFY2(!source.isEmpty(), "Unable to read MainWindow.cpp");
    QVERIFY2(source.contains(QStringLiteral("m_moduleNavigationLabels.at(index)")),
             "Sidebar navigation must keep showing the current module menu");
    QVERIFY2(!source.contains(QStringLiteral("rebuildNavigation")),
             "Sidebar navigation must not be rebuilt as a global grouped tree");
}

void PageLayoutTest::sidebarNavigationUsesIcons()
{
    const QString mainWindowSource = readSourceFile(QStringLiteral("src/ui/MainWindow.cpp"));
    const QString pageLayoutSource = readSourceFile(QStringLiteral("src/ui/PageLayout.cpp"));
    QVERIFY2(!mainWindowSource.isEmpty(), "Unable to read MainWindow.cpp");
    QVERIFY2(!pageLayoutSource.isEmpty(), "Unable to read PageLayout.cpp");
    QVERIFY2(mainWindowSource.contains(QStringLiteral("makeSidebarLineIcon(label)")),
             "Sidebar menu items must be created with a line icon");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("setIconSize(QSize(18, 18))")),
             "Sidebar icons must stay at the reference 18px size");
}

void PageLayoutTest::aiConfigPageUsesUnifiedTemplate()
{
    const QString source = readSourceFile(QStringLiteral("src/ui/tools/pages/AiConfigToolPage.cpp"));
    const QString aiConfigSource = extractAiConfigPageSource(source);
    const QString pageLayoutSource = readSourceFile(QStringLiteral("src/ui/PageLayout.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!source.isEmpty(), "Unable to read AiConfigToolPage.cpp");
    QVERIFY2(!aiConfigSource.isEmpty(), "Unable to locate AiConfigToolPage constructor");
    QVERIFY2(!pageLayoutSource.isEmpty(), "Unable to read PageLayout.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("aiConfigSection")),
             "AI config page must use the shared section panel style");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("aiQuickHelpPanel")),
             "AI config page must include a Quick Help panel");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("查看文档")),
             "AI config page must expose a documentation hint");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("aiConfigSaveButton")),
             "AI config save action must use the compact save button style");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("makeAiConfigActionButton")),
             "AI config buttons must use the shared icon/text action button helper");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("aiConfigActions")),
             "AI config page must include dedicated action buttons");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("setProperty(\"cardStackPage\", true)")),
             "AI config page must use the card-stack shell instead of a single content box");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("formPanelLayout->addWidget(actions)")),
             "AI config action buttons must live inside the API configuration card");
    QVERIFY2(!aiConfigSource.contains(QStringLiteral("layout->addWidget(actions)")),
             "AI config action buttons must not sit outside the API configuration card");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("layout->addWidget(formPanel)"))
             && aiConfigSource.indexOf(QStringLiteral("layout->addWidget(formPanel)"))
                    < aiConfigSource.indexOf(QStringLiteral("layout->addWidget(helpPanel)")),
             "API configuration card must appear above the quick start card");
    QVERIFY2(!aiConfigSource.contains(QStringLiteral("formPanelLayout->addWidget(helpPanel)")),
             "Quick start must stay outside the API configuration card");
    QVERIFY2(!aiConfigSource.contains(QStringLiteral("contentArea")),
             "AI config page must use a flat page layout without nested content hosts");
    QVERIFY2(!aiConfigSource.contains(QStringLiteral("layout->addStretch()")),
             "AI config page must not stretch content away from the quick start card");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("wrapScrollableCardStack")),
             "Card-stack pages must bypass the single white content panel wrapper");
    QVERIFY2(styleSource.contains(QStringLiteral("QScrollArea#cardStackScroll")),
             "Card-stack scroll area must stay transparent on the workspace background");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("makeFormRow")),
             "AI config fields must use the reference label/input row layout");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("quickHelpBadge")),
             "AI config quick start must use step badges");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("quickHelpArrow")),
             "AI config quick start must show the ordered arrow relationship");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("quickHelpBulb")),
             "AI config quick start must include the final lightbulb hint icon");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("makeAiConfigActionButton(QStringLiteral(\"保存配置\")")),
             "AI config save button must include a line icon with text spacing");
    QVERIFY2(!aiConfigSource.contains(QStringLiteral("setProperty(\"fitFirstScreen\", true)")),
             "AI config page must scroll when content exceeds the viewport");
    QVERIFY2(readSourceFile(QStringLiteral("src/ui/MainWindow.cpp"))
                 .contains(QStringLiteral("rootLayout->setContentsMargins(0, 0, 5, 0)")),
             "Main shell sidebar-to-content gap must be 5px");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("panelLayout->setContentsMargins(Space20, Space20, Space20, Space20)")),
             "Content panel must provide the required 20px page padding");
    QVERIFY2(styleSource.contains(QStringLiteral("QWidget#moduleTabBar")),
             "Module tab bar style must be defined");
    QVERIFY2(styleSource.contains(QStringLiteral("QPushButton#moduleTabButton")),
             "Module tabs must use capsule button styling");
    QVERIFY2(styleSource.contains(QStringLiteral("QPushButton#aiConfigSaveButton")),
             "AI config save button style must be defined");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("PageLayout::applyLighterCardShadow(formPanel)")),
             "API configuration card must use the lighter elevation shadow");
    QVERIFY2(aiConfigSource.contains(QStringLiteral("PageLayout::applyLighterCardShadow(helpPanel)")),
             "Quick start card must use the lighter elevation shadow");
    QVERIFY2(styleSource.contains(QStringLiteral("QFrame#aiConfigSection"))
                 && styleSource.contains(QStringLiteral("QFrame#aiQuickHelpPanel"))
                 && styleSource.contains(QStringLiteral("border: none;")),
             "AI config cards must drop border lines in favor of shadow elevation");
    QVERIFY2(styleSource.contains(QStringLiteral("max-height: 32px;")),
             "AI config action buttons must stay at the compact 32px height");
    QVERIFY2(styleSource.contains(QStringLiteral("image: url(:/images/ui/check.svg)")),
             "Modern checkbox checked state must render a visible check mark");
    QVERIFY2(styleSource.contains(QStringLiteral("QLabel#quickHelpLink")) && styleSource.contains(QStringLiteral("#4E8EFA")),
             "Quick Help links must use the reference blue color");

    const QString mainWindowSource = readSourceFile(QStringLiteral("src/ui/MainWindow.cpp"));
    QVERIFY2(mainWindowSource.contains(QStringLiteral("createDashboardPage()"))
                 && mainWindowSource.contains(QStringLiteral("setProperty(\"cardStackPage\", true)")),
             "Deploy tool pages must use card-stack layout like AI chat");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("applyToolPage")),
             "PageLayout must provide compact tool page spacing");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("configureToolCard")),
             "PageLayout must provide compact tool card spacing");
}

void PageLayoutTest::aiChatPageMatchesPixsoLayout()
{
    const QString widgetSource = readSourceFile(QStringLiteral("src/ui/AiChatWidget.cpp"));
    const QString pageLayoutSource = readSourceFile(QStringLiteral("src/ui/PageLayout.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!widgetSource.isEmpty(), "Unable to read AiChatWidget.cpp");
    QVERIFY2(!pageLayoutSource.isEmpty(), "Unable to read PageLayout.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");
    QVERIFY2(widgetSource.contains(QStringLiteral("workspacePage")),
             "AI chat page must bypass the white content panel wrapper");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("wrapWorkspacePage")),
             "PageLayout must support workspace pages without contentPanel");
    QVERIFY2(styleSource.contains(QStringLiteral("QWidget#workspacePageShell")),
             "Workspace page shell must stay transparent on the gray background");
    QVERIFY2(widgetSource.contains(QStringLiteral("aiChatHeader")),
             "AI chat page must include the Pixso gradient header");
    QVERIFY2(widgetSource.contains(QStringLiteral("aiChatClearButton")),
             "AI chat page must expose the clear action in the header");
    QVERIFY2(widgetSource.contains(QStringLiteral("aiBotAvatar")),
             "AI chat messages must include bot avatars");
    QVERIFY2(widgetSource.contains(QStringLiteral("aiUserAvatar")),
             "AI chat messages must include user avatars");
    QVERIFY2(widgetSource.contains(QStringLiteral("showTypingIndicator")),
             "AI chat page must show a typing indicator while streaming");
    QVERIFY2(widgetSource.contains(QStringLiteral("aiInputContent")),
             "AI chat input must use the bordered content area");
    QVERIFY2(widgetSource.contains(QStringLiteral("setSelectedChip")),
             "AI chat quick chips must support selected state");
    QVERIFY2(styleSource.contains(QStringLiteral("QFrame#aiChatHeader")),
             "AI chat header style must be defined");
    QVERIFY2(styleSource.contains(QStringLiteral("QPushButton#aiActionChip[selected=\"true\"]")),
             "AI chat chip selected state must be styled");
}

void PageLayoutTest::logAiAnalysisShowsResultFirst()
{
    const QString widgetSource = readSourceFile(QStringLiteral("src/ui/LogAiAnalysisWidget.cpp"));
    const QString dialogSource = readSourceFile(QStringLiteral("src/ui/DeploymentLogDialog.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!widgetSource.isEmpty(), "Unable to read LogAiAnalysisWidget.cpp");
    QVERIFY2(!dialogSource.isEmpty(), "Unable to read DeploymentLogDialog.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");

    QVERIFY2(!widgetSource.contains(QStringLiteral("BubbleRole::User")),
             "Log AI analysis must not render the source log as a user question bubble");
    QVERIFY2(widgetSource.contains(QStringLiteral("logAiAnalysisToolbar")),
             "Log AI analysis controls must stay in a subdued toolbar");
    QVERIFY2(widgetSource.contains(QStringLiteral("title->setObjectName(QStringLiteral(\"sectionLabel\"))")),
             "Log AI analysis title must live in the same row as the actions");
    QVERIFY2(widgetSource.contains(QStringLiteral("toolbarLayout->addWidget(m_analyzeButton)"))
                 && widgetSource.contains(QStringLiteral("toolbarLayout->addWidget(m_stopButton)")),
             "Stop action must stay beside the analyze action");
    QVERIFY2(widgetSource.contains(QStringLiteral("logAiAnalysisResult")),
             "Log AI analysis must render into a dedicated full-width result area");
    QVERIFY2(widgetSource.contains(QStringLiteral("setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding)")),
             "Log AI analysis result must expand across the available row");
    QVERIFY2(widgetSource.contains(QStringLiteral("resultFrame->setMinimumHeight(220)")),
             "Log AI analysis result must remain usable in the default desktop dialog size");
    QVERIFY2(!dialogSource.contains(QStringLiteral("auto *aiTitle = new QLabel")),
             "Deployment log dialog must not add a separate AI title row above the action row");
    QVERIFY2(dialogSource.contains(QStringLiteral("resize(1040, 720)"))
                 && dialogSource.contains(QStringLiteral("setMinimumSize(900, 620)")),
             "Deployment log dialog must use a desktop default size that prevents card collapse");
    QVERIFY2(dialogSource.contains(QStringLiteral("splitter->setSizes({320, 320})")),
             "Deployment log dialog must allocate enough default height to AI analysis results");
    QVERIFY2(styleSource.contains(QStringLiteral("QFrame#logAiAnalysisToolbar")),
             "Log AI analysis toolbar style must be defined");
    QVERIFY2(styleSource.contains(QStringLiteral("QFrame#logAiAnalysisResult")),
             "Log AI analysis result style must be defined");
}

void PageLayoutTest::projectManagerSelectionIsProminent()
{
    const QString widgetSource = readSourceFile(QStringLiteral("src/ui/ProjectManagerWidget.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!widgetSource.isEmpty(), "Unable to read ProjectManagerWidget.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");

    QVERIFY2(widgetSource.contains(QStringLiteral("setObjectName(QStringLiteral(\"projectListNav\"))")),
             "Project manager must use a dedicated list object name for project-specific selection styling");
    QVERIFY2(styleSource.contains(QStringLiteral("QListWidget#projectListNav::item:selected")),
             "Project list must define a dedicated selected state");
    QVERIFY2(styleSource.contains(QStringLiteral("background-color: #EAF2FF"))
                 && styleSource.contains(QStringLiteral("border-left: 3px solid #2563EB"))
                 && styleSource.contains(QStringLiteral("font-weight: 600")),
             "Project list selected state must be visually stronger than the generic tool list state");
    QVERIFY2(styleSource.contains(QStringLiteral("QListWidget#projectListNav::item:selected:focus")),
             "Project list focus selection must stay prominent when the list has focus");
}

void PageLayoutTest::contentButtonsStayCompact()
{
    const QString projectManagerSource = readSourceFile(QStringLiteral("src/ui/ProjectManagerWidget.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!projectManagerSource.isEmpty(), "Unable to read ProjectManagerWidget.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");

    QVERIFY2(styleSource.contains(QStringLiteral("padding: 0 12px")),
             "Right-side content buttons must use compact horizontal padding");
    QVERIFY2(styleSource.contains(QStringLiteral("min-width: 96px")),
             "Default content buttons must have a smaller minimum width");
    QVERIFY2(styleSource.contains(QStringLiteral("min-width: 104px")),
             "Form action buttons must stay compact");
    QVERIFY2(projectManagerSource.contains(QStringLiteral("m_projectSearch->setMinimumWidth(300)")),
             "Project management search input must be wider than the original compact field");
}

void PageLayoutTest::serviceModuleButtonsKeepConsistentSize()
{
    const QString panelSource = readSourceFile(QStringLiteral("src/ui/ServiceProductPanel.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!panelSource.isEmpty(), "Unable to read ServiceProductPanel.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");

    QVERIFY2(panelSource.contains(QStringLiteral("constexpr int kServiceToolbarButtonWidth = 96")),
             "Big data and database toolbar buttons must share one fixed width");
    QVERIFY2(panelSource.contains(QStringLiteral("constexpr int kServiceToolbarButtonHeight = 32")),
             "Big data and database toolbar buttons must share one fixed height");
    QVERIFY2(panelSource.contains(QStringLiteral("configureServiceToolbarButton(addButton)"))
                 && panelSource.contains(QStringLiteral("configureServiceToolbarButton(editButton)"))
                 && panelSource.contains(QStringLiteral("configureServiceToolbarButton(deleteButton)"))
                 && panelSource.contains(QStringLiteral("configureServiceToolbarButton(refreshButton)")),
             "Service list toolbar buttons must use the shared size helper");
    QVERIFY2(panelSource.contains(QStringLiteral("configureServiceToolbarButton(button)")),
             "Service detail toolbar action buttons must use the shared size helper");
    QVERIFY2(panelSource.contains(QStringLiteral("setProperty(\"serviceToolbar\", true)")),
             "Service toolbar buttons must set the serviceToolbar property for QSS height override");
    QVERIFY2(panelSource.contains(QStringLiteral("unpolish(button)")),
             "Service toolbar buttons must force style re-polish after setting properties");
    QVERIFY2(styleSource.contains(QStringLiteral("QPushButton[serviceToolbar=\"true\"]")),
             "Service toolbar button QSS must override the base 40px min-height with 32px");
    QVERIFY2(styleSource.contains(QStringLiteral("min-height: 32px;"))
                 && styleSource.contains(QStringLiteral("min-width: 96px;")),
             "Service toolbar QSS must lock both width and height");
}

void PageLayoutTest::projectServiceControlCopyExplainsRemoteCommands()
{
    const QString dialogSource = readSourceFile(QStringLiteral("src/ui/ProjectDialog.cpp"));
    const QString serverDialogSource = readSourceFile(QStringLiteral("src/ui/ServerDialog.cpp"));
    const QString sshSource = readSourceFile(QStringLiteral("src/adapters/ssh/SshRemoteMonitor.cpp"));
    const QString winRmSource = readSourceFile(QStringLiteral("src/adapters/winrm/WinRmRemoteMonitor.cpp"));
    QVERIFY2(!dialogSource.isEmpty(), "Unable to read ProjectDialog.cpp");
    QVERIFY2(!serverDialogSource.isEmpty(), "Unable to read ServerDialog.cpp");
    QVERIFY2(!sshSource.isEmpty(), "Unable to read SshRemoteMonitor.cpp");
    QVERIFY2(!winRmSource.isEmpty(), "Unable to read WinRmRemoteMonitor.cpp");

    QVERIFY2(dialogSource.contains(QStringLiteral("部署后重启方式")),
             "Project dialog must distinguish deploy-time restart from project manager service control");
    QVERIFY2(dialogSource.contains(QStringLiteral("本地脚本（部署时上传执行）")),
             "Project dialog must explain restart scripts are local deploy-time scripts");
    QVERIFY2(dialogSource.contains(QStringLiteral("远程服务命令（项目管理可用）")),
             "Project dialog must expose remote commands as the mode used by project management");
    QVERIFY2(dialogSource.contains(QStringLiteral("本地脚本路径")),
             "Project dialog script field must not imply the path is remote");
    QVERIFY2(dialogSource.contains(QStringLiteral("makeFormCard(this, QStringLiteral(\"基本信息\")"))
                 && dialogSource.contains(QStringLiteral("PageLayout::makeDeploySectionCard(this, QStringLiteral(\"部署后重启方式\")")),
             "Project dialog cards must use real deploy-style headers instead of QGroupBox titles");
    QVERIFY2(!dialogSource.contains(QStringLiteral("new QGroupBox")),
             "Project dialog must not use QGroupBox titles because they do not fill the full card width");
    QVERIFY2(!serverDialogSource.contains(QStringLiteral("new QGroupBox")),
             "Server dialog must not use QGroupBox titles because they do not fill the full card width");
    QVERIFY2(sshSource.contains(QStringLiteral("请编辑项目并填写「远程服务命令」里的启动命令"))
                 && winRmSource.contains(QStringLiteral("请编辑项目并填写「远程服务命令」里的启动命令")),
             "Start service failures must point users to the remote start command field");
    QVERIFY2(sshSource.contains(QStringLiteral("wrapCommandWithWorkingDirectory(QStringLiteral(\"linux\"), commandText, remoteBaseDir)"))
                 && winRmSource.contains(QStringLiteral("wrapCommandWithWorkingDirectory(QStringLiteral(\"windows\"), commandText, remoteBaseDir)")),
             "Project manager service commands must run from the configured remote deploy directory");
}

void PageLayoutTest::legacyGroupBoxCardsUseDeploySectionTitleStyle()
{
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    const QString mainWindowSource = readSourceFile(QStringLiteral("src/ui/MainWindow.cpp"));
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");
    QVERIFY2(!mainWindowSource.isEmpty(), "Unable to read MainWindow.cpp");

    QVERIFY2(!styleSource.contains(QStringLiteral("QGroupBox#dialogFormBox::title {\n            subcontrol-origin: margin;\n            left: 10px;\n            top: -8px")),
             "Dialog form box titles must not float on the border with a negative top offset");
    QVERIFY2(mainWindowSource.contains(QStringLiteral("makeDeploySectionCard(page, QStringLiteral(\"全局设置\")"))
                 && mainWindowSource.contains(QStringLiteral("makeDeploySectionCard(page, QStringLiteral(\"数据库驱动 (JDBC)\")"))
                 && mainWindowSource.contains(QStringLiteral("makeDeploySectionCard(page, QStringLiteral(\"AI 辅助\")")),
             "Deploy, database, and AI settings cards must use real deploy-style headers");
    QVERIFY2(!mainWindowSource.contains(QStringLiteral("setObjectName(QStringLiteral(\"deployConfigBox\"))")),
             "Settings pages must not use QGroupBox title styling for card headers");
}

void PageLayoutTest::deployPageMatchesStitchLayout()
{
    const QString mainWindowSource = readSourceFile(QStringLiteral("src/ui/MainWindow.cpp"));
    const QString pageLayoutSource = readSourceFile(QStringLiteral("src/ui/PageLayout.cpp"));
    const QString styleSource = readSourceFile(QStringLiteral("src/ui/style.qss"));
    QVERIFY2(!mainWindowSource.isEmpty(), "Unable to read MainWindow.cpp");
    QVERIFY2(!pageLayoutSource.isEmpty(), "Unable to read PageLayout.cpp");
    QVERIFY2(!styleSource.isEmpty(), "Unable to read style.qss");

    const int deployStart = mainWindowSource.indexOf(QStringLiteral("QWidget *MainWindow::createDeployPage()"));
    QVERIFY2(deployStart >= 0, "Deploy page factory must exist");
    const int deployEnd = mainWindowSource.indexOf(QStringLiteral("QWidget *MainWindow::createHistoryPage()"), deployStart);
    QVERIFY2(deployEnd > deployStart, "Deploy page factory must be bounded");
    const QString deploySource = mainWindowSource.mid(deployStart, deployEnd - deployStart);

    QVERIFY2(deploySource.contains(QStringLiteral("makeDeploySectionCard")),
             "Deploy page must use shared section cards");
    QVERIFY2(deploySource.contains(QStringLiteral("makeDeploySectionCard(configColumn, QStringLiteral(\"远程日志\")")),
             "Deploy log card must use the shared section header style");
    QVERIFY2(deploySource.contains(QStringLiteral("deployLogPathRow"))
                 && deploySource.contains(QStringLiteral("logPathRowLayout->addWidget(m_logPathInput, 1)")),
             "Deploy log path input must occupy its own full-width row");
    QVERIFY2(deploySource.contains(QStringLiteral("deployLogActionsRow"))
                 && deploySource.contains(QStringLiteral("logActionsLayout->addWidget(m_refreshLogListButton)"))
                 && deploySource.contains(QStringLiteral("logActionsLayout->addWidget(m_viewLogButton)")),
             "Deploy log action buttons must be kept in a separate row");
    QVERIFY2(deploySource.contains(QStringLiteral("makeDeployFieldLabel")),
             "Deploy page fields must use vertical field labels");
    QVERIFY2(deploySource.contains(QStringLiteral("deployStatusPanel")),
             "Deploy page must include the health status panel");
    QVERIFY2(deploySource.contains(QStringLiteral("deployStartButton")),
             "Deploy page must use the dedicated start button style");
    QVERIFY2(deploySource.contains(QStringLiteral("deployTaskValue")),
             "Deploy page must show the current task label");
    QVERIFY2(deploySource.contains(QStringLiteral("deployConsoleFooter")),
             "Deploy console must include a footer status bar");
    QVERIFY2(deploySource.contains(QStringLiteral("body->addWidget(configColumn, 5)"))
                 && deploySource.contains(QStringLiteral("body->addWidget(execCard, 7)")),
             "Deploy page must use a 5:7 two-column layout");
    QVERIFY2(deploySource.contains(QStringLiteral("setProperty(\"workspacePage\", true)")),
             "Deploy page must fill the workspace without scroll wrappers");

    QVERIFY2(pageLayoutSource.contains(QStringLiteral("makeDeploySectionCard")),
             "PageLayout must provide deploy section cards");
    QVERIFY2(styleSource.contains(QStringLiteral("QFrame#deploySectionCard")),
             "Deploy section card style must be defined");
    QVERIFY2(styleSource.contains(QStringLiteral("QPlainTextEdit#deployLog")),
             "Deploy console style must be defined");
}
