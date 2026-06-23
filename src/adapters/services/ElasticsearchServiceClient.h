#pragma once

#include "infra/ServiceNodeConnection.h"

#include "adapters/services/RedisServiceClient.h"

#include <QJsonObject>
#include <QString>

class ElasticsearchServiceClient final
{
public:
    static ServiceResult ping(const ServiceEndpoint &endpoint);
    static ServiceResult listIndices(const ServiceEndpoint &endpoint);
    static ServiceResult searchIndex(const ServiceEndpoint &endpoint, const QString &index, int size = 10);
    static QString kibanaUrl(const ServiceEndpoint &endpoint);
};
