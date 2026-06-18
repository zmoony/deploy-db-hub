#include "ConfigStoreTest.h"

#include "infra/ConfigStore.h"
#include "infra/LogPath.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QTemporaryDir>
#include <QThread>
#include <QTest>

namespace {
QJsonObject minimalProject()
{
    return {
        {QStringLiteral("id"), QStringLiteral("demo")},
        {QStringLiteral("name"), QStringLiteral("Demo")},
        {QStringLiteral("type"), QStringLiteral("java-maven")},
        {QStringLiteral("source"), QJsonObject{
            {QStringLiteral("kind"), QStringLiteral("local")},
            {QStringLiteral("localPath"), QStringLiteral("D:/projects/demo")}
        }},
        {QStringLiteral("build"), QJsonObject{
            {QStringLiteral("location"), QStringLiteral("local")},
            {QStringLiteral("command"), QStringLiteral("mvn package")},
            {QStringLiteral("artifactPath"), QStringLiteral("target/*.jar")}
        }},
        {QStringLiteral("deploy"), QJsonObject{
            {QStringLiteral("serverId"), QStringLiteral("srv-1")},
            {QStringLiteral("remoteBaseDir"), QStringLiteral("/opt/demo")},
            {QStringLiteral("restartScript"), QStringLiteral("restart.sh")},
            {QStringLiteral("failureStrategy"), QStringLiteral("keep")}
        }}
    };
}

QJsonObject minimalLinuxServer()
{
    return {
        {QStringLiteral("id"), QStringLiteral("srv-1")},
        {QStringLiteral("name"), QStringLiteral("Srv")},
        {QStringLiteral("os"), QStringLiteral("linux")},
        {QStringLiteral("host"), QStringLiteral("127.0.0.1")},
        {QStringLiteral("username"), QStringLiteral("deploy")},
        {QStringLiteral("auth"), QJsonObject{{QStringLiteral("mode"), QStringLiteral("manual")}}},
        {QStringLiteral("defaultRemoteBaseDir"), QStringLiteral("/opt/deploy-hub")},
        {QStringLiteral("ssh"), QJsonObject{{QStringLiteral("knownHostsPolicy"), QStringLiteral("strict-with-prompt")}}},
        {QStringLiteral("winrm"), QJsonValue::Null}
    };
}

QJsonObject pendingDeployment()
{
    return {
        {QStringLiteral("id"), QStringLiteral("deploy-20260615-153000")},
        {QStringLiteral("projectId"), QStringLiteral("demo")},
        {QStringLiteral("serverId"), QStringLiteral("srv-1")},
        {QStringLiteral("version"), QStringLiteral("20260615-153000")},
        {QStringLiteral("status"), QStringLiteral("pending")},
        {QStringLiteral("startedAt"), QStringLiteral("2026-06-15T15:30:00+08:00")},
        {QStringLiteral("logPath"), LogPath::relativePath(QStringLiteral("deploy-20260615-153000"))}
    };
}
}

void ConfigStoreTest::rejectsInvalidProject()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    auto project = minimalProject();
    project.remove(QStringLiteral("name"));

    QVERIFY(!store.upsertProject(QStringLiteral("demo"), project, &error));
    QVERIFY(error.contains(QStringLiteral("name is required")));
}

void ConfigStoreTest::rejectsInvalidServer()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    auto server = minimalLinuxServer();
    server.remove(QStringLiteral("defaultRemoteBaseDir"));

    QVERIFY(!store.upsertServer(QStringLiteral("srv-1"), server, &error));
    QVERIFY(error.contains(QStringLiteral("defaultRemoteBaseDir is required")));
}

void ConfigStoreTest::upsertsPendingDeployment()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));
    QVERIFY(store.upsertDeployment(pendingDeployment(), &error));
}

void ConfigStoreTest::rejectsDeploymentWithInvalidLogPath()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    auto record = pendingDeployment();
    record[QStringLiteral("logPath")] = QStringLiteral("/abs/deploy.log");

    QVERIFY(!store.upsertDeployment(record, &error));
    QVERIFY(error.contains(QStringLiteral("logPath")));
}

void ConfigStoreTest::listsDeploymentsInStartedAtOrder()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    auto older = pendingDeployment();
    older[QStringLiteral("id")] = QStringLiteral("deploy-older");
    older[QStringLiteral("startedAt")] = QStringLiteral("2026-06-15T10:00:00+08:00");
    older[QStringLiteral("logPath")] = LogPath::relativePath(QStringLiteral("deploy-older"));
    QVERIFY(store.upsertDeployment(older, &error));

    auto newer = pendingDeployment();
    newer[QStringLiteral("id")] = QStringLiteral("deploy-newer");
    newer[QStringLiteral("startedAt")] = QStringLiteral("2026-06-15T16:00:00+08:00");
    newer[QStringLiteral("logPath")] = LogPath::relativePath(QStringLiteral("deploy-newer"));
    QVERIFY(store.upsertDeployment(newer, &error));

    QVector<StoredRecord> records;
    QVERIFY(store.listDeployments(&records, &error));
    QCOMPARE(records.size(), 2);
    QCOMPARE(records.at(0).id, QStringLiteral("deploy-newer"));
    QCOMPARE(records.at(1).id, QStringLiteral("deploy-older"));
}

void ConfigStoreTest::getsLatestDeploymentForProject()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    auto older = pendingDeployment();
    older[QStringLiteral("id")] = QStringLiteral("deploy-older");
    older[QStringLiteral("startedAt")] = QStringLiteral("2026-06-15T10:00:00+08:00");
    older[QStringLiteral("logPath")] = LogPath::relativePath(QStringLiteral("deploy-older"));
    QVERIFY(store.upsertDeployment(older, &error));

    auto newer = pendingDeployment();
    newer[QStringLiteral("id")] = QStringLiteral("deploy-newer");
    newer[QStringLiteral("startedAt")] = QStringLiteral("2026-06-15T16:00:00+08:00");
    newer[QStringLiteral("logPath")] = LogPath::relativePath(QStringLiteral("deploy-newer"));
    QVERIFY(store.upsertDeployment(newer, &error));

    QJsonObject latest;
    QVERIFY(store.getLatestDeploymentForProject(QStringLiteral("demo"), &latest, &error));
    QCOMPARE(latest.value(QStringLiteral("id")).toString(), QStringLiteral("deploy-newer"));
}

void ConfigStoreTest::clearsAllDeployments()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));
    QVERIFY(store.upsertDeployment(pendingDeployment(), &error));

    auto second = pendingDeployment();
    second[QStringLiteral("id")] = QStringLiteral("deploy-second");
    second[QStringLiteral("logPath")] = LogPath::relativePath(QStringLiteral("deploy-second"));
    QVERIFY(store.upsertDeployment(second, &error));

    int removedCount = 0;
    QVERIFY(store.clearAllDeployments(&removedCount, &error));
    QCOMPARE(removedCount, 2);

    QVector<StoredRecord> records;
    QVERIFY(store.listDeployments(&records, &error));
    QCOMPARE(records.size(), 0);
}

void ConfigStoreTest::opensSameDatabaseFromWorkerThread()
{
    QTemporaryDir temp;
    const QString databasePath = temp.filePath(QStringLiteral("test.db"));

    ConfigStore mainStore(databasePath);
    QString error;
    QVERIFY(mainStore.open(&error));
    QVERIFY(mainStore.upsertProject(QStringLiteral("demo"), minimalProject(), &error));

    bool ok = false;
    QString workerError;
    QThread *worker = QThread::create([&]() {
        ConfigStore workerStore(databasePath);
        QJsonObject project;
        ok = workerStore.open(&workerError)
            && workerStore.getProject(QStringLiteral("demo"), &project, &workerError)
            && project.value(QStringLiteral("id")).toString() == QStringLiteral("demo");
    });

    worker->start();
    QVERIFY(worker->wait(10000));
    delete worker;
    QVERIFY2(ok, qPrintable(workerError));
}
