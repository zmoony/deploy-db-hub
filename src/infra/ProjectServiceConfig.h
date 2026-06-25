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
bool usesCustomServiceControl(const QJsonObject &project);
bool isLocalRestartScriptPath(const QString &path, const QString &projectRoot);
QString resolveLocalRestartScriptPath(const QString &path, const QString &projectRoot);
QString remoteRestartScriptPath(const QJsonObject &project, const QString &localScriptPath);
RestartExecutionPlan buildRestartExecutionPlan(const QJsonObject &project, const QString &projectRoot);
QString renderProjectServiceCommand(QString commandTemplate,
                                    const QJsonObject &project,
                                    const QString &artifactPath = {});
// Wrap a remote command so it runs under the directory of the remote artifact.
// Linux: cd '<dir>' && <command>; Windows: cd /d "<dir>" && <command>.
// Returns the original command when workingDir is empty.
QString wrapCommandWithWorkingDirectory(const QString &os,
                                        const QString &command,
                                        const QString &workingDir);
QString remoteProjectJarPath(const QJsonObject &project, const QString &artifactFileName);
QString remoteProjectBackupPath(const QJsonObject &project,
                                const QString &artifactFileName,
                                const QString &version);
QString pgrepSafePattern(const QString &pattern);
QString defaultLinuxServiceStatusCommand(const QJsonObject &project);
QString defaultWindowsServiceStatusCommand(const QJsonObject &project);
