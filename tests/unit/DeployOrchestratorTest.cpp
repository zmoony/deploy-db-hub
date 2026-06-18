#include "DeployOrchestratorTest.h"

#include "core/DeployOrchestrator.h"
#include "infra/ConfigStore.h"
#include "infra/LogPath.h"

#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTest>

void DeployOrchestratorTest::beginCreatesLogFileAndPendingRecord()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    DeployOrchestrator orchestrator(store, temp.path());
    DeployOrchestrator::ActiveDeployment deployment(FailureStrategy::Keep);
    QVERIFY(orchestrator.begin(QStringLiteral("demo"), QStringLiteral("srv-1"), FailureStrategy::Keep, &deployment, &error));

    QCOMPARE(deployment.record.value(QStringLiteral("status")).toString(), QStringLiteral("pending"));
    QCOMPARE(deployment.relativeLogPath, LogPath::relativePath(deployment.id));
    QVERIFY(QFile::exists(LogPath::absolutePath(temp.path(), deployment.relativeLogPath)));
}

void DeployOrchestratorTest::persistJobStateUpdatesBuildingStatus()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    DeployOrchestrator orchestrator(store, temp.path());
    DeployOrchestrator::ActiveDeployment deployment(FailureStrategy::Keep);
    QVERIFY(orchestrator.begin(QStringLiteral("demo"), QStringLiteral("srv-1"), FailureStrategy::Keep, &deployment, &error));
    QVERIFY(deployment.job.transitionTo(DeploymentStatus::Building));
    QVERIFY(orchestrator.persistJobState(&deployment, &error));
    QCOMPARE(deployment.record.value(QStringLiteral("status")).toString(), QStringLiteral("building"));
}

void DeployOrchestratorTest::completeSuccessPersistsTerminalRecord()
{
    QTemporaryDir temp;
    ConfigStore store(temp.filePath(QStringLiteral("test.db")));
    QString error;
    QVERIFY(store.open(&error));

    DeployOrchestrator orchestrator(store, temp.path());
    DeployOrchestrator::ActiveDeployment deployment(FailureStrategy::Keep);
    QVERIFY(orchestrator.begin(QStringLiteral("demo"), QStringLiteral("srv-1"), FailureStrategy::Keep, &deployment, &error));
    QVERIFY(deployment.job.transitionTo(DeploymentStatus::Building));
    QVERIFY(deployment.job.transitionTo(DeploymentStatus::Uploading));
    QVERIFY(deployment.job.transitionTo(DeploymentStatus::Restarting));
    QVERIFY(orchestrator.completeSuccess(&deployment,
                                         QStringLiteral("/opt/demo/releases/20260615-153000"),
                                         {QStringLiteral("app.jar")},
                                         &error));

    QCOMPARE(deployment.record.value(QStringLiteral("status")).toString(), QStringLiteral("success"));
    QVERIFY(deployment.record.value(QStringLiteral("finishedAt")).isString());
    QCOMPARE(deployment.record.value(QStringLiteral("artifactNames")).toArray().size(), 1);
}
