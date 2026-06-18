#pragma once

#include "core/DeployJob.h"

#include <QDateTime>
#include <QJsonObject>
#include <QStringList>

namespace DeploymentRecord {

QString formatTimestamp(const QDateTime &dateTime);

QJsonObject createPending(const QString &id,
                          const QString &projectId,
                          const QString &serverId,
                          const QString &version,
                          const QDateTime &startedAt);

QJsonObject applyJobState(const QJsonObject &record, const DeployJob &job, const QDateTime &finishedAt = {});

QJsonObject withSuccessDetails(QJsonObject record,
                               const QString &remoteVersionDir,
                               const QStringList &artifactNames);

bool isTerminal(DeploymentStatus status);

}
