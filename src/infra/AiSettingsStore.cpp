#include "infra/AiSettingsStore.h"

#include "infra/DataPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace {

constexpr int kSchemaVersion = 1;

void applySettingsObject(const QJsonObject &object, AiSettings *settings)
{
    settings->apiBaseUrl = object.value(QStringLiteral("apiBaseUrl")).toString().trimmed();
    settings->model = object.value(QStringLiteral("model")).toString().trimmed();
    if (settings->model.isEmpty()) {
        settings->model = QStringLiteral("gpt-4o-mini");
    }
    settings->credentialRef = object.value(QStringLiteral("credentialRef")).toString().trimmed();
    if (settings->credentialRef.isEmpty()) {
        settings->credentialRef = QStringLiteral("deploy-hub/ai-api-key");
    }
    settings->rememberKey = object.value(QStringLiteral("rememberKey")).toBool(true);
}

}

AiSettingsStore::AiSettingsStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

bool AiSettingsStore::load(AiSettings *settings, QString *error) const
{
    if (settings == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("settings output is null");
        }
        return false;
    }

    *settings = {};
    QFile file(m_filePath);
    if (!file.exists()) {
        return true;
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
            *error = QStringLiteral("ai-settings file must contain an object");
        }
        return false;
    }

    applySettingsObject(document.object(), settings);
    return true;
}

bool AiSettingsStore::save(const AiSettings &settings, QString *error) const
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
        {QStringLiteral("schemaVersion"), kSchemaVersion},
        {QStringLiteral("apiBaseUrl"), settings.apiBaseUrl.trimmed()},
        {QStringLiteral("model"), settings.model.trimmed()},
        {QStringLiteral("credentialRef"), settings.credentialRef.trimmed()},
        {QStringLiteral("rememberKey"), settings.rememberKey}
    };
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

QString AiSettingsStore::defaultSettingsFile()
{
    return QDir(DataPaths::configDir()).filePath(QStringLiteral("ai-settings.json"));
}
