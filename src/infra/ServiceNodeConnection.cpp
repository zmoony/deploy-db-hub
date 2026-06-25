#include "infra/ServiceNodeConnection.h"

#include "infra/ConfigStore.h"
#include "infra/ServiceDefaults.h"

#include <QJsonArray>

namespace {

int defaultPortForProduct(const QString &productKey)
{
    if (productKey == QStringLiteral("kafka")) {
        return 18103;
    }
    if (productKey == QStringLiteral("redis")) {
        return 6379;
    }
    if (productKey == QStringLiteral("elasticsearch")) {
        return 9200;
    }
    if (productKey == QStringLiteral("oracle")) {
        return 1521;
    }
    if (productKey == QStringLiteral("postgresql")) {
        return 5432;
    }
    return 0;
}

}

bool ServiceNodeConnection::usesDirectConnect(const QString &productKey)
{
    return productKey != QStringLiteral("kafka");
}

bool ServiceNodeConnection::decodeInfo(const QString &info,
                                       const QString &productKey,
                                       ServiceConnectionFields *fields,
                                       QString *error)
{
    if (fields == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("fields output is null");
        }
        return false;
    }

    *fields = {};
    ServiceEndpoint endpoint;
    if (!parseInfo(info, productKey, &endpoint, error)) {
        return false;
    }

    fields->port = endpoint.port;
    fields->username = endpoint.username;
    fields->password = endpoint.password;
    fields->database = endpoint.database;
    fields->authFingerprint = endpoint.authFingerprint;
    return true;
}

QString ServiceNodeConnection::encodeInfo(const ServiceConnectionFields &fields, const QString &productKey)
{
    if (productKey == QStringLiteral("kafka")) {
        return fields.port > 0 ? QString::number(fields.port) : QString();
    }

    if (productKey == QStringLiteral("redis")) {
        if (fields.port <= 0) {
            return QString();
        }
        if (fields.password.isEmpty() && fields.username.isEmpty()) {
            return QString::number(fields.port);
        }
        if (fields.username.isEmpty()) {
            return QStringLiteral("%1:%2").arg(fields.port).arg(fields.password);
        }
        return QStringLiteral("%1:%2:%3").arg(fields.port).arg(fields.password, fields.username);
    }

    if (productKey == QStringLiteral("elasticsearch")) {
        if (fields.port <= 0) {
            return QString();
        }
        QString encoded = QString::number(fields.port);
        if (!fields.username.isEmpty()) {
            encoded += QLatin1Char(':') + fields.username;
            if (!fields.password.isEmpty()) {
                encoded += QLatin1Char(':') + fields.password;
                if (!fields.authFingerprint.isEmpty()) {
                    encoded += QLatin1Char(':') + fields.authFingerprint;
                }
            }
        }
        return encoded;
    }

    if (productKey == QStringLiteral("oracle") || productKey == QStringLiteral("postgresql")) {
        return QStringLiteral("%1:%2:%3:%4")
            .arg(fields.database)
            .arg(fields.port)
            .arg(fields.username)
            .arg(fields.password);
    }

    return {};
}

QJsonObject ServiceNodeConnection::primaryNode(const QJsonObject &instance)
{
    const QJsonArray nodes = instance.value(QStringLiteral("nodes")).toArray();
    if (nodes.isEmpty()) {
        return {};
    }
    return nodes.first().toObject();
}

QString ServiceNodeConnection::hostFromNodeLabel(const QJsonObject &node)
{
    const QString customHost = node.value(QStringLiteral("customHost")).toString().trimmed();
    if (!customHost.isEmpty()) {
        return customHost;
    }

    const QString label = node.value(QStringLiteral("serverLabel")).toString();
    const int open = label.indexOf(QLatin1Char('('));
    if (open >= 0) {
        const int colon = label.indexOf(QLatin1Char(':'), open);
        if (colon > open) {
            return label.mid(open + 1, colon - open - 1).trimmed();
        }
    }
    if (label.contains(QLatin1Char('.')) || label.contains(QLatin1Char(':'))) {
        return label.section(QLatin1Char(' '), 0, 0).trimmed();
    }
    return label.trimmed();
}

bool ServiceNodeConnection::resolveServerForNode(const QJsonObject &node,
                                                 ConfigStore *store,
                                                 QJsonObject *server,
                                                 QString *error)
{
    if (server == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("server output is null");
        }
        return false;
    }

    const QString serverId = node.value(QStringLiteral("serverId")).toString();
    if (!serverId.isEmpty() && store != nullptr) {
        return store->getServer(serverId, server, error);
    }

    const QString host = hostFromNodeLabel(node);
    if (host.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请选择服务器或输入 IP/主机名");
        }
        return false;
    }

    *server = QJsonObject{
        {QStringLiteral("id"), QString()},
        {QStringLiteral("name"), host},
        {QStringLiteral("host"), host},
        {QStringLiteral("port"), 22},
        {QStringLiteral("username"), QStringLiteral("root")},
        {QStringLiteral("os"), QStringLiteral("linux")},
        {QStringLiteral("auth"), QJsonObject{{QStringLiteral("mode"), QStringLiteral("password")}}}
    };
    return true;
}

bool ServiceNodeConnection::parseInfo(const QString &info,
                                      const QString &productKey,
                                      ServiceEndpoint *endpoint,
                                      QString *error)
{
    if (endpoint == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("endpoint output is null");
        }
        return false;
    }

    const QString trimmed = info.trimmed();
    if (trimmed.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("节点信息不能为空");
        }
        return false;
    }

    const QStringList parts = trimmed.split(QLatin1Char(':'));

    if (productKey == QStringLiteral("kafka")) {
        endpoint->port = trimmed.toInt();
        if (endpoint->port <= 0) {
            endpoint->port = defaultPortForProduct(productKey);
        }
        return true;
    }

    if (productKey == QStringLiteral("redis")) {
        endpoint->port = parts.value(0).toInt();
        if (endpoint->port <= 0) {
            endpoint->port = defaultPortForProduct(productKey);
        }
        if (parts.size() >= 2) {
            endpoint->password = parts.at(1);
        }
        if (parts.size() >= 3) {
            endpoint->username = parts.at(2);
        }
        return true;
    }

    if (productKey == QStringLiteral("elasticsearch")) {
        endpoint->port = parts.value(0).toInt();
        if (endpoint->port <= 0) {
            endpoint->port = defaultPortForProduct(productKey);
        }
        if (parts.size() >= 2) {
            endpoint->username = parts.at(1);
        }
        if (parts.size() >= 3) {
            endpoint->password = parts.at(2);
        }
        if (parts.size() >= 4) {
            endpoint->authFingerprint = parts.mid(3).join(QLatin1Char(':'));
        }
        return true;
    }

    if (productKey == QStringLiteral("oracle") || productKey == QStringLiteral("postgresql")) {
        if (parts.size() < 4) {
            if (error != nullptr) {
                *error = QStringLiteral("数据库信息格式应为 db_name:port:username:password");
            }
            return false;
        }
        endpoint->database = parts.at(0);
        endpoint->port = parts.at(1).toInt();
        endpoint->username = parts.at(2);
        endpoint->password = parts.mid(3).join(QLatin1Char(':'));
        if (endpoint->port <= 0) {
            endpoint->port = defaultPortForProduct(productKey);
        }
        return true;
    }

    if (error != nullptr) {
        *error = QStringLiteral("未知组件类型: %1").arg(productKey);
    }
    return false;
}

bool ServiceNodeConnection::resolvePrimaryNode(const QJsonObject &instance,
                                               const QJsonObject &server,
                                               const QString &productKey,
                                               ServiceEndpoint *endpoint,
                                               QString *error)
{
    const QJsonObject node = primaryNode(instance);
    if (node.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("实例未配置节点");
        }
        return false;
    }

    if (!parseInfo(node.value(QStringLiteral("info")).toString(), productKey, endpoint, error)) {
        return false;
    }

    endpoint->installPath = node.value(QStringLiteral("installPath")).toString().trimmed();
    if (endpoint->installPath.isEmpty()) {
        endpoint->installPath = ServiceDefaults::installPath(productKey);
    }
    endpoint->storagePath = node.value(QStringLiteral("storagePath")).toString().trimmed();
    if (endpoint->storagePath.isEmpty()) {
        endpoint->storagePath = ServiceDefaults::storagePath(productKey);
    }
    endpoint->serverId = node.value(QStringLiteral("serverId")).toString();

    if (productKey == QStringLiteral("elasticsearch")) {
        const int configured = node.value(QStringLiteral("kibanaPort")).toInt(0);
        endpoint->kibanaPort = configured > 0 ? configured : ServiceDefaults::kibanaPort(productKey);
    }

    if (productKey == QStringLiteral("redis")) {
        const int configuredDb = node.value(QStringLiteral("redisDb")).toInt(0);
        endpoint->redisDatabase = configuredDb >= 0 && configuredDb <= 15 ? configuredDb : 0;
    }

    if (!server.isEmpty()) {
        endpoint->host = server.value(QStringLiteral("host")).toString().trimmed();
    }
    if (endpoint->host.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("无法解析节点主机地址");
        }
        return false;
    }
    return true;
}
