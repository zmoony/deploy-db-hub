#include "DeployJobTest.h"

#include "core/DeployJob.h"

#include <QTest>

void DeployJobTest::successfulLifecycleEndsInSuccess()
{
    DeployJob job(FailureStrategy::Keep);

    QVERIFY(job.transitionTo(DeploymentStatus::Building));
    QVERIFY(job.transitionTo(DeploymentStatus::Uploading));
    QVERIFY(job.transitionTo(DeploymentStatus::Restarting));
    QVERIFY(job.transitionTo(DeploymentStatus::Success));

    QCOMPARE(toString(job.status()), QStringLiteral("success"));
}

void DeployJobTest::restartFailureWithRollbackEndsRolledBack()
{
    DeployJob job(FailureStrategy::Rollback);

    QVERIFY(job.transitionTo(DeploymentStatus::Building));
    QVERIFY(job.transitionTo(DeploymentStatus::Uploading));
    QVERIFY(job.transitionTo(DeploymentStatus::Restarting));
    QVERIFY(job.fail(FailureStep::Restarting, QStringLiteral("restart exited with code 1"), true));

    QCOMPARE(toString(job.status()), QStringLiteral("rolled_back"));
    QCOMPARE(toString(job.failureStep()), QStringLiteral("restarting"));
    QCOMPARE(job.failureReason(), QStringLiteral("restart exited with code 1"));
}

void DeployJobTest::cancelFromUploadingEndsCanceled()
{
    DeployJob job(FailureStrategy::Rollback);

    QVERIFY(job.transitionTo(DeploymentStatus::Building));
    QVERIFY(job.transitionTo(DeploymentStatus::Uploading));
    QVERIFY(job.cancel());

    QCOMPARE(toString(job.status()), QStringLiteral("canceled"));
}
