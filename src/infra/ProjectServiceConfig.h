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

struct RestartExecutionPlan {
    QString localScriptPath;
    QString remoteCommand;
    bool requiresScriptUpload = false;
};

ProjectServiceConfig projectServiceConfig(const QJsonObject &project);
bool isLocalRestartScriptPath(const QString &path, const QString &projectRoot);
QString resolveLocalRestartScriptPath(const QString &path, const QString &projectRoot);
QString remoteRestartScriptPath(const QJsonObject &project, const QString &localScriptPath);
RestartExecutionPlan buildRestartExecutionPlan(const QJsonObject &project, const QString &projectRoot);
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
