#pragma once

#include "infra/ServiceNodeConnection.h"

#include "adapters/services/RedisServiceClient.h"

#include <QString>

class SqlServiceClient final
{
public:
    static ServiceResult ping(const ServiceEndpoint &endpoint, const QString &productKey);
    static ServiceResult listSchemas(const ServiceEndpoint &endpoint, const QString &productKey);
    static ServiceResult listTables(const ServiceEndpoint &endpoint, const QString &productKey, const QString &schema);
    static ServiceResult executeQuery(const ServiceEndpoint &endpoint, const QString &productKey, const QString &sql);
    static ServiceResult describeTable(const ServiceEndpoint &endpoint,
                                       const QString &productKey,
                                       const QString &schema,
                                       const QString &table);
    static ServiceResult fetchTableDdl(const ServiceEndpoint &endpoint,
                                       const QString &productKey,
                                       const QString &schema,
                                       const QString &table);
    static QString ddlTextFromResult(const ServiceResult &result);
    static ServiceResult dropTable(const ServiceEndpoint &endpoint,
                                   const QString &productKey,
                                   const QString &schema,
                                   const QString &table);
    static QString withSchemaContext(const QString &productKey, const QString &schema, const QString &sql);
    static ServiceResult sampleRows(const ServiceEndpoint &endpoint,
                                    const QString &productKey,
                                    const QString &schema,
                                    const QString &table,
                                    int limit = 20);
    static QString defaultSelectSql(const QString &productKey,
                                    const QString &schema,
                                    const QString &table,
                                    int limit = 20);
};
