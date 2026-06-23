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
