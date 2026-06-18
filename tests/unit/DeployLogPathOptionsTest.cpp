#include "DeployLogPathOptionsTest.h"

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

    QCOMPARE(deployLogPathOptionsForProject(project),
             QStringList{QStringLiteral("/home/app/logs/*.log")});
}

void DeployLogPathOptionsTest::detectsRemoteLogPaths()
{
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/*.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("/home/app/logs/app.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("D:/psmp/logs/*.log")));
    QVERIFY(isRemoteDeployLogPath(QStringLiteral("D:\\psmp\\logs\\app.log")));
    QVERIFY(!isRemoteDeployLogPath(QStringLiteral("logs/deploy-001.log")));
}
