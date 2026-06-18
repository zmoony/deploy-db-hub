#pragma once

#include <QJsonObject>
#include <QString>

#include <functional>

struct RemoteConnectionContext {
    QJsonObject serverConfig;
    QString password;
    QString keyPassphrase;
};

using HostKeyPromptHandler = std::function<bool(const QString &fingerprintSha256, QString *error)>;
