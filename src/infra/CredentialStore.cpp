#include "infra/CredentialStore.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString normalizeRef(const QString &credentialRef)
{
    return credentialRef.trimmed();
}

}

CredentialStore::CredentialStore(const QString &storagePath)
    : m_storagePath(storagePath)
{
    QString error;
    reload(&error);
}

bool CredentialStore::save(const QString &credentialRef, const QString &secret, QString *error)
{
    const QString ref = normalizeRef(credentialRef);
    if (ref.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("credentialRef is empty");
        }
        return false;
    }

    m_secrets.insert(ref, secret);
    return persist(error);
}

QString CredentialStore::load(const QString &credentialRef) const
{
    return m_secrets.value(normalizeRef(credentialRef));
}

bool CredentialStore::remove(const QString &credentialRef, QString *error)
{
    m_secrets.remove(normalizeRef(credentialRef));
    return persist(error);
}

bool CredentialStore::has(const QString &credentialRef) const
{
    return m_secrets.contains(normalizeRef(credentialRef));
}

bool CredentialStore::persist(QString *error)
{
    QJsonObject object;
    for (auto it = m_secrets.constBegin(); it != m_secrets.constEnd(); ++it) {
        object.insert(it.key(), it.value());
    }

    QFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error != nullptr) {
            *error = QStringLiteral("cannot write credential store: %1").arg(m_storagePath);
        }
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    return true;
}

bool CredentialStore::reload(QString *error)
{
    m_secrets.clear();

    QFile file(m_storagePath);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = QStringLiteral("cannot read credential store: %1").arg(m_storagePath);
        }
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("credential store is not a JSON object");
        }
        return false;
    }

    const QJsonObject object = document.object();
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.value().isString()) {
            m_secrets.insert(it.key(), it.value().toString());
        }
    }
    return true;
}
