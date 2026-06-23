#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "infra/ServiceNodeConnection.h"

#include "adapters/services/RedisServiceClient.h"

#include <QJsonObject>
#include <QString>

class ServiceBroker final
{
public:
    enum class TabKind {
        Topic,
        ConsumerGroup,
        Node,
        Index,
        Key,
        Table
    };

    static ServiceResult testInstance(const QJsonObject &instance,
                                      const QJsonObject &server,
                                      const QString &productKey,
                                      const RemoteConnectionContext &remote);

    static ServiceResult loadTab(const QJsonObject &instance,
                                 const QJsonObject &server,
                                 const QString &productKey,
                                 TabKind tab,
                                 const RemoteConnectionContext &remote,
                                 const QString &schema = QString());

    static ServiceResult runAction(const QJsonObject &instance,
                                   const QJsonObject &server,
                                   const QString &productKey,
                                   TabKind tab,
                                   const QString &action,
                                   const QJsonObject &selection,
                                   const RemoteConnectionContext &remote,
                                   const QString &schema = QString(),
                                   const QString &input = QString());

    static TabKind tabKindFromTitle(const QString &title);
    static bool resolveContext(const QJsonObject &instance,
                               const QJsonObject &server,
                               const QString &productKey,
                               ServiceEndpoint *endpoint,
                               QString *error);

    static QString metadataDescription(const QJsonObject &instance, const QString &key);
    static QJsonObject setMetadataDescription(QJsonObject instance, const QString &key, const QString &description);
};
