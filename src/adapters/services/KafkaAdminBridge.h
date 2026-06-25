#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/services/RedisServiceClient.h"
#include "infra/ServiceNodeConnection.h"

#include <QVector>

struct KafkaAdminPayload {
    bool ok = false;
    QString message;
    QVector<QJsonObject> rows;
    QVector<QJsonObject> stats;
    QVector<QJsonObject> partitions;
};

class KafkaAdminBridge final
{
public:
    static bool isAvailable();
    static KafkaAdminPayload run(const ServiceEndpoint &endpoint,
                                 const RemoteConnectionContext &remote,
                                 const QStringList &args,
                                 int timeoutSec = 60);
};
