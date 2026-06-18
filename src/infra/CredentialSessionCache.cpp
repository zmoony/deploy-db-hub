#include "infra/CredentialSessionCache.h"

void CredentialSessionCache::put(const QString &credentialRef, const QString &secret)
{
    m_secrets.insert(credentialRef, secret);
}

QString CredentialSessionCache::get(const QString &credentialRef) const
{
    return m_secrets.value(credentialRef);
}

void CredentialSessionCache::clear(const QString &credentialRef)
{
    m_secrets.remove(credentialRef);
}

void CredentialSessionCache::clearAll()
{
    m_secrets.clear();
}
