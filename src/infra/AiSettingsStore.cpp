#include "infra/AiSettingsStore.h"

#include "infra/DataPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include <utility>

namespace {

constexpr int kSchemaVersion = 2;

QJsonObject connectionToJson(const AiConnection &connection)
{
    return QJsonObject{
        {QStringLiteral("id"), connection.id},
        {QStringLiteral("name"), connection.name},
        {QStringLiteral("apiBaseUrl"), connection.apiBaseUrl.trimmed()},
        {QStringLiteral("model"), connection.model.trimmed()},
        {QStringLiteral("credentialRef"), connection.credentialRef.trimmed()}
    };
}

AiConnection connectionFromJson(const QJsonObject &object)
{
    AiConnection connection;
    connection.id = object.value(QStringLiteral("id")).toString().trimmed();
    connection.name = object.value(QStringLiteral("name")).toString().trimmed();
    connection.apiBaseUrl = object.value(QStringLiteral("apiBaseUrl")).toString().trimmed();
    connection.model = object.value(QStringLiteral("model")).toString().trimmed();
    connection.credentialRef = object.value(QStringLiteral("credentialRef")).toString().trimmed();
    if (connection.model.isEmpty()) {
        connection.model = QStringLiteral("gpt-4o-mini");
    }
    if (connection.credentialRef.isEmpty()) {
        connection.credentialRef = AiSettingsStore::buildCredentialRef(connection.id);
    }
    return connection;
}

QVector<AiConnection> connectionsFromJson(const QJsonArray &array)
{
    QVector<AiConnection> result;
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        AiConnection connection = connectionFromJson(value.toObject());
        if (connection.id.isEmpty()) {
            continue;
        }
        result.append(std::move(connection));
    }
    return result;
}

QJsonArray connectionsToJson(const QVector<AiConnection> &connections)
{
    QJsonArray array;
    for (const AiConnection &connection : connections) {
        array.append(connectionToJson(connection));
    }
    return array;
}

void ensureActiveConnection(AiSettings *settings)
{
    if (settings->connections.isEmpty()) {
        AiConnection fallback;
        fallback.id = QStringLiteral("default");
        fallback.name = QStringLiteral("默认连接");
        fallback.apiBaseUrl = settings->apiBaseUrl;
        fallback.model = settings->model;
        fallback.credentialRef = settings->credentialRef.isEmpty()
            ? AiSettingsStore::buildCredentialRef(fallback.id)
            : settings->credentialRef;
        settings->connections.append(std::move(fallback));
        if (settings->activeConnectionId.isEmpty()) {
            settings->activeConnectionId = settings->connections.first().id;
        }
        return;
    }

    bool found = false;
    for (const AiConnection &connection : settings->connections) {
        if (connection.id == settings->activeConnectionId) {
            found = true;
            break;
        }
    }
    if (!found) {
        settings->activeConnectionId = settings->connections.first().id;
    }
}

}

QString AiSettings::generateConnectionId()
{
    return QStringLiteral("conn-") + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

QString AiSettingsStore::buildCredentialRef(const QString &connectionId)
{
    return QStringLiteral("deploy-hub/ai/") + connectionId;
}

QStringList AiSettingsStore::defaultModelPresets()
{
    return {
        QStringLiteral("gpt-4o-mini"),
        QStringLiteral("gpt-4o"),
        QStringLiteral("Qwen/Qwen2-7B"),
        QStringLiteral("deepseek-chat"),
        QStringLiteral("claude-3-5-sonnet"),
        QStringLiteral("gemini-1.5-pro")
    };
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
        ensureActiveConnection(settings);
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

    const QJsonObject object = document.object();
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

    settings->connections = connectionsFromJson(object.value(QStringLiteral("connections")).toArray());
    settings->activeConnectionId = object.value(QStringLiteral("activeConnectionId")).toString().trimmed();

    if (settings->connections.isEmpty()) {
        AiConnection legacy;
        legacy.id = QStringLiteral("default");
        legacy.name = QStringLiteral("默认连接");
        legacy.apiBaseUrl = settings->apiBaseUrl;
        legacy.model = settings->model;
        legacy.credentialRef = settings->credentialRef;
        settings->connections.append(std::move(legacy));
        if (settings->activeConnectionId.isEmpty()) {
            settings->activeConnectionId = settings->connections.first().id;
        }
    }
    ensureActiveConnection(settings);
    return true;
}

bool AiSettingsStore::save(const AiSettings &settings, QString *error) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());

    AiSettings copy = settings;
    ensureActiveConnection(&copy);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    const QJsonObject object{
        {QStringLiteral("schemaVersion"), kSchemaVersion},
        {QStringLiteral("apiBaseUrl"), copy.apiBaseUrl.trimmed()},
        {QStringLiteral("model"), copy.model.trimmed()},
        {QStringLiteral("credentialRef"), copy.credentialRef.trimmed()},
        {QStringLiteral("rememberKey"), copy.rememberKey},
        {QStringLiteral("connections"), connectionsToJson(copy.connections)},
        {QStringLiteral("activeConnectionId"), copy.activeConnectionId.trimmed()}
    };
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

QString AiSettingsStore::defaultSettingsFile()
{
    return QDir(DataPaths::configDir()).filePath(QStringLiteral("ai-settings.json"));
}
