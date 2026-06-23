#include "MainWindowStartupTest.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QTest>

namespace {

QString readMainWindowSource()
{
    QDir dir(QDir::current());
    QString path;
    for (int i = 0; i < 4; ++i) {
        const QString candidate = dir.filePath(QStringLiteral("src/ui/MainWindow.cpp"));
        if (QFile::exists(candidate)) {
            path = candidate;
            break;
        }
        dir.cdUp();
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

}

void MainWindowStartupTest::startupRefreshesDoNotPromptForRemoteAccess()
{
    const QString source = readMainWindowSource();
    QVERIFY2(!source.isEmpty(), "failed to read MainWindow.cpp");

    QVERIFY2(source.contains(QStringLiteral("refreshLocalLogFiles(false);")),
             "startup and selector refreshes must not prompt for remote log credentials");
    QVERIFY2(source.contains(QStringLiteral("refreshServiceStatus(false);")),
             "startup and selector refreshes must not prompt for service-status credentials");
    QVERIFY2(source.contains(QStringLiteral("refreshRemoteLogPathOptions(allowRemotePrompt);")),
             "remote log refresh must pass through the prompt policy");
    QVERIFY2(source.contains(QStringLiteral("RemoteCredentialResolver::resolve(server, m_credentials.get(), m_sessionCache.get(), this, allowPrompt)")),
             "service status refresh must pass through the prompt policy");
    QVERIFY2(source.contains(QRegularExpression(QStringLiteral("if \\(!allowPrompt\\) \\{\\s*return;\\s*\\}"))),
             "silent remote log refresh must not start remote discovery that can show host-key prompts");
    const int constructorStart = source.indexOf(QStringLiteral("MainWindow::MainWindow"));
    const int constructorEnd = source.indexOf(QStringLiteral("MainWindow::~MainWindow"), constructorStart);
    QVERIFY2(constructorStart >= 0 && constructorEnd > constructorStart, "failed to locate MainWindow constructor");
    const QString constructorSource = source.mid(constructorStart, constructorEnd - constructorStart);
    QVERIFY2(!constructorSource.contains(QStringLiteral("refreshServiceStatus(")),
             "startup must not trigger service-status remote checks during MainWindow construction");
    QVERIFY2(source.contains(QStringLiteral("if (!allowPrompt) {")),
             "silent service-status refresh must not connect to remote servers");
    QVERIFY2(source.contains(QStringLiteral("m_serviceStatusLabel->setText(QStringLiteral(\"未检测\"));")),
             "silent service-status refresh must show a local placeholder state");
}
