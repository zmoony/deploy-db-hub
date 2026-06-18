#pragma once

#include "adapters/remote/RemoteConnection.h"

#include <QJsonObject>
#include <QString>

class CredentialSessionCache;
class CredentialStore;
class QWidget;

class RemoteCredentialResolver final
{
public:
    static RemoteConnectionContext resolve(const QJsonObject &serverConfig,
                                           CredentialStore *store,
                                           CredentialSessionCache *sessionCache,
                                           QWidget *promptParent = nullptr,
                                           bool allowPrompt = true);

    static QString credentialRefFor(const QJsonObject &serverConfig);
};
