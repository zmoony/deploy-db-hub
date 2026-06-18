#include "infra/DataPaths.h"

#include "infra/AppSettingsStore.h"

#include <QCoreApplication>
#include <QDir>

namespace DataPaths {

namespace {
QString g_configDir;
}

void initialize()
{
    AppSettings settings;
    QString error;
    AppSettingsStore(AppSettingsStore::bootstrapSettingsFile()).load(&settings, &error);
    if (!settings.configDirOverride.trimmed().isEmpty()) {
        g_configDir = QDir::fromNativeSeparators(settings.configDirOverride.trimmed());
    }
}

QString configDir()
{
    const QString dir = g_configDir.isEmpty()
        ? QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config"))
        : g_configDir;
    QDir().mkpath(dir);
    return dir;
}

QString databaseFile()
{
    return QDir(configDir()).filePath(QStringLiteral("deploy-hub.db"));
}

QString logsDir()
{
    const QString dir = QDir(configDir()).filePath(QStringLiteral("logs"));
    QDir().mkpath(dir);
    return dir;
}

QString workspaceDir()
{
    const QString dir = QDir(configDir()).filePath(QStringLiteral("workspace"));
    QDir().mkpath(dir);
    return dir;
}

QString credentialsFile()
{
    return QDir(configDir()).filePath(QStringLiteral("credentials.json"));
}

QString jdkProfilesFile()
{
    return QDir(configDir()).filePath(QStringLiteral("jdk-profiles.json"));
}

}
