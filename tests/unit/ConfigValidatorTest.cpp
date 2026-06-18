#include "ConfigValidatorTest.h"

#include "infra/ConfigValidator.h"
#include "infra/LogPath.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QTest>

namespace {
QJsonObject validProject()
{
    return {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), QStringLiteral("demo")},
        {QStringLiteral("name"), QStringLiteral("Demo")},
        {QStringLiteral("type"), QStringLiteral("java-maven")},
        {QStringLiteral("source"), QJsonObject{
            {QStringLiteral("kind"), QStringLiteral("local")},
            {QStringLiteral("localPath"), QStringLiteral("D:/projects/demo")}
        }},
        {QStringLiteral("build"), QJsonObject{
            {QStringLiteral("location"), QStringLiteral("local")},
            {QStringLiteral("workingDir"), QStringLiteral(".")},
            {QStringLiteral("command"), QStringLiteral("mvn clean package -DskipTests")},
            {QStringLiteral("artifactPath"), QStringLiteral("target/*.jar")},
            {QStringLiteral("artifactMatchPolicy"), QStringLiteral("fail-if-multiple")},
            {QStringLiteral("env"), QJsonObject{}},
            {QStringLiteral("timeoutSec"), 600}
        }},
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("serverId"), QStringLiteral("prod-linux-1")},
            {QStringLiteral("remoteBaseDir"), QStringLiteral("/opt/deploy-hub/apps/demo")},
            {QStringLiteral("restartScript"), QStringLiteral("restart.sh")},
            {QStringLiteral("failureStrategy"), QStringLiteral("rollback")}
        }}
    };
}

QJsonObject validLinuxServer()
{
    return {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), QStringLiteral("prod-linux-1")},
        {QStringLiteral("name"), QStringLiteral("Prod Linux 1")},
        {QStringLiteral("os"), QStringLiteral("linux")},
        {QStringLiteral("host"), QStringLiteral("192.168.1.10")},
        {QStringLiteral("port"), 22},
        {QStringLiteral("username"), QStringLiteral("deploy")},
        {QStringLiteral("auth"), QJsonObject{
            {QStringLiteral("mode"), QStringLiteral("system-keychain")},
            {QStringLiteral("credentialRef"), QStringLiteral("deploy-hub/prod-linux-1")},
            {QStringLiteral("sshPrivateKeyPath"), QJsonValue::Null}
        }},
        {QStringLiteral("defaultRemoteBaseDir"), QStringLiteral("/opt/deploy-hub")},
        {QStringLiteral("ssh"), QJsonObject{
            {QStringLiteral("knownHostsPolicy"), QStringLiteral("strict-with-prompt")}
        }},
        {QStringLiteral("winrm"), QJsonValue::Null}
    };
}

QJsonObject validWindowsServer()
{
    return {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), QStringLiteral("win-prod")},
        {QStringLiteral("name"), QStringLiteral("Windows Prod")},
        {QStringLiteral("os"), QStringLiteral("windows")},
        {QStringLiteral("host"), QStringLiteral("192.168.1.10")},
        {QStringLiteral("port"), 5986},
        {QStringLiteral("username"), QStringLiteral("deploy")},
        {QStringLiteral("auth"), QJsonObject{
            {QStringLiteral("mode"), QStringLiteral("manual")},
            {QStringLiteral("credentialRef"), QJsonValue::Null},
            {QStringLiteral("sshPrivateKeyPath"), QJsonValue::Null}
        }},
        {QStringLiteral("defaultRemoteBaseDir"), QStringLiteral("C:/deploy-hub")},
        {QStringLiteral("ssh"), QJsonValue::Null},
        {QStringLiteral("winrm"), QJsonObject{
            {QStringLiteral("scheme"), QStringLiteral("https")},
            {QStringLiteral("tlsVerify"), true},
            {QStringLiteral("trustedCertFingerprint"), QJsonValue::Null}
        }}
    };
}

QJsonObject validSuccessDeployment()
{
    return {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), QStringLiteral("deploy-20260615-001")},
        {QStringLiteral("projectId"), QStringLiteral("demo")},
        {QStringLiteral("serverId"), QStringLiteral("prod-linux-1")},
        {QStringLiteral("version"), QStringLiteral("20260615-153000")},
        {QStringLiteral("remoteVersionDir"), QStringLiteral("/opt/deploy-hub/apps/demo/releases/20260615-153000")},
        {QStringLiteral("artifactNames"), QJsonArray{QStringLiteral("app.jar")}},
        {QStringLiteral("status"), QStringLiteral("success")},
        {QStringLiteral("failureStep"), QJsonValue::Null},
        {QStringLiteral("failureReason"), QJsonValue::Null},
        {QStringLiteral("rolledBackFrom"), QJsonValue::Null},
        {QStringLiteral("startedAt"), QStringLiteral("2026-06-15T15:30:00+08:00")},
        {QStringLiteral("finishedAt"), QStringLiteral("2026-06-15T15:31:20+08:00")},
        {QStringLiteral("logPath"), LogPath::relativePath(QStringLiteral("deploy-20260615-001"))}
    };
}
}

void ConfigValidatorTest::acceptsValidLocalProject()
{
    const auto result = ConfigValidator::validateProject(validProject());

    QVERIFY(result.ok);
    QVERIFY(result.errors.isEmpty());
}

void ConfigValidatorTest::acceptsProjectWithCustomServiceCommands()
{
    auto project = validProject();
    QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    deploy.remove(QStringLiteral("restartScript"));
    deploy.insert(QStringLiteral("serviceMatch"), QStringLiteral("demo"));
    deploy.insert(QStringLiteral("startCommand"), QStringLiteral("nohup java -jar {targetJarPath} &"));
    deploy.insert(QStringLiteral("targetJarPath"), QStringLiteral("/home/demo/demo.jar"));
    deploy.insert(QStringLiteral("backupPolicy"), QStringLiteral("backup"));
    project.insert(QStringLiteral("deploy"), deploy);

    const auto result = ConfigValidator::validateProject(project);

    QVERIFY(result.ok);
    QVERIFY(result.errors.isEmpty());
}

void ConfigValidatorTest::rejectsGithubProjectWithoutRef()
{
    auto project = validProject();
    project[QStringLiteral("source")] = QJsonObject{
        {QStringLiteral("kind"), QStringLiteral("github")},
        {QStringLiteral("repoUrl"), QStringLiteral("https://github.com/org/repo.git")}
    };

    const auto result = ConfigValidator::validateProject(project);

    QVERIFY(!result.ok);
    QVERIFY(result.errors.contains(QStringLiteral("source.ref is required for github source")));
}

void ConfigValidatorTest::rejectsWindowsServerWithSshKeyAuth()
{
    auto server = validWindowsServer();
    server[QStringLiteral("auth")] = QJsonObject{
        {QStringLiteral("mode"), QStringLiteral("ssh-key")},
        {QStringLiteral("sshPrivateKeyPath"), QStringLiteral("C:/keys/id_rsa")}
    };

    const auto result = ConfigValidator::validateServer(server);

    QVERIFY(!result.ok);
    QVERIFY(result.errors.contains(QStringLiteral("auth.mode ssh-key is only supported for linux servers")));
}

void ConfigValidatorTest::rejectsServerWithoutDefaultRemoteBaseDir()
{
    auto server = validLinuxServer();
    server.remove(QStringLiteral("defaultRemoteBaseDir"));

    const auto result = ConfigValidator::validateServer(server);

    QVERIFY(!result.ok);
    QVERIFY(result.errors.contains(QStringLiteral("defaultRemoteBaseDir is required")));
}

void ConfigValidatorTest::rejectsInvalidWinrmFingerprint()
{
    auto server = validWindowsServer();
    QJsonObject winrm = server.value(QStringLiteral("winrm")).toObject();
    winrm[QStringLiteral("trustedCertFingerprint")] = QStringLiteral("abc");
    server[QStringLiteral("winrm")] = winrm;

    const auto result = ConfigValidator::validateServer(server);

    QVERIFY(!result.ok);
    QVERIFY(result.errors.contains(QStringLiteral("winrm.trustedCertFingerprint must be 40 hex digits")));
}

void ConfigValidatorTest::acceptsValidSuccessDeployment()
{
    const auto result = ConfigValidator::validateDeployment(validSuccessDeployment());

    QVERIFY(result.ok);
    QVERIFY(result.errors.isEmpty());
}

void ConfigValidatorTest::rejectsFailedDeploymentWithoutFailureStep()
{
    auto record = validSuccessDeployment();
    record[QStringLiteral("status")] = QStringLiteral("failed");
    record.remove(QStringLiteral("failureStep"));
    record[QStringLiteral("failureReason")] = QStringLiteral("upload timeout");
    record[QStringLiteral("finishedAt")] = QStringLiteral("2026-06-15T15:31:20+08:00");

    const auto result = ConfigValidator::validateDeployment(record);

    QVERIFY(!result.ok);
    QVERIFY(result.errors.contains(QStringLiteral("failureStep is required")));
}

void ConfigValidatorTest::rejectsDeploymentWithInvalidLogPath()
{
    auto record = validSuccessDeployment();
    record[QStringLiteral("logPath")] = QStringLiteral("/tmp/deploy.log");

    const auto result = ConfigValidator::validateDeployment(record);

    QVERIFY(!result.ok);
    QVERIFY(result.errors.contains(QStringLiteral("logPath must be relative, e.g. logs/<deploy-id>.log")));
}

void ConfigValidatorTest::acceptsPendingDeploymentWithoutFinishedAt()
{
    auto record = validSuccessDeployment();
    record[QStringLiteral("status")] = QStringLiteral("pending");
    record.remove(QStringLiteral("remoteVersionDir"));
    record.remove(QStringLiteral("artifactNames"));
    record.remove(QStringLiteral("finishedAt"));

    const auto result = ConfigValidator::validateDeployment(record);

    QVERIFY(result.ok);
}
