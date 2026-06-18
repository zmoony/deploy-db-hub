#pragma once

#include <QHash>
#include <QString>

class CredentialStore final
{
public:
    explicit CredentialStore(const QString &storagePath);

    bool save(const QString &credentialRef, const QString &secret, QString *error = nullptr);
    QString load(const QString &credentialRef) const;
    bool remove(const QString &credentialRef, QString *error = nullptr);
    bool has(const QString &credentialRef) const;

private:
    bool persist(QString *error);
    bool reload(QString *error);

    QString m_storagePath;
    QHash<QString, QString> m_secrets;
};
