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

bool isLocalRestartScriptPath(const QString &path, const QString &projectRoot)
{
    const QString normalized = QDir::fromNativeSeparators(path.trimmed());
    if (normalized.isEmpty()) {
        return false;
    }
    if (QFileInfo::exists(normalized)) {
        return true;
    }
    if (normalized.size() >= 3 && normalized.at(1) == QLatin1Char(':')
        && (normalized.at(2) == QLatin1Char('/') || normalized.at(2) == QLatin1Char('\\'))) {
        return true;
    }
    if (!QDir::isAbsolutePath(normalized)) {
        return !normalized.startsWith(QLatin1Char('/'));
    }
    if (normalized.startsWith(QLatin1Char('/'))) {
        return false;
    }
    return false;
}

QString resolveLocalRestartScriptPath(const QString &path, const QString &projectRoot)
{
    const QString normalized = QDir::fromNativeSeparators(path.trimmed());
    if (QDir::isAbsolutePath(normalized)) {
        return normalized;
    }
    return QDir(projectRoot).filePath(normalized);
}

QString remoteRestartScriptPath(const QJsonObject &project, const QString &localScriptPath)
{
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString remoteBaseDir = normalizedRemotePath(deploy.value(QStringLiteral("remoteBaseDir")).toString());
    return joinRemotePath(remoteBaseDir, QFileInfo(localScriptPath).fileName());
}

RestartExecutionPlan buildRestartExecutionPlan(const QJsonObject &project, const QString &projectRoot)
{
    RestartExecutionPlan plan;
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString restartCommand = deploy.value(QStringLiteral("restartCommand")).toString().trimmed();
    const QString restartScript = deploy.value(QStringLiteral("restartScript")).toString().trimmed();

    if (!restartCommand.isEmpty()) {
        if (isLocalRestartScriptPath(restartCommand, projectRoot)) {
            plan.localScriptPath = resolveLocalRestartScriptPath(restartCommand, projectRoot);
            plan.requiresScriptUpload = true;
        } else {
            plan.remoteCommand = restartCommand;
        }
    } else if (!restartScript.isEmpty()) {
        plan.localScriptPath = resolveLocalRestartScriptPath(restartScript, projectRoot);
        plan.requiresScriptUpload = true;
    }

    if (plan.requiresScriptUpload && !plan.localScriptPath.isEmpty()) {
        const QString remoteScript = remoteRestartScriptPath(project, plan.localScriptPath);
        plan.remoteCommand = QStringLiteral("bash %1").arg(shellQuoteSingle(remoteScript));
    }
    return plan;
}

QString pgrepSafePattern(const QString &pattern)
{
    if (pattern.isEmpty()) {
        return QString();
    }
    return QStringLiteral("[%1]%2").arg(pattern.at(0)).arg(pattern.mid(1));
}

QString defaultLinuxServiceStatusCommand(const QJsonObject &project)
{
    const ProjectServiceConfig service = projectServiceConfig(project);
    const QString remoteBaseDir = normalizedRemotePath(
        project.value(QStringLiteral("deploy")).toObject().value(QStringLiteral("remoteBaseDir")).toString());

    QString command = QStringLiteral(
        "check() { for pid in $(pgrep -f -- \"$1\" 2>/dev/null); do "
        "if kill -0 \"$pid\" 2>/dev/null; then echo RUNNING:$pid; exit 0; fi; done; }; ");

    if (!service.targetJarPath.isEmpty()) {
        const QString jarName = QFileInfo(service.targetJarPath).fileName();
        command += QStringLiteral("check %1; check %2; ")
                       .arg(shellQuoteSingle(QStringLiteral("[j]ava.*-jar.*") + pgrepSafePattern(jarName)),
                            shellQuoteSingle(pgrepSafePattern(jarName)));
    } else if (!remoteBaseDir.isEmpty()) {
        command += QStringLiteral(
            "latest_jar=$(find %1 -maxdepth 1 -type f -name '*.jar' -printf '%%T@ %%f\\n' 2>/dev/null | "
            "sort -nr | head -n 1 | cut -d' ' -f2-); "
            "if [ -n \"$latest_jar\" ]; then "
            "  jar_pat=$(printf '%%s' \"$latest_jar\" | sed 's/^\\(.\\)/[\\1]/'); "
            "  check \"$jar_pat\"; "
            "fi; ")
            .arg(shellQuoteSingle(remoteBaseDir));
    }

    if (!service.serviceMatch.isEmpty() && service.targetJarPath.isEmpty() && remoteBaseDir.isEmpty()) {
        command += QStringLiteral("check %1; ").arg(shellQuoteSingle(pgrepSafePattern(service.serviceMatch)));
    }

    command += QStringLiteral("echo STOPPED");
    return command;
}

QString defaultWindowsServiceStatusCommand(const QJsonObject &project)
{
    const ProjectServiceConfig service = projectServiceConfig(project);
    const QString remoteBaseDir = normalizedRemotePath(
        project.value(QStringLiteral("deploy")).toObject().value(QStringLiteral("remoteBaseDir")).toString());

    if (!service.targetJarPath.isEmpty()) {
        const QString match = shellQuoteSingle(QFileInfo(service.targetJarPath).fileName());
        return QStringLiteral(
            "powershell -NoProfile -Command \""
            "$match = %1; "
            "$p = Get-CimInstance Win32_Process | Where-Object { "
            "$_.Name -eq 'java.exe' -and $_.CommandLine -like ('*' + $match + '*') "
            "} | Select-Object -First 1; "
            "if ($null -eq $p) { 'STOPPED' } else { 'RUNNING:' + $p.ProcessId }\"")
            .arg(match);
    }

    if (!remoteBaseDir.isEmpty()) {
        const QString nativeBase = QDir::toNativeSeparators(remoteBaseDir);
        return QStringLiteral(
            "powershell -NoProfile -Command \""
            "$base = %1; "
            "$latest = Get-ChildItem -LiteralPath $base -Filter '*.jar' -File | Sort-Object LastWriteTime -Descending | Select-Object -First 1; "
            "if ($null -eq $latest) { 'STOPPED'; exit 0 }; "
            "$match = $latest.Name; "
            "$p = Get-CimInstance Win32_Process | Where-Object { "
            "$_.Name -eq 'java.exe' -and $_.CommandLine -like ('*' + $match + '*') "
            "} | Select-Object -First 1; "
            "if ($null -eq $p) { 'STOPPED' } else { 'RUNNING:' + $p.ProcessId }\"")
            .arg(shellQuoteSingle(nativeBase));
    }

    const QString match = shellQuoteSingle(service.serviceMatch);
    return QStringLiteral(
        "powershell -NoProfile -Command \""
        "$match = %1; "
        "$p = Get-CimInstance Win32_Process | Where-Object { "
        "$_.CommandLine -like ('*' + $match + '*') -and $_.CommandLine -notlike '*powershell*' -and $_.CommandLine -notlike '*winrs*' "
        "} | Select-Object -First 1; "
        "if ($null -eq $p) { 'STOPPED' } else { 'RUNNING:' + $p.ProcessId }\"")
        .arg(match);
}
