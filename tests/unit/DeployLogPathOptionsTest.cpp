#include "DeployLogPathOptionsTest.h"

#include "infra/RemoteLogPath.h"
#include "ui/DeployLogPathOptions.h"

#include <QJsonObject>
#include <QTest>

void DeployLogPathOptionsTest::includesProjectDeployLogDirGlob()
{
    const QJsonObject project{
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("logDir"), QStringLiteral("/home/app/logs/")}
        }}
    };

    const QStringList options = deployLogPathOptionsForProject(project);
    QCOMPARE(options.size(), 2);
    QVERIFY(options.contains(QStringLiteral("/home/app/logs/*.log")));
    QVERIFY(options.contains(QStringLiteral("/home/app/logs/*.txt")));
}

void DeployLogPathOptionsTest::includesRemoteBaseDirLogCandidates()
{
    const QJsonObject project{
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("remoteBaseDir"), QStringLiteral("/home/wva/wva-gateway")},
            {QStringLiteral("logDir"), QStringLiteral("/home/wva/wva-gateway/logs/wva-gateway.log")}
        }}
    };

    const QStringList options = deployLogPathOptionsForProject(project);
    QVERIFY(options.contains(QStringLiteral("/home/wva/wva-gateway/logs/*.log")));
    QVERIFY(options.contains(QStringLiteral("/home/wva/wva-gateway/log/*.log")));
    QVERIFY(options.contains(QStringLiteral("/home/wva/wva-gateway/logs/*.txt")));
    QVERIFY(!options.contains(QStringLiteral("/home/wva/wva-gateway/logs/wva-gateway.log/*.log")));
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
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/*.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/*.txt")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/app.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("D:/psmp/logs/*.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("D:\\psmp\\logs\\app.log")));
    QVERIFY(!isRemoteDeployLogPath(QStringLiteral("logs/deploy-001.log")));
}
