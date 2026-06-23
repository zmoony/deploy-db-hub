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
    static ServiceResult listKeys(const ServiceEndpoint &endpoint, const QString &pattern, int limit = 200);
    static ServiceResult readKey(const ServiceEndpoint &endpoint, const QString &key);
};
