#include "ProjectServiceConfigTest.h"

#include "infra/ProjectServiceConfig.h"

#include <QJsonObject>
#include <QTest>

namespace {

QJsonObject project()
{
    return {
        {QStringLiteral("id"), QStringLiteral("yz-wwa-gateway")},
        {QStringLiteral("name"), QStringLiteral("Gateway")},
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("remoteBaseDir"), QStringLiteral("/home/yz-wwa-gateway")},
            {QStringLiteral("logDir"), QStringLiteral("/home/yz-wwa-gateway/logs")}
        }}
    };
}

}

void ProjectServiceConfigTest::derivesServiceMatchFromProjectId()
{
    QCOMPARE(projectServiceConfig(project()).serviceMatch,
             QStringLiteral("yz-wwa-gateway"));
}

void ProjectServiceConfigTest::buildsDefaultTargetJarAndBackupPath()
{
    QCOMPARE(remoteProjectJarPath(project(), QStringLiteral("app.jar")),
             QStringLiteral("/home/yz-wwa-gateway/app.jar"));
    QCOMPARE(remoteProjectBackupPath(project(), QStringLiteral("app.jar"), QStringLiteral("20260618-153805")),
             QStringLiteral("/home/yz-wwa-gateway/bak/app-20260618-153805.bak.jar"));
}

void ProjectServiceConfigTest::rendersCommandPlaceholders()
{
    const QString command = renderProjectServiceCommand(
        QStringLiteral("nohup java -jar {targetJarPath} --name {projectId} > {logDir}/app.log 2>&1 &"),
        project(),
        QStringLiteral("/home/yz-wwa-gateway/app.jar"));

    QCOMPARE(command,
             QStringLiteral("nohup java -jar /home/yz-wwa-gateway/app.jar --name yz-wwa-gateway > /home/yz-wwa-gateway/logs/app.log 2>&1 &"));
}

void ProjectServiceConfigTest::pgrepSafePatternAvoidsSelfMatch()
{
    QCOMPARE(pgrepSafePattern(QStringLiteral("yz-wva-gateway")),
             QStringLiteral("[y]z-wva-gateway"));
}

void ProjectServiceConfigTest::buildsLinuxStatusCommandWithBracketPattern()
{
    const ProjectServiceConfig service = projectServiceConfig(project());
    const QString command = defaultLinuxServiceStatusCommand(service);
    QVERIFY(command.contains(QStringLiteral("[y]z-wwa-gateway")));
    QVERIFY(command.contains(QStringLiteral("kill -0")));
    QVERIFY(command.contains(QStringLiteral("echo STOPPED")));
}

void ProjectServiceConfigTest::buildsLinuxStatusCommandForTargetJar()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.insert(QStringLiteral("targetJarPath"), QStringLiteral("/home/yz-wwa-gateway/app.jar"));
    configured.insert(QStringLiteral("deploy"), deploy);

    const QString command = defaultLinuxServiceStatusCommand(projectServiceConfig(configured));
    QVERIFY(command.contains(QStringLiteral("[j]ava.*-jar.*[a]pp.jar")));
}

void ProjectServiceConfigTest::buildsWindowsStatusCommandExcludesShell()
{
    const ProjectServiceConfig service = projectServiceConfig(project());
    const QString command = defaultWindowsServiceStatusCommand(service);
    QVERIFY(command.contains(QStringLiteral("powershell*")));
    QVERIFY(command.contains(QStringLiteral("winrs*")));
}
