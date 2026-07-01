#include "ProjectManagerWidgetTest.h"

#include "ui/ProjectManagerWidget.h"

#include <QJsonObject>
#include <QTest>

void ProjectManagerWidgetTest::quickAddDraftCopiesProjectDetailsWithNewIdentity()
{
    const QJsonObject original{
        {QStringLiteral("id"), QStringLiteral("gateway")},
        {QStringLiteral("name"), QStringLiteral("Gateway")},
        {QStringLiteral("type"), QStringLiteral("java-maven")},
        {QStringLiteral("source"), QJsonObject{
            {QStringLiteral("kind"), QStringLiteral("github")},
            {QStringLiteral("repoUrl"), QStringLiteral("https://github.com/acme/gateway.git")},
            {QStringLiteral("ref"), QStringLiteral("main")},
            {QStringLiteral("localPath"), QJsonValue::Null}
        }},
        {QStringLiteral("build"), QJsonObject{
            {QStringLiteral("location"), QStringLiteral("local")},
            {QStringLiteral("mode"), QStringLiteral("build")},
            {QStringLiteral("workingDir"), QStringLiteral(".")},
            {QStringLiteral("command"), QStringLiteral("mvn clean package -DskipTests")},
            {QStringLiteral("artifactPath"), QStringLiteral("target/*.jar")},
            {QStringLiteral("artifactMatchPolicy"), QStringLiteral("fail-if-multiple")},
            {QStringLiteral("env"), QJsonObject{}},
            {QStringLiteral("timeoutSec"), 600}
        }},
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("serverId"), QStringLiteral("srv-1")},
            {QStringLiteral("remoteBaseDir"), QStringLiteral("/home/gateway")},
            {QStringLiteral("logDir"), QStringLiteral("/home/gateway/logs")},
            {QStringLiteral("restartScript"), QStringLiteral("restart.sh")},
            {QStringLiteral("serviceMatch"), QStringLiteral("gateway")},
            {QStringLiteral("targetJarPath"), QStringLiteral("/home/gateway/app.jar")},
            {QStringLiteral("startCommand"), QStringLiteral("nohup java -jar {targetJarPath} > {logDir}/app.log 2>&1 &")},
            {QStringLiteral("backupPolicy"), QStringLiteral("backup")},
            {QStringLiteral("failureStrategy"), QStringLiteral("rollback")}
        }}
    };

    const QJsonObject draft = ProjectManagerWidget::makeQuickAddDraft(original);

    QCOMPARE(draft.value(QStringLiteral("id")).toString(), QStringLiteral("gateway-copy"));
    QCOMPARE(draft.value(QStringLiteral("name")).toString(), QStringLiteral("Gateway Copy"));
    QCOMPARE(draft.value(QStringLiteral("type")).toString(), original.value(QStringLiteral("type")).toString());
    QCOMPARE(draft.value(QStringLiteral("source")).toObject(), original.value(QStringLiteral("source")).toObject());
    QCOMPARE(draft.value(QStringLiteral("build")).toObject(), original.value(QStringLiteral("build")).toObject());
    QCOMPARE(draft.value(QStringLiteral("deploy")).toObject(), original.value(QStringLiteral("deploy")).toObject());
}

void ProjectManagerWidgetTest::projectSearchMatchesNameAndServer()
{
    const QJsonObject project{
        {QStringLiteral("id"), QStringLiteral("gateway")},
        {QStringLiteral("name"), QStringLiteral("Gateway API")},
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("serverId"), QStringLiteral("prod-linux-1")}
        }}
    };
    const QJsonObject server{
        {QStringLiteral("id"), QStringLiteral("prod-linux-1")},
        {QStringLiteral("name"), QStringLiteral("Production Linux")},
        {QStringLiteral("host"), QStringLiteral("172.17.112.121")},
        {QStringLiteral("username"), QStringLiteral("deploy")}
    };

    QVERIFY(ProjectManagerWidget::projectMatchesSearch(project, server, QStringLiteral("gateway")));
    QVERIFY(ProjectManagerWidget::projectMatchesSearch(project, server, QStringLiteral("prod-linux")));
    QVERIFY(ProjectManagerWidget::projectMatchesSearch(project, server, QStringLiteral("Production")));
    QVERIFY(ProjectManagerWidget::projectMatchesSearch(project, server, QStringLiteral("172.17")));
    QVERIFY(ProjectManagerWidget::projectMatchesSearch(project, server, QStringLiteral("")));
    QVERIFY(!ProjectManagerWidget::projectMatchesSearch(project, server, QStringLiteral("staging")));
}
