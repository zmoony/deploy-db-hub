#pragma once

#include "adapters/services/RedisServiceClient.h"
#include "infra/SqlDriverLoader.h"
#include "infra/ServiceNodeConnection.h"

#include <QString>

class JdbcSqlBridge final
{
public:
    static QString driverJarForProduct(const QString &productKey);
    static bool isConfigured(const QString &productKey);
    static bool isJdbcReady(const QString &productKey);
    static ServiceResult executeQuery(const ServiceEndpoint &endpoint,
                                      const QString &productKey,
                                      const QString &sql);
    static SqlDriverProbeResult probe();
    static SqlDriverProbeResult probe(const AppSettings &settings);
};
