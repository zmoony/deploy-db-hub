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
QString defaultRestartCommandSuffix();

// Returns a shell command (Linux or Windows) that locates the running service
// process for the given project, sends SIGTERM, waits up to 5s, then sends
// SIGKILL to any remaining processes. Echoes the list of killed PIDs.
// Returns empty string if the project has no identifiable jar/service match.
QString stopAndKillServiceCommand(const QJsonObject &project);

// Returns a shell command that polls `host:port` until it is listening or the
// timeout (in seconds) is reached. Echoes "LISTENING:<port>" on success or
// "TIMEOUT" on failure. Linux uses `ss`/`nc`; Windows uses Test-NetConnection.
QString waitForPortOpenCommand(const QString &os, const QString &host, int port, int timeoutSec);

// Returns a shell command that polls for the given pid to be alive (Linux
// uses `kill -0`; Windows uses `Get-Process`). Echoes "ALIVE" or "DEAD".
QString waitForPidAliveCommand(const QString &os, int pid, int timeoutSec);

// Best-effort port to health-check after restart. Reads the project's
// `deploy.healthCheckPort` (int) or falls back to the JAR filename's most
// likely port. Returns 0 if no port can be determined (caller skips health
// check in that case).
int resolveHealthCheckPort(const QJsonObject &project);
