#pragma once

#include "infra/ServiceNodeConnection.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

struct ServiceResult {
    bool ok = false;
    QString message;
    QString text;
    QVector<QJsonObject> rows;
};

class RedisServiceClient final
{
public:
    static ServiceResult ping(const ServiceEndpoint &endpoint);
    static ServiceResult serverInfo(const ServiceEndpoint &endpoint);
    static ServiceResult listKeys(const ServiceEndpoint &endpoint,
                                  const QString &pattern,
                                  const QString &typeFilter = QString(),
                                  int limit = 200);
    static ServiceResult readKey(const ServiceEndpoint &endpoint, const QString &key);
    static ServiceResult writeKey(const ServiceEndpoint &endpoint,
                                  const QString &key,
                                  const QString &value,
                                  int ttlSec = -1);
    static ServiceResult deleteKey(const ServiceEndpoint &endpoint, const QString &key);
};
