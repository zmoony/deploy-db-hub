#include "infra/LogPath.h"

#include <QDir>
#include <QRegularExpression>

namespace {
const QRegularExpression DeployIdPattern(QStringLiteral("^[a-zA-Z0-9._-]+$"));
const QRegularExpression RelativeLogPathPattern(QStringLiteral("^logs/[a-zA-Z0-9._-]+\\.log$"));
}

bool LogPath::isValidDeployId(const QString &deployId)
{
    return !deployId.isEmpty() && DeployIdPattern.match(deployId).hasMatch();
}

bool LogPath::isValidRelativePath(const QString &relativePath)
{
    return RelativeLogPathPattern.match(relativePath).hasMatch();
}

QString LogPath::relativePath(const QString &deployId)
{
    if (!isValidDeployId(deployId)) {
        return {};
    }
    return QStringLiteral("logs/") + deployId + QStringLiteral(".log");
}

QString LogPath::absolutePath(const QString &dataDir, const QString &relativePath)
{
    if (dataDir.isEmpty() || !isValidRelativePath(relativePath)) {
        return {};
    }
    return QDir(dataDir).filePath(relativePath);
}
