#include "infra/RemoteLogPath.h"

#include <QDir>
#include <QFileInfo>

namespace {

QString shellQuoteSingle(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'%1'").arg(value);
}

QString powerShellSingleQuote(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("''"));
    return QStringLiteral("'%1'").arg(value);
}

QString trimTrailingSlash(QString path)
{
    while (path.size() > 1 && path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    return path;
}

bool looksLikeLogFilePath(const QString &path)
{
    return path.endsWith(QStringLiteral(".log"), Qt::CaseInsensitive)
        || path.endsWith(QStringLiteral(".txt"), Qt::CaseInsensitive);
}

QString directoryFromConfiguredLogDir(const QString &logDir)
{
    const QString normalized = trimTrailingSlash(QDir::fromNativeSeparators(logDir.trimmed()));
    if (normalized.isEmpty()) {
        return {};
    }
    if (looksLikeLogFilePath(normalized)) {
        return trimTrailingSlash(QFileInfo(normalized).path());
    }
    return normalized;
}

void appendUnique(QStringList *values, const QString &value)
{
    if (values == nullptr || value.isEmpty() || values->contains(value)) {
        return;
    }
    values->append(value);
}

}

QString normalizedRemoteLogPath(QString path)
{
    return trimTrailingSlash(QDir::fromNativeSeparators(path.trimmed()));
}

QString remoteBaseDirFromProject(const QJsonObject &project)
{
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    return normalizedRemoteLogPath(deploy.value(QStringLiteral("remoteBaseDir")).toString());
}

QStringList defaultRemoteLogGlobPatterns()
{
    return {QStringLiteral("*.log"), QStringLiteral("*.txt")};
}

QStringList candidateRemoteLogDirectories(const QJsonObject &project)
{
    QStringList directories;
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString remoteBaseDir = remoteBaseDirFromProject(project);
    const QString configuredLogDir = directoryFromConfiguredLogDir(
        deploy.value(QStringLiteral("logDir")).toString());

    if (!configuredLogDir.isEmpty()) {
        appendUnique(&directories, configuredLogDir);
    }
    if (!remoteBaseDir.isEmpty()) {
        appendUnique(&directories, remoteBaseDir + QStringLiteral("/logs"));
        appendUnique(&directories, remoteBaseDir + QStringLiteral("/log"));
    }
    return directories;
}

QStringList buildRemoteLogPathOptions(const QStringList &directories, const QStringList &patterns)
{
    QStringList options;
    for (const QString &directory : directories) {
        const QString normalizedDir = normalizedRemoteLogPath(directory);
        if (normalizedDir.isEmpty()) {
            continue;
        }
        for (const QString &pattern : patterns) {
            appendUnique(&options, normalizedDir + QLatin1Char('/') + pattern);
        }
    }
    return options;
}

QStringList deployLogPathOptionsForProject(const QJsonObject &project)
{
    return buildRemoteLogPathOptions(candidateRemoteLogDirectories(project), defaultRemoteLogGlobPatterns());
}

RemoteLogGlobSpec parseRemoteLogGlobPath(const QString &path)
{
    RemoteLogGlobSpec spec;
    const QString normalized = normalizedRemoteLogPath(path);
    const int slashIndex = normalized.lastIndexOf(QLatin1Char('/'));
    if (slashIndex < 0) {
        if (normalized.contains(QLatin1Char('*'))) {
            spec.directory = QStringLiteral(".");
            spec.filePattern = normalized;
        }
        return spec;
    }

    spec.directory = normalized.left(slashIndex);
    const QString filePart = normalized.mid(slashIndex + 1);
    if (filePart.contains(QLatin1Char('*')) || filePart.contains(QLatin1Char('?'))) {
        spec.filePattern = filePart;
    }
    return spec;
}

bool isRemoteDeployLogPath(const QString &path)
{
    const QString normalized = normalizedRemoteLogPath(path);
    if (normalized.isEmpty()) {
        return false;
    }
    if (normalized.startsWith(QStringLiteral("logs/"))) {
        return false;
    }
    if (normalized.startsWith(QLatin1Char('/')) || normalized.contains(QStringLiteral(":/"))) {
        return true;
    }
    return false;
}

QString remoteDiscoverLogDirectoriesCommand(const QJsonObject &server, const QString &remoteBaseDir)
{
    const QString base = normalizedRemoteLogPath(remoteBaseDir);
    if (base.isEmpty()) {
        return {};
    }

    if (server.value(QStringLiteral("os")).toString() == QStringLiteral("windows")) {
        const QString nativeBase = QDir::toNativeSeparators(base);
        return QStringLiteral("powershell -NoProfile -Command \""
                              "Get-ChildItem -LiteralPath %1 -Directory | "
                              "Where-Object { $_.Name -match '^(?i)logs?$' } | "
                              "ForEach-Object { $_.FullName.Replace('\\','/') }\"")
            .arg(powerShellSingleQuote(nativeBase));
    }

    return QStringLiteral("find %1 -maxdepth 1 -type d \\( -iname 'logs' -o -iname 'log' \\) 2>/dev/null | sort")
        .arg(shellQuoteSingle(base));
}

QStringList parseDiscoveredLogDirectories(const QString &output)
{
    QStringList directories;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        appendUnique(&directories, normalizedRemoteLogPath(line));
    }
    return directories;
}

QStringList remoteLogPathOptionsFromDiscovered(const QJsonObject &project, const QStringList &discovered)
{
    QStringList directories = discovered;
    for (const QString &directory : candidateRemoteLogDirectories(project)) {
        appendUnique(&directories, directory);
    }
    if (directories.isEmpty()) {
        return deployLogPathOptionsForProject(project);
    }
    return buildRemoteLogPathOptions(directories, defaultRemoteLogGlobPatterns());
}

QString sshTailLatestMatchingFileCommand(const RemoteLogGlobSpec &spec, int lineCount)
{
    const int count = qMax(1, lineCount);
    const QString directory = spec.directory.isEmpty() ? QStringLiteral("/") : spec.directory;
    const QString pattern = spec.filePattern.isEmpty() ? QStringLiteral("*.log") : spec.filePattern;
    return QStringLiteral(
        "latest=$(find %1 -maxdepth 1 -type f -name %2 -printf '%%T@ %%p\\n' 2>/dev/null | sort -nr | head -n 1 | cut -d' ' -f2-); "
        "if [ -z \"$latest\" ]; then echo '未找到匹配日志：%3' >&2; exit 1; fi; "
        "tail -n %4 -- \"$latest\"")
        .arg(shellQuoteSingle(directory),
             shellQuoteSingle(pattern),
             directory + QLatin1Char('/') + pattern,
             QString::number(count));
}

QString winRmTailLatestMatchingFileCommand(const RemoteLogGlobSpec &spec, int lineCount)
{
    const int count = qMax(1, lineCount);
    const QString directory = spec.directory.isEmpty() ? QStringLiteral(".") : spec.directory;
    const QString pattern = spec.filePattern.isEmpty() ? QStringLiteral("*.log") : spec.filePattern;
    const QString nativeDir = QDir::toNativeSeparators(directory);
    return QStringLiteral("powershell -NoProfile -Command \""
                          "$latest = Get-ChildItem -LiteralPath %1 -Filter %2 -File | "
                          "Sort-Object LastWriteTime -Descending | Select-Object -First 1; "
                          "if ($null -eq $latest) { Write-Error '未找到匹配日志：%3'; exit 1 }; "
                          "Get-Content -LiteralPath $latest.FullName -Tail %4\"")
        .arg(powerShellSingleQuote(nativeDir),
             powerShellSingleQuote(pattern),
             directory + QLatin1Char('/') + pattern,
             QString::number(count));
}
