#pragma once

#include <QObject>

class LogSanitizerTest final : public QObject
{
    Q_OBJECT

private slots:
    void masksPasswordTokenAndAuthorization();
    void masksPrivateKeyBlock();
};
