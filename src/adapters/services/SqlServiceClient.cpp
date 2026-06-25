#include "adapters/services/SqlServiceClient.h"

#include "adapters/services/JdbcSqlBridge.h"
#include "infra/SqlDriverLoader.h"

#include <QJsonObject>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

namespace {

QString driverNameForProduct(const QString &productKey)
{
    return productKey == QStringLiteral("oracle") ? QStringLiteral("QOCI") : QStringLiteral("QPSQL");
}

QString connectionName()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ServiceResult runSql(const ServiceEndpoint &endpoint, const QString &productKey, const QString &sql)
{
    if (JdbcSqlBridge::isJdbcReady(productKey)) {
        return JdbcSqlBridge::executeQuery(endpoint, productKey, sql);
    }

    SqlDriverLoader::applyFromStore();

    const QString driver = driverNameForProduct(productKey);
    if (!QSqlDatabase::isDriverAvailable(driver)) {
        return {false,
                QStringLiteral("未加载 %1 驱动。请在「设置 → 数据库驱动」配置 JDBC 驱动 JAR。").arg(driver),
                {},
                {}};
    }

    const QString name = connectionName();
    QSqlDatabase database = QSqlDatabase::addDatabase(driver, name);
    database.setHostName(endpoint.host);
    database.setPort(endpoint.port);
    database.setDatabaseName(endpoint.database);
    database.setUserName(endpoint.username);
    database.setPassword(endpoint.password);

    if (!database.open()) {
        const QString error = database.lastError().text();
        QSqlDatabase::removeDatabase(name);
        return {false, error, {}, {}};
    }

    QSqlQuery query(database);
    if (!query.exec(sql)) {
        const ServiceResult result{false, query.lastError().text(), {}, {}};
        database.close();
        QSqlDatabase::removeDatabase(name);
        return result;
    }

    ServiceResult result{true, {}, {}, {}};
    if (query.isSelect()) {
        while (query.next()) {
            QJsonObject row;
            const QSqlRecord record = query.record();
            for (int i = 0; i < record.count(); ++i) {
                row.insert(record.fieldName(i), record.value(i).toString());
            }
            result.rows.append(row);
        }
        if (query.lastError().isValid()) {
            result = {false, query.lastError().text(), {}, {}};
        }
    } else {
        result.message = QStringLiteral("影响行数: %1").arg(query.numRowsAffected());
    }

    database.close();
    QSqlDatabase::removeDatabase(name);
    return result;
}

ServiceResult mapListTablesResult(const ServiceResult &loaded,
                                  const QString &productKey)
{
    if (!loaded.ok) {
        return loaded;
    }

    ServiceResult mapped{true, {}, {}, {}};
    for (const QJsonValue &value : loaded.rows) {
        const QJsonObject row = value.toObject();
        if (productKey == QStringLiteral("oracle")) {
            mapped.rows.append(QJsonObject{
                {QStringLiteral("name"), row.value(QStringLiteral("table_name")).toString()},
                {QStringLiteral("description"), row.value(QStringLiteral("table_name")).toString()},
                {QStringLiteral("columns"), QStringLiteral("-")},
                {QStringLiteral("partitioned"), QStringLiteral("no")},
                {QStringLiteral("rows"), row.value(QStringLiteral("num_rows")).toString()}
            });
        } else {
            mapped.rows.append(QJsonObject{
                {QStringLiteral("name"), row.value(QStringLiteral("table_name")).toString()},
                {QStringLiteral("description"), row.value(QStringLiteral("table_name")).toString()},
                {QStringLiteral("columns"), row.value(QStringLiteral("column_count")).toString()},
                {QStringLiteral("partitioned"), QStringLiteral("no")},
                {QStringLiteral("rows"), QStringLiteral("-")}
            });
        }
    }
    return mapped;
}

QString listTablesSql(const QString &productKey, const QString &schema)
{
    QString escapedSchema = schema;
    escapedSchema.replace(QLatin1Char('\''), QStringLiteral("''"));
    if (productKey == QStringLiteral("oracle")) {
        return QStringLiteral("SELECT table_name, num_rows FROM all_tables WHERE owner = '%1' ORDER BY table_name")
            .arg(escapedSchema.toUpper());
    }
    return QStringLiteral(
               "SELECT c.table_name, "
               "(SELECT COUNT(*) FROM information_schema.columns col "
               " WHERE col.table_schema = c.table_schema AND col.table_name = c.table_name) AS column_count, "
               "COALESCE(pg_total_relation_size(format('%I.%I', c.table_schema, c.table_name)), 0) AS rel_size "
               "FROM information_schema.tables c "
               "WHERE c.table_schema = '%1' AND c.table_type = 'BASE TABLE' "
               "ORDER BY c.table_name")
        .arg(escapedSchema);
}

}

ServiceResult SqlServiceClient::ping(const ServiceEndpoint &endpoint, const QString &productKey)
{
    const QString sql = productKey == QStringLiteral("oracle") ? QStringLiteral("SELECT 1 FROM DUAL")
                                                               : QStringLiteral("SELECT 1");
    const ServiceResult result = runSql(endpoint, productKey, sql);
    if (!result.ok) {
        return result;
    }
    return {true, QStringLiteral("数据库连接成功"), {}, {}};
}

ServiceResult SqlServiceClient::listSchemas(const ServiceEndpoint &endpoint, const QString &productKey)
{
    const QString sql = productKey == QStringLiteral("oracle")
        ? QStringLiteral("SELECT DISTINCT owner FROM all_tables ORDER BY owner")
        : QStringLiteral("SELECT schema_name FROM information_schema.schemata "
                         "WHERE schema_name NOT IN ('pg_catalog','information_schema') ORDER BY 1");
    return runSql(endpoint, productKey, sql);
}

ServiceResult SqlServiceClient::listTables(const ServiceEndpoint &endpoint,
                                           const QString &productKey,
                                           const QString &schema)
{
    return mapListTablesResult(runSql(endpoint, productKey, listTablesSql(productKey, schema)), productKey);
}

ServiceResult SqlServiceClient::executeQuery(const ServiceEndpoint &endpoint,
                                             const QString &productKey,
                                             const QString &sql)
{
    return runSql(endpoint, productKey, sql);
}

QString SqlServiceClient::defaultSelectSql(const QString &productKey,
                                           const QString &schema,
                                           const QString &table,
                                           int limit)
{
    if (productKey == QStringLiteral("oracle")) {
        return QStringLiteral("SELECT * FROM %1.%2 FETCH FIRST %3 ROWS ONLY")
            .arg(schema, table, QString::number(limit));
    }
    return QStringLiteral("SELECT * FROM %1.%2 LIMIT %3").arg(schema, table, QString::number(limit));
}

ServiceResult SqlServiceClient::sampleRows(const ServiceEndpoint &endpoint,
                                           const QString &productKey,
                                           const QString &schema,
                                           const QString &table,
                                           int limit)
{
    return executeQuery(endpoint, productKey, defaultSelectSql(productKey, schema, table, limit));
}
