#pragma once

#include <QJsonObject>
#include <QString>

struct ProjectServiceConfig {
    QString serviceMatch;
    QString startCommand;
    QString stopCommand;
    QString restartCommand;
    QString statusCommand;
    QString targetJarPath;
    QString backupPolicy;
    QString backupDir;
};

ProjectServiceConfig projectServiceConfig(const QJsonObject &project);
QString renderProjectServiceCommand(QString commandTemplate,
                                    const QJsonObject &project,
                                    const QString &artifactPath = {});
QString remoteProjectJarPath(const QJsonObject &project, const QString &artifactFileName);
QString remoteProjectBackupPath(const QJsonObject &project,
                                const QString &artifactFileName,
                                const QString &version);
QString pgrepSafePattern(const QString &pattern);
QString defaultLinuxServiceStatusCommand(const ProjectServiceConfig &service);
QString defaultWindowsServiceStatusCommand(const ProjectServiceConfig &service);
