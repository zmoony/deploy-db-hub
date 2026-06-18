#pragma once

#include <QHash>
#include <QString>

class CredentialSessionCache final
{
public:
    void put(const QString &credentialRef, const QString &secret);
    QString get(const QString &credentialRef) const;
    void clear(const QString &credentialRef);
    void clearAll();

private:
    QHash<QString, QString> m_secrets;
};
