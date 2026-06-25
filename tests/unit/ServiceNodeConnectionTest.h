#pragma once

#include <QObject>

class ServiceNodeConnectionTest final : public QObject
{
    Q_OBJECT

private slots:
    void parseKafkaPort();
    void parseDatabaseInfo();
    void parseElasticsearchInfo();
    void encodeDecodeDirectConnection();
    void resolveUsesDefaultInstallPath();
    void resolveElasticsearchKibanaPort();
    void resolveRedisDatabase();
};
