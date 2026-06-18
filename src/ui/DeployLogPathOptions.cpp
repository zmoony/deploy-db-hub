#include "ui/DeployLogPathOptions.h"

#include <QDir>

QStringList deployLogPathOptionsForProject(const QJsonObject &project)
{
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString logDir = QDir::fromNativeSeparators(deploy.value(QStringLiteral("logDir")).toString().trimmed());
    if (logDir.isEmpty()) {
        return {};
    }

    QString normalized = logDir;
    while (normalized.size() > 1 && normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }

    return {normalized + QStringLiteral("/*.log")};
}

bool isRemoteDeployLogPath(const QString &path)
{
    const QString normalized = QDir::fromNativeSeparators(path.trimmed());
    if (normalized.isEmpty()) {
        return false;
    }
    if (normalized.startsWith(QStringLiteral("logs/"))) {
        return false;
    }
    return normalized.startsWith(QLatin1Char('/'))
        || normalized.contains(QStringLiteral(":/"))
        || normalized.contains(QStringLiteral("/*.log"));
}
