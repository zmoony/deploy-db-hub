#pragma once

#include "infra/AppSettingsStore.h"

#include <QString>

struct SqlDriverProbeResult {
    bool postgresAvailable = false;
    bool oracleAvailable = false;
    QString message;
};

class SqlDriverLoader final
{
public:
    static void applySettings(const AppSettings &settings);
    static void applyFromStore();
    static SqlDriverProbeResult probe();
};
