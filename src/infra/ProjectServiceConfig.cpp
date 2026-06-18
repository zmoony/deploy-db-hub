#include "infra/ProjectServiceConfig.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString normalizedRemotePath(QString path)
{
    path = QDir::fromNativeSeparators(path.trimmed());
    while (path.size() > 1 && path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    return path;
}

QString joinRemotePath(const QString &base, const QString &name)
{
    if (base.isEmpty()) {
        return name;
    }
    if (base.endsWith(QLatin1Char('/'))) {
        return base + name;
    }
    return base + QLatin1Char('/') + name;
}

QString defaultServiceMatch(const QJsonObject &project)
{
    const QString id = project.value(QStringLiteral("id")).toString();
    if (!id.isEmpty()) {
        return id;
    }
    return project.value(QStringLiteral("name")).toString();
}

QString shellQuoteSingle(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'%1'").arg(value);
}

}

ProjectServiceConfig projectServiceConfig(const QJsonObject &project)
{
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    ProjectServiceConfig config;
    config.serviceMatch = deploy.value(QStringLiteral("serviceMatch")).toString().trimmed();
    if (config.serviceMatch.isEmpty()) {
        config.serviceMatch = defaultServiceMatch(project);
    }
    config.startCommand = deploy.value(QStringLiteral("startCommand")).toString().trimmed();
    config.stopCommand = deploy.value(QStringLiteral("stopCommand")).toString().trimmed();
    config.restartCommand = deploy.value(QStringLiteral("restartCommand")).toString().trimmed();
    if (config.restartCommand.isEmpty()) {
        const QString restartScript = deploy.value(QStringLiteral("restartScript")).toString().trimmed();
        if (!restartScript.isEmpty()) {
            config.restartCommand = restartScript;
        }
    }
    config.statusCommand = deploy.value(QStringLiteral("statusCommand")).toString().trimmed();
    config.targetJarPath = normalizedRemotePath(deploy.value(QStringLiteral("targetJarPath")).toString());
    config.backupPolicy = deploy.value(QStringLiteral("backupPolicy")).toString(QStringLiteral("backup"));
    config.backupDir = normalizedRemotePath(deploy.value(QStringLiteral("backupDir")).toString());
    return config;
}

QString renderProjectServiceCommand(QString commandTemplate,
                                    const QJsonObject &project,
                                    const QString &artifactPath)
{
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const ProjectServiceConfig service = projectServiceConfig(project);
    const QString remoteBaseDir = normalizedRemotePath(deploy.value(QStringLiteral("remoteBaseDir")).toString());
    const QString logDir = normalizedRemotePath(deploy.value(QStringLiteral("logDir")).toString());
    const QString targetJar = artifactPath.isEmpty()
        ? service.targetJarPath
        : artifactPath;

    commandTemplate.replace(QStringLiteral("{projectId}"), project.value(QStringLiteral("id")).toString());
    commandTemplate.replace(QStringLiteral("{projectName}"), project.value(QStringLiteral("name")).toString());
    commandTemplate.replace(QStringLiteral("{remoteBaseDir}"), remoteBaseDir);
    commandTemplate.replace(QStringLiteral("{logDir}"), logDir);
    commandTemplate.replace(QStringLiteral("{serviceMatch}"), service.serviceMatch);
    commandTemplate.replace(QStringLiteral("{artifactPath}"), targetJar);
    commandTemplate.replace(QStringLiteral("{targetJarPath}"), targetJar);
    return commandTemplate;
}

QString remoteProjectJarPath(const QJsonObject &project, const QString &artifactFileName)
{
    const ProjectServiceConfig service = projectServiceConfig(project);
    if (!service.targetJarPath.isEmpty()) {
        return service.targetJarPath;
    }
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString remoteBaseDir = normalizedRemotePath(deploy.value(QStringLiteral("remoteBaseDir")).toString());
    return joinRemotePath(remoteBaseDir, artifactFileName);
}

QString remoteProjectBackupPath(const QJsonObject &project,
                                const QString &artifactFileName,
                                const QString &version)
{
    const ProjectServiceConfig service = projectServiceConfig(project);
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString remoteBaseDir = normalizedRemotePath(deploy.value(QStringLiteral("remoteBaseDir")).toString());
    const QString backupDir = service.backupDir.isEmpty()
        ? joinRemotePath(remoteBaseDir, QStringLiteral("bak"))
        : service.backupDir;
    const QString stem = QFileInfo(artifactFileName).completeBaseName();
    const QString suffix = QFileInfo(artifactFileName).suffix();
    const QString backupName = suffix.isEmpty()
        ? QStringLiteral("%1-%2.bak").arg(stem, version)
        : QStringLiteral("%1-%2.bak.%3").arg(stem, version, suffix);
    return joinRemotePath(backupDir, backupName);
}

QString pgrepSafePattern(const QString &pattern)
{
    if (pattern.isEmpty()) {
        return QString();
    }
    return QStringLiteral("[%1]%2").arg(pattern.at(0)).arg(pattern.mid(1));
}

QString defaultLinuxServiceStatusCommand(const ProjectServiceConfig &service)
{
    QString pgrepPattern;
    if (!service.targetJarPath.isEmpty()) {
        const QString jarName = QFileInfo(service.targetJarPath).fileName();
        pgrepPattern = QStringLiteral("[j]ava.*-jar.*%1").arg(pgrepSafePattern(jarName));
    } else {
        pgrepPattern = pgrepSafePattern(service.serviceMatch);
    }

    return QStringLiteral(
        "for pid in $(pgrep -f -- %1 2>/dev/null); do "
        "if kill -0 \"$pid\" 2>/dev/null; then echo RUNNING:$pid; exit 0; fi; "
        "done; echo STOPPED")
        .arg(shellQuoteSingle(pgrepPattern));
}

QString defaultWindowsServiceStatusCommand(const ProjectServiceConfig &service)
{
    const QString matchLiteral = service.targetJarPath.isEmpty()
        ? service.serviceMatch
        : QFileInfo(service.targetJarPath).fileName();
    const QString match = shellQuoteSingle(matchLiteral);
    if (!service.targetJarPath.isEmpty()) {
        return QStringLiteral(
            "powershell -NoProfile -Command \""
            "$match = %1; "
            "$p = Get-CimInstance Win32_Process | Where-Object { "
            "$_.Name -eq 'java.exe' -and $_.CommandLine -like ('*' + $match + '*') -and $_.CommandLine -like '*-jar*' "
            "} | Select-Object -First 1; "
            "if ($null -eq $p) { 'STOPPED' } else { 'RUNNING:' + $p.ProcessId }\"")
            .arg(match);
    }

    return QStringLiteral(
        "powershell -NoProfile -Command \""
        "$match = %1; "
        "$p = Get-CimInstance Win32_Process | Where-Object { "
        "$_.CommandLine -like ('*' + $match + '*') -and $_.CommandLine -notlike '*powershell*' -and $_.CommandLine -notlike '*winrs*' "
        "} | Select-Object -First 1; "
        "if ($null -eq $p) { 'STOPPED' } else { 'RUNNING:' + $p.ProcessId }\"")
        .arg(match);
}
