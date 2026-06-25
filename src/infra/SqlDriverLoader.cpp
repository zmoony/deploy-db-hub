#include "infra/SqlDriverLoader.h"

#include "adapters/services/JdbcSqlBridge.h"

void SqlDriverLoader::applySettings(const AppSettings &)
{
}

void SqlDriverLoader::applyFromStore()
{
}

SqlDriverProbeResult SqlDriverLoader::probe()
{
    return JdbcSqlBridge::probe();
}
