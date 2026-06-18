#include "core/DeploymentRecord.h"

#include "infra/LogPath.h"

#include <QJsonArray>
#include <QJsonValue>

QString DeploymentRecord::formatTimestamp(const QDateTime &dateTime)
{
    return dateTime.toString(Qt::ISODate);
}

QJsonObject DeploymentRecord::createPending(const QString &id,
                                            const QString &projectId,
                                            const QString &serverId,
                                            const QString &version,
                                            const QDateTime &startedAt)
{
    return {
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), id},
        {QStringLiteral("projectId"), projectId},
        {QStringLiteral("serverId"), serverId},
        {QStringLiteral("version"), version},
        {QStringLiteral("status"), toString(DeploymentStatus::Pending)},
        {QStringLiteral("failureStep"), QJsonValue::Null},
        {QStringLiteral("failureReason"), QJsonValue::Null},
        {QStringLiteral("rolledBackFrom"), QJsonValue::Null},
        {QStringLiteral("startedAt"), formatTimestamp(startedAt)},
        {QStringLiteral("finishedAt"), QJsonValue::Null},
        {QStringLiteral("logPath"), LogPath::relativePath(id)}
    };
}

bool DeploymentRecord::isTerminal(DeploymentStatus status)
{
    return status == DeploymentStatus::Success
        || status == DeploymentStatus::Failed
        || status == DeploymentStatus::RolledBack
        || status == DeploymentStatus::RollbackFailed
        || status == DeploymentStatus::Canceled;
}

QJsonObject DeploymentRecord::applyJobState(const QJsonObject &record, const DeployJob &job, const QDateTime &finishedAt)
{
    QJsonObject updated = record;
    updated[QStringLiteral("status")] = toString(job.status());

    if (job.failureStep() == FailureStep::None) {
        updated[QStringLiteral("failureStep")] = QJsonValue::Null;
    } else {
        updated[QStringLiteral("failureStep")] = toString(job.failureStep());
    }

    if (job.failureReason().isEmpty()) {
        updated[QStringLiteral("failureReason")] = QJsonValue::Null;
    } else {
        updated[QStringLiteral("failureReason")] = job.failureReason();
    }

    if (isTerminal(job.status())) {
        const QDateTime endTime = finishedAt.isValid() ? finishedAt : QDateTime::currentDateTime();
        updated[QStringLiteral("finishedAt")] = formatTimestamp(endTime);
    }

    return updated;
}

QJsonObject DeploymentRecord::withSuccessDetails(QJsonObject record,
                                                 const QString &remoteVersionDir,
                                                 const QStringList &artifactNames)
{
    record[QStringLiteral("remoteVersionDir")] = remoteVersionDir;
    QJsonArray names;
    for (const QString &name : artifactNames) {
        names.append(name);
    }
    record[QStringLiteral("artifactNames")] = names;
    return record;
}
