#include "infra/AppSettingsStore.h"

#include "infra/DataPaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <utility>

namespace {

QString bootstrapConfigDir()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config"));
}

QString legacySettingsFile()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(root.isEmpty() ? QDir::homePath() : root).filePath(QStringLiteral("settings.json"));
}

bool loadSettingsObject(const QString &filePath, QJsonObject *object, QString *error)
{
    QFile file(filePath);
    if (!file.exists()) {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("settings file must contain an object");
        }
        return false;
    }

    *object = document.object();
    return true;
}

void applySettingsObject(const QJsonObject &object, AppSettings *settings)
{
    settings->configDirOverride = QDir::fromNativeSeparators(object.value(QStringLiteral("configDir")).toString().trimmed());
    settings->mavenHome = QDir::fromNativeSeparators(object.value(QStringLiteral("mavenHome")).toString().trimmed());
    settings->mavenRepository = QDir::fromNativeSeparators(object.value(QStringLiteral("mavenRepository")).toString().trimmed());
}

}

AppSettingsStore::AppSettingsStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

bool AppSettingsStore::load(AppSettings *settings, QString *error) const
{
    if (settings == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("settings output is null");
        }
        return false;
    }

    *settings = {};
    QJsonObject object;
    QString loadError;
    if (loadSettingsObject(m_filePath, &object, &loadError)) {
        applySettingsObject(object, settings);
        return true;
    }
    if (!loadError.isEmpty()) {
        if (error != nullptr) {
            *error = loadError;
        }
        return false;
    }

    if (m_filePath == defaultSettingsFile() && loadSettingsObject(legacySettingsFile(), &object, &loadError)) {
        applySettingsObject(object, settings);
        save(*settings, nullptr);
        return true;
    }

    return true;
}

bool AppSettingsStore::save(const AppSettings &settings, QString *error) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    const QJsonObject object{
        {QStringLiteral("configDir"), QDir::fromNativeSeparators(settings.configDirOverride.trimmed())},
        {QStringLiteral("mavenHome"), QDir::fromNativeSeparators(settings.mavenHome.trimmed())},
        {QStringLiteral("mavenRepository"), QDir::fromNativeSeparators(settings.mavenRepository.trimmed())}
    };
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

QString AppSettingsStore::defaultSettingsFile()
{
    return QDir(DataPaths::configDir()).filePath(QStringLiteral("settings.json"));
}

QString AppSettingsStore::bootstrapSettingsFile()
{
    return QDir(bootstrapConfigDir()).filePath(QStringLiteral("settings.json"));
}
