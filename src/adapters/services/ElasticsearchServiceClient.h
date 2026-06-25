#pragma once

#include "infra/ServiceNodeConnection.h"

#include "adapters/services/RedisServiceClient.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

class ElasticsearchServiceClient final
{
public:
    static ServiceResult ping(const ServiceEndpoint &endpoint);
    static ServiceResult clusterHealth(const ServiceEndpoint &endpoint);
    static ServiceResult listIndices(const ServiceEndpoint &endpoint);
    static QVector<QJsonObject> organizeIndexRows(const QVector<QJsonObject> &flatRows);
    static ServiceResult searchIndex(const ServiceEndpoint &endpoint,
                                     const QString &index,
                                     int from = 0,
                                     int size = 10,
                                     const QString &queryText = {});
    static ServiceResult searchIndexBody(const ServiceEndpoint &endpoint,
                                         const QString &index,
                                         const QJsonObject &body);
    static ServiceResult indexCount(const ServiceEndpoint &endpoint, const QString &index);
    static QString kibanaUrl(const ServiceEndpoint &endpoint);
};
