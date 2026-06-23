#include "adapters/services/SqlServiceClient.h"

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

ServiceResult openDatabase(const ServiceEndpoint &endpoint, const QString &productKey, QSqlDatabase *database)
{
    const QString driver = driverNameForProduct(productKey);
    if (!QSqlDatabase::isDriverAvailable(driver)) {
        return {false,
                QStringLiteral("当前环境未安装 %1 驱动，无法连接 %2")
                    .arg(driver, productKey),
                {},
                {}};
    }

    const QString name = connectionName();
    *database = QSqlDatabase::addDatabase(driver, name);
    database->setHostName(endpoint.host);
    database->setPort(endpoint.port);
    database->setDatabaseName(endpoint.database);
    database->setUserName(endpoint.username);
    database->setPassword(endpoint.password);

    if (!database->open()) {
        const QString error = database->lastError().text();
        QSqlDatabase::removeDatabase(name);
        return {false, error, {}, {}};
    }
    return {true, QStringLiteral("connected"), {}, {}};
}

void closeDatabase(QSqlDatabase *database)
{
    if (database == nullptr || !database->isValid()) {
        return;
    }
    const QString name = database->connectionName();
    database->close();
    QSqlDatabase::removeDatabase(name);
}

ServiceResult rowsFromQuery(QSqlQuery &query)
{
    ServiceResult result{true, {}, {}, {}};
    while (query.next()) {
        QJsonObject row;
        const QSqlRecord record = query.record();
        for (int i = 0; i < record.count(); ++i) {
            row.insert(record.fieldName(i), record.value(i).toString());
        }
        result.rows.append(row);
    }
    if (query.lastError().isValid()) {
        return {false, query.lastError().text(), {}, {}};
    }
    return result;
}

}

ServiceResult SqlServiceClient::ping(const ServiceEndpoint &endpoint, const QString &productKey)
{
    QSqlDatabase database;
    const ServiceResult opened = openDatabase(endpoint, productKey, &database);
    if (!opened.ok) {
        return opened;
    }
    QSqlQuery query(database);
    const bool ok = productKey == QStringLiteral("oracle") ? query.exec(QStringLiteral("SELECT 1 FROM DUAL"))
                                                           : query.exec(QStringLiteral("SELECT 1"));
    if (!ok) {
        const ServiceResult result{false, query.lastError().text(), {}, {}};
        closeDatabase(&database);
        return result;
    }
    closeDatabase(&database);
    return {true, QStringLiteral("数据库连接成功"), {}, {}};
}

ServiceResult SqlServiceClient::listSchemas(const ServiceEndpoint &endpoint, const QString &productKey)
{
    QSqlDatabase database;
    const ServiceResult opened = openDatabase(endpoint, productKey, &database);
    if (!opened.ok) {
        return opened;
    }

    QSqlQuery query(database);
    const bool ok = productKey == QStringLiteral("oracle")
        ? query.exec(QStringLiteral("SELECT DISTINCT owner FROM all_tables ORDER BY owner"))
        : query.exec(QStringLiteral("SELECT schema_name FROM information_schema.schemata "
                                    "WHERE schema_name NOT IN ('pg_catalog','information_schema') ORDER BY 1"));
    const ServiceResult result = ok ? rowsFromQuery(query) : ServiceResult{false, query.lastError().text(), {}, {}};
    closeDatabase(&database);
    return result;
}

ServiceResult SqlServiceClient::listTables(const ServiceEndpoint &endpoint,
                                           const QString &productKey,
                                           const QString &schema)
{
    QSqlDatabase database;
    const ServiceResult opened = openDatabase(endpoint, productKey, &database);
    if (!opened.ok) {
        return opened;
    }

    QSqlQuery query(database);
    bool ok = false;
    if (productKey == QStringLiteral("oracle")) {
        query.prepare(QStringLiteral("SELECT table_name, num_rows FROM all_tables WHERE owner = :schema ORDER BY table_name"));
        query.bindValue(QStringLiteral(":schema"), schema.toUpper());
        ok = query.exec();
    } else {
        query.prepare(QStringLiteral(
            "SELECT c.table_name, "
            "(SELECT COUNT(*) FROM information_schema.columns col "
            " WHERE col.table_schema = c.table_schema AND col.table_name = c.table_name) AS column_count, "
            "COALESCE(pg_total_relation_size(format('%I.%I', c.table_schema, c.table_name)), 0) AS rel_size "
            "FROM information_schema.tables c "
            "WHERE c.table_schema = :schema AND c.table_type = 'BASE TABLE' "
            "ORDER BY c.table_name"));
        query.bindValue(QStringLiteral(":schema"), schema);
        ok = query.exec();
    }

    if (!ok) {
        const ServiceResult result{false, query.lastError().text(), {}, {}};
        closeDatabase(&database);
        return result;
    }

    ServiceResult mapped{true, {}, {}, {}};
    while (query.next()) {
        if (productKey == QStringLiteral("oracle")) {
            mapped.rows.append(QJsonObject{
                {QStringLiteral("name"), query.value(0).toString()},
                {QStringLiteral("description"), query.value(0).toString()},
                {QStringLiteral("columns"), QStringLiteral("-")},
                {QStringLiteral("partitioned"), QStringLiteral("no")},
                {QStringLiteral("rows"), query.value(1).toString()}
            });
        } else {
            mapped.rows.append(QJsonObject{
                {QStringLiteral("name"), query.value(0).toString()},
                {QStringLiteral("description"), query.value(0).toString()},
                {QStringLiteral("columns"), query.value(1).toString()},
                {QStringLiteral("partitioned"), QStringLiteral("no")},
                {QStringLiteral("rows"), QStringLiteral("-")}
            });
        }
    }
    closeDatabase(&database);
    return mapped;
}

ServiceResult SqlServiceClient::executeQuery(const ServiceEndpoint &endpoint,
                                             const QString &productKey,
                                             const QString &sql)
{
    QSqlDatabase database;
    const ServiceResult opened = openDatabase(endpoint, productKey, &database);
    if (!opened.ok) {
        return opened;
    }

    QSqlQuery query(database);
    if (!query.exec(sql)) {
        const ServiceResult result{false, query.lastError().text(), {}, {}};
        closeDatabase(&database);
        return result;
    }

    if (query.isSelect()) {
        const ServiceResult result = rowsFromQuery(query);
        closeDatabase(&database);
        return result;
    }

    const ServiceResult result{true,
                               QStringLiteral("影响行数: %1").arg(query.numRowsAffected()),
                               {},
                               {}};
    closeDatabase(&database);
    return result;
}
