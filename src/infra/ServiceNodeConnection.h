#pragma once

#include <QJsonObject>
#include <QString>

class ConfigStore;

struct ServiceEndpoint {
    QString host;
    int port = 0;
    QString database;
    QString username;
    QString password;
    QString authFingerprint;
    QString installPath;
    QString storagePath;
    QString serverId;
    int kibanaPort = 0;
    int redisDatabase = 0;
    bool useRemoteHost = true;
};

struct ServiceConnectionFields {
    int port = 0;
    QString username;
    QString password;
    QString database;
    QString authFingerprint;
};

struct ServiceNodeConnection final
{
    static bool usesDirectConnect(const QString &productKey);
    static bool decodeInfo(const QString &info, const QString &productKey, ServiceConnectionFields *fields, QString *error = nullptr);
    static QString encodeInfo(const ServiceConnectionFields &fields, const QString &productKey);
    static bool parseInfo(const QString &info, const QString &productKey, ServiceEndpoint *endpoint, QString *error);
    static bool resolvePrimaryNode(const QJsonObject &instance,
                                   const QJsonObject &server,
                                   const QString &productKey,
                                   ServiceEndpoint *endpoint,
                                   QString *error);
    static QJsonObject primaryNode(const QJsonObject &instance);
    static QString hostFromNodeLabel(const QJsonObject &node);
    static bool resolveServerForNode(const QJsonObject &node,
                                     ConfigStore *store,
                                     QJsonObject *server,
                                     QString *error = nullptr);
};
