#include "DeploymentRecordTest.h"

#include "core/DeploymentRecord.h"

#include <QDateTime>
#include <QTest>

void DeploymentRecordTest::createPendingUsesRelativeLogPath()
{
    const QDateTime startedAt = QDateTime::fromString(QStringLiteral("2026-06-15T15:30:00+08:00"),
                                                      Qt::ISODate);
    const auto record = DeploymentRecord::createPending(QStringLiteral("deploy-20260615-153000"),
                                                        QStringLiteral("demo"),
                                                        QStringLiteral("srv-1"),
                                                        QStringLiteral("20260615-153000"),
                                                        startedAt);

    QCOMPARE(record.value(QStringLiteral("logPath")).toString(),
             QStringLiteral("logs/deploy-20260615-153000.log"));
    QCOMPARE(record.value(QStringLiteral("status")).toString(), QStringLiteral("pending"));
}

void DeploymentRecordTest::applyJobStateSetsFinishedAtOnFailure()
{
    const auto pending = DeploymentRecord::createPending(QStringLiteral("deploy-20260615-153000"),
                                                         QStringLiteral("demo"),
                                                         QStringLiteral("srv-1"),
                                                         QStringLiteral("20260615-153000"),
                                                         QDateTime::currentDateTime());
    DeployJob job(FailureStrategy::Keep);
    QVERIFY(job.transitionTo(DeploymentStatus::Building));
    QVERIFY(job.fail(FailureStep::Building, QStringLiteral("mvn not found"), false));

    const QDateTime finishedAt = QDateTime::fromString(QStringLiteral("2026-06-15T15:31:00+08:00"),
                                                       Qt::ISODate);
    const auto updated = DeploymentRecord::applyJobState(pending, job, finishedAt);

    QCOMPARE(updated.value(QStringLiteral("status")).toString(), QStringLiteral("failed"));
    QCOMPARE(updated.value(QStringLiteral("failureStep")).toString(), QStringLiteral("building"));
    QCOMPARE(updated.value(QStringLiteral("finishedAt")).toString(),
             DeploymentRecord::formatTimestamp(finishedAt));
}
