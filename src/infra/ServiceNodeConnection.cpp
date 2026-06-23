#include "infra/ServiceNodeConnection.h"

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

QJsonObject ServiceNodeConnection::primaryNode(const QJsonObject &instance)
{
    const QJsonArray nodes = instance.value(QStringLiteral("nodes")).toArray();
    if (nodes.isEmpty()) {
        return {};
    }
    return nodes.first().toObject();
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
            endpoint->password = parts.mid(1).join(QLatin1Char(':'));
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
