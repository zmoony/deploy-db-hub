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
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.remove(QStringLiteral("remoteBaseDir"));
    configured.insert(QStringLiteral("deploy"), deploy);

    const QString command = defaultLinuxServiceStatusCommand(configured);
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

    const QString command = defaultLinuxServiceStatusCommand(configured);
    QVERIFY(command.contains(QStringLiteral("[j]ava.*-jar.*[a]pp.jar")));
    QVERIFY(command.contains(QStringLiteral("[a]pp.jar")));
}

void ProjectServiceConfigTest::buildsLinuxStatusCommandForLatestJarInRemoteBaseDir()
{
    const QString command = defaultLinuxServiceStatusCommand(project());
    QVERIFY(command.contains(QStringLiteral("latest_jar=")));
    QVERIFY(command.contains(QStringLiteral("/home/yz-wwa-gateway")));
    QVERIFY(!command.contains(QStringLiteral("[y]z-wwa-gateway")));
}

void ProjectServiceConfigTest::buildsWindowsStatusCommandExcludesShell()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.remove(QStringLiteral("remoteBaseDir"));
    configured.insert(QStringLiteral("deploy"), deploy);

    const QString command = defaultWindowsServiceStatusCommand(configured);
    QVERIFY(command.contains(QStringLiteral("powershell*")));
    QVERIFY(command.contains(QStringLiteral("winrs*")));
}

void ProjectServiceConfigTest::detectsLocalRestartScriptPath()
{
    QVERIFY(isLocalRestartScriptPath(QStringLiteral("D:/project/scripts/restart-java.sh"),
                                    QStringLiteral("D:/project/app")));
    QVERIFY(isLocalRestartScriptPath(QStringLiteral("scripts/restart.sh"),
                                    QStringLiteral("D:/project/app")));
    QVERIFY(!isLocalRestartScriptPath(QStringLiteral("/home/app/restart.sh"),
                                      QStringLiteral("D:/project/app")));
}

void ProjectServiceConfigTest::buildsRestartPlanForLocalScript()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.insert(QStringLiteral("restartScript"), QStringLiteral("D:/tools/restart-java.sh"));
    configured.insert(QStringLiteral("deploy"), deploy);

    const RestartExecutionPlan plan = buildRestartExecutionPlan(configured, QStringLiteral("D:/project/app"));
    QVERIFY(plan.requiresScriptUpload);
    QCOMPARE(plan.localScriptPath, QStringLiteral("D:/tools/restart-java.sh"));
    QCOMPARE(plan.remoteCommand, QStringLiteral("bash '/home/yz-wwa-gateway/restart-java.sh'"));
}

void ProjectServiceConfigTest::prefersCustomServiceControlOverRestartScript()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.insert(QStringLiteral("restartMode"), QStringLiteral("service-command"));
    deploy.insert(QStringLiteral("restartScript"), QStringLiteral("restart.sh"));
    deploy.insert(QStringLiteral("startCommand"), QStringLiteral("nohup java -jar {targetJarPath} &"));
    configured.insert(QStringLiteral("deploy"), deploy);

    const RestartExecutionPlan plan = buildRestartExecutionPlan(configured, QStringLiteral("D:/project/app"));
    QVERIFY(!plan.requiresScriptUpload);
    QVERIFY(plan.remoteCommand.isEmpty());
    QVERIFY(usesCustomServiceControl(configured));
}

void ProjectServiceConfigTest::honorsRestartScriptModeExplicitly()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.insert(QStringLiteral("restartMode"), QStringLiteral("restart-script"));
    deploy.insert(QStringLiteral("restartScript"), QStringLiteral("restart.sh"));
    deploy.insert(QStringLiteral("startCommand"), QStringLiteral("nohup java -jar {targetJarPath} &"));
    configured.insert(QStringLiteral("deploy"), deploy);

    QVERIFY(!usesCustomServiceControl(configured));
    const RestartExecutionPlan plan = buildRestartExecutionPlan(configured, QStringLiteral("D:/project/app"));
    QVERIFY(plan.requiresScriptUpload);
    QVERIFY(!plan.remoteCommand.isEmpty());
}

void ProjectServiceConfigTest::wrapsLinuxCommandWithWorkingDirectory()
{
    const QString wrapped = wrapCommandWithWorkingDirectory(
        QStringLiteral("linux"),
        QStringLiteral("java -jar keyperson-yangzhong.jar"),
        QStringLiteral("/home/psmp/keyperson-yangzhong/keyperosn_yz"));
    QCOMPARE(wrapped,
             QStringLiteral("cd '/home/psmp/keyperson-yangzhong/keyperosn_yz' && java -jar keyperson-yangzhong.jar"));
}

void ProjectServiceConfigTest::wrapsWindowsCommandWithWorkingDirectory()
{
    const QString wrapped = wrapCommandWithWorkingDirectory(
        QStringLiteral("windows"),
        QStringLiteral("java -jar app.jar"),
        QStringLiteral("C:/Users/deploy/app"));
    QCOMPARE(wrapped,
             QStringLiteral("cd /d \"C:\\Users\\deploy\\app\" && java -jar app.jar"));
}

void ProjectServiceConfigTest::doesNotWrapWhenWorkingDirEmpty()
{
    const QString command = QStringLiteral("java -jar app.jar");
    QCOMPARE(wrapCommandWithWorkingDirectory(QStringLiteral("linux"), command, QString()), command);
    QCOMPARE(wrapCommandWithWorkingDirectory(QStringLiteral("linux"), command, QStringLiteral("   ")), command);
}

void ProjectServiceConfigTest::backupPathIsJoinedUnderRemoteBaseDir()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    deploy.insert(QStringLiteral("backupDir"), QStringLiteral("bak"));
    configured.insert(QStringLiteral("deploy"), deploy);

    QCOMPARE(remoteProjectBackupPath(configured, QStringLiteral("app.jar"), QStringLiteral("20260618-153805")),
             QStringLiteral("/home/yz-wwa-gateway/bak/app-20260618-153805.bak.jar"));
}

void ProjectServiceConfigTest::backupPathAcceptsAbsoluteLookingInputAsSubdir()
{
    QJsonObject configured = project();
    QJsonObject deploy = configured.value(QStringLiteral("deploy")).toObject();
    // "/bak" is the exact value the user enters; it must NOT resolve to the
    // filesystem root but to a subdirectory of remoteBaseDir.
    deploy.insert(QStringLiteral("backupDir"), QStringLiteral("/bak"));
    configured.insert(QStringLiteral("deploy"), deploy);

    QCOMPARE(remoteProjectBackupPath(configured, QStringLiteral("app.jar"), QStringLiteral("20260618-153805")),
             QStringLiteral("/home/yz-wwa-gateway/bak/app-20260618-153805.bak.jar"));
}
