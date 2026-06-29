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
                 .contains(QStringLiteral("rootLayout->setContentsMargins(0, 0, PageLayout::Space24, 0)")),
             "Main shell sidebar must start from the left edge without an outer gap");
    QVERIFY2(pageLayoutSource.contains(QStringLiteral("panelLayout->setContentsMargins(Space24, Space24, Space24, Space24)")),
             "Content panel must provide the required 24px page padding");
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
}
