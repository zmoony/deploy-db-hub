#pragma once

#include <QObject>

class AiSettingsStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsSettings();
};
