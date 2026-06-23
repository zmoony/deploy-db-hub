#pragma once

#include <QJsonObject>
#include <QString>

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
    bool useRemoteHost = true;
};

struct ServiceNodeConnection final
{
    static bool parseInfo(const QString &info, const QString &productKey, ServiceEndpoint *endpoint, QString *error);
    static bool resolvePrimaryNode(const QJsonObject &instance,
                                   const QJsonObject &server,
                                   const QString &productKey,
                                   ServiceEndpoint *endpoint,
                                   QString *error);
    static QJsonObject primaryNode(const QJsonObject &instance);
};
