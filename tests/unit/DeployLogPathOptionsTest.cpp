#include "DeployLogPathOptionsTest.h"

#include "infra/RemoteLogPath.h"
#include "ui/DeployLogPathOptions.h"

#include <QJsonObject>
#include <QTest>

void DeployLogPathOptionsTest::returnsConfiguredLogPathAsDefault()
{
    const QJsonObject project{
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("logDir"), QStringLiteral("/home/app/logs/app.log")}
        }}
    };

    QCOMPARE(defaultRemoteLogPath(project), QStringLiteral("/home/app/logs/app.log"));
}

void DeployLogPathOptionsTest::normalizesConfiguredDirectoryLogDir()
{
    const QJsonObject project{
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("logDir"), QStringLiteral("/home/app/logs/")}
        }}
    };

    QCOMPARE(defaultRemoteLogPath(project), QStringLiteral("/home/app/logs"));
}

void DeployLogPathOptionsTest::buildsRemoteLogFileDiscoveryCommand()
{
    const QJsonObject server{{QStringLiteral("os"), QStringLiteral("linux")}};
    const QString command = remoteDiscoverLogFilesCommand(
        server, {QStringLiteral("/home/wva/wva-gateway/logs")});

    QVERIFY(command.contains(QStringLiteral("find '/home/wva/wva-gateway/logs'")));
    QVERIFY(command.contains(QStringLiteral("*.log")));
    QVERIFY(command.contains(QStringLiteral("*.txt")));
    QVERIFY(!command.contains(QStringLiteral("/*.log")));
}

void DeployLogPathOptionsTest::parsesRemoteLogGlobPath()
{
    const RemoteLogGlobSpec spec = parseRemoteLogGlobPath(QStringLiteral("/home/app/logs/*.txt"));
    QCOMPARE(spec.directory, QStringLiteral("/home/app/logs"));
    QCOMPARE(spec.filePattern, QStringLiteral("*.txt"));
    QVERIFY(spec.isGlob());
}

void DeployLogPathOptionsTest::detectsRemoteLogPaths()
{
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/app.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/*.txt")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("D:/psmp/logs/app.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("D:\\psmp\\logs\\app.log")));
    QVERIFY(!isRemoteDeployLogPath(QStringLiteral("logs/deploy-001.log")));
}
