#include "infra/ServiceDefaults.h"

namespace ServiceDefaults {

QString installPath(const QString &productKey)
{
    if (productKey == QStringLiteral("kafka")) {
        return QStringLiteral("/usr/local/wiscom/kafka_2.13-2.7.0");
    }
    if (productKey == QStringLiteral("redis")) {
        return QStringLiteral("/usr/local/redis");
    }
    if (productKey == QStringLiteral("elasticsearch")) {
        return QStringLiteral("/usr/share/elasticsearch");
    }
    if (productKey == QStringLiteral("postgresql")) {
        return QStringLiteral("/usr/pgsql");
    }
    if (productKey == QStringLiteral("oracle")) {
        return QStringLiteral("/u01/app/oracle");
    }
    return QString();
}

QString storagePath(const QString &productKey)
{
    if (productKey == QStringLiteral("kafka")) {
        return QStringLiteral("/data/kafka");
    }
    if (productKey == QStringLiteral("redis")) {
        return QStringLiteral("/var/lib/redis");
    }
    if (productKey == QStringLiteral("elasticsearch")) {
        return QStringLiteral("/var/lib/elasticsearch");
    }
    if (productKey == QStringLiteral("postgresql")) {
        return QStringLiteral("/var/lib/pgsql");
    }
    if (productKey == QStringLiteral("oracle")) {
        return QStringLiteral("/u01/oradata");
    }
    return QString();
}

QString defaultInfo(const QString &productKey)
{
    if (productKey == QStringLiteral("kafka")) {
        return QStringLiteral("18103");
    }
    if (productKey == QStringLiteral("redis")) {
        return QStringLiteral("6379");
    }
    if (productKey == QStringLiteral("elasticsearch")) {
        return QStringLiteral("9200");
    }
    return QString();
}

int kibanaPort(const QString &productKey)
{
    if (productKey == QStringLiteral("elasticsearch")) {
        return 5601;
    }
    return 0;
}

}
