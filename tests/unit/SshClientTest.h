#pragma once

#include <QObject>

class SshClientTest final : public QObject
{
    Q_OBJECT

private slots:
    void windowsSshArgsDoNotEnableConnectionSharing();
};
