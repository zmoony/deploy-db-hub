#include "infra/ServiceInstanceStore.h"

#include "infra/DataPaths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

ServiceInstanceStore::ServiceInstanceStore(QString filePath)
    : m_filePath(std::move(filePath))
{
}

QString ServiceInstanceStore::defaultFilePath()
{
    return QDir(DataPaths::configDir()).filePath(QStringLiteral("service-instances.json"));
}

bool ServiceInstanceStore::loadAll(QJsonObject *document, QString *error) const
{
    if (document == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("document output is null");
        }
        return false;
    }

    document->insert(QStringLiteral("version"), 1);
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

    const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    if (!json.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("service-instances.json must be an object");
        }
        return false;
    }

    *document = json.object();
    return true;
}

bool ServiceInstanceStore::saveAll(const QJsonObject &document, QString *error) const
{
    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(document).toJson(QJsonDocument::Indented));
    return true;
}

bool ServiceInstanceStore::load(const QString &productKey, QVector<QJsonObject> *instances, QString *error) const
{
    if (instances == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("instances output is null");
        }
        return false;
    }

    instances->clear();
    QJsonObject document;
    if (!loadAll(&document, error)) {
        return false;
    }

    const QJsonArray array = document.value(productKey).toArray();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            instances->append(value.toObject());
        }
    }
    return true;
}

bool ServiceInstanceStore::save(const QString &productKey, const QVector<QJsonObject> &instances, QString *error) const
{
    QJsonObject document;
    if (!loadAll(&document, error)) {
        return false;
    }

    QJsonArray array;
    for (const QJsonObject &instance : instances) {
        array.append(instance);
    }
    document.insert(productKey, array);
    document.insert(QStringLiteral("version"), 1);
    return saveAll(document, error);
}
