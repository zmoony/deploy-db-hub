#pragma once

#include <QObject>

class AiSettingsStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsSettings();
    void migratesLegacySingleConnection();
    void persistsMultipleConnections();
    void buildCredentialRef_isStable();
    void defaultModelPresets_returnsNonEmpty();
};
