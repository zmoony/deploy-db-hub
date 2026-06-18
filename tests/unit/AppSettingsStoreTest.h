#pragma once

#include <QObject>

class AppSettingsStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsSettings();
};
