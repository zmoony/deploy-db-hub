#include "ServiceNodeConnectionTest.h"

#include "adapters/services/ElasticsearchServiceClient.h"
#include "infra/ServiceDefaults.h"
#include "infra/ServiceNodeConnection.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

void ServiceNodeConnectionTest::parseKafkaPort()
{
    ServiceEndpoint endpoint;
    QString error;
    QVERIFY(ServiceNodeConnection::parseInfo(QStringLiteral("18103"), QStringLiteral("kafka"), &endpoint, &error));
    QCOMPARE(endpoint.port, 18103);
}

void ServiceNodeConnectionTest::parseDatabaseInfo()
{
    ServiceEndpoint endpoint;
    QString error;
    QVERIFY(ServiceNodeConnection::parseInfo(QStringLiteral("appdb:5432:postgres:secret"),
                                             QStringLiteral("postgresql"),
                                             &endpoint,
                                             &error));
    QCOMPARE(endpoint.database, QStringLiteral("appdb"));
    QCOMPARE(endpoint.port, 5432);
    QCOMPARE(endpoint.username, QStringLiteral("postgres"));
    QCOMPARE(endpoint.password, QStringLiteral("secret"));
}

void ServiceNodeConnectionTest::parseElasticsearchInfo()
{
    ServiceEndpoint endpoint;
    QString error;
    QVERIFY(ServiceNodeConnection::parseInfo(QStringLiteral("9200:elastic:changeme"),
                                             QStringLiteral("elasticsearch"),
                                             &endpoint,
                                             &error));
    QCOMPARE(endpoint.port, 9200);
    QCOMPARE(endpoint.username, QStringLiteral("elastic"));
    QCOMPARE(endpoint.password, QStringLiteral("changeme"));
}

void ServiceNodeConnectionTest::encodeDecodeDirectConnection()
{
    ServiceConnectionFields fields;
    fields.port = 6379;
    fields.password = QStringLiteral("secret");
    fields.username = QStringLiteral("default");

    const QString encoded = ServiceNodeConnection::encodeInfo(fields, QStringLiteral("redis"));
    QCOMPARE(encoded, QStringLiteral("6379:secret:default"));

    ServiceConnectionFields decoded;
    QVERIFY(ServiceNodeConnection::decodeInfo(encoded, QStringLiteral("redis"), &decoded));
    QCOMPARE(decoded.port, 6379);
    QCOMPARE(decoded.password, QStringLiteral("secret"));
    QCOMPARE(decoded.username, QStringLiteral("default"));
    QVERIFY(ServiceNodeConnection::usesDirectConnect(QStringLiteral("redis")));
    QVERIFY(!ServiceNodeConnection::usesDirectConnect(QStringLiteral("kafka")));
}

void ServiceNodeConnectionTest::resolveUsesDefaultInstallPath()
{
    QJsonObject instance{
        {QStringLiteral("nodes"),
         QJsonArray{QJsonObject{
             {QStringLiteral("serverId"), QStringLiteral("s1")},
             {QStringLiteral("info"), QStringLiteral("18103")}
         }}}
    };
    QJsonObject server{{QStringLiteral("host"), QStringLiteral("192.168.1.10")}};

    ServiceEndpoint endpoint;
    QString error;
    QVERIFY(ServiceNodeConnection::resolvePrimaryNode(instance, server, QStringLiteral("kafka"), &endpoint, &error));
    QCOMPARE(endpoint.installPath, ServiceDefaults::installPath(QStringLiteral("kafka")));
    QCOMPARE(endpoint.storagePath, ServiceDefaults::storagePath(QStringLiteral("kafka")));
}

void ServiceNodeConnectionTest::resolveElasticsearchKibanaPort()
{
    QJsonObject instance{
        {QStringLiteral("nodes"),
         QJsonArray{QJsonObject{
             {QStringLiteral("serverId"), QStringLiteral("s1")},
             {QStringLiteral("info"), QStringLiteral("9200")}
         }}}
    };
    QJsonObject server{{QStringLiteral("host"), QStringLiteral("192.168.1.20")}};

    ServiceEndpoint endpoint;
    QString error;
    QVERIFY(ServiceNodeConnection::resolvePrimaryNode(instance, server, QStringLiteral("elasticsearch"), &endpoint, &error));
    QCOMPARE(endpoint.kibanaPort, 5601);
    QCOMPARE(ElasticsearchServiceClient::kibanaUrl(endpoint), QStringLiteral("http://192.168.1.20:5601"));

    QJsonArray nodes = instance.value(QStringLiteral("nodes")).toArray();
    QJsonObject node = nodes.first().toObject();
    node.insert(QStringLiteral("kibanaPort"), 15601);
    nodes.replace(0, node);
    instance.insert(QStringLiteral("nodes"), nodes);
    endpoint = {};
    QVERIFY(ServiceNodeConnection::resolvePrimaryNode(instance, server, QStringLiteral("elasticsearch"), &endpoint, &error));
    QCOMPARE(endpoint.kibanaPort, 15601);
    QCOMPARE(ElasticsearchServiceClient::kibanaUrl(endpoint), QStringLiteral("http://192.168.1.20:15601"));
}

void ServiceNodeConnectionTest::resolveRedisDatabase()
{
    QJsonObject instance{
        {QStringLiteral("nodes"),
         QJsonArray{QJsonObject{
             {QStringLiteral("customHost"), QStringLiteral("172.18.12.66")},
             {QStringLiteral("serverLabel"), QStringLiteral("172.18.12.66:6379")},
             {QStringLiteral("info"), QStringLiteral("6379:secret")},
             {QStringLiteral("redisDb"), 6}
         }}}
    };
    QJsonObject server{{QStringLiteral("host"), QStringLiteral("172.18.12.66")}};

    ServiceEndpoint endpoint;
    QString error;
    QVERIFY(ServiceNodeConnection::resolvePrimaryNode(instance, server, QStringLiteral("redis"), &endpoint, &error));
    QCOMPARE(endpoint.port, 6379);
    QCOMPARE(endpoint.password, QStringLiteral("secret"));
    QCOMPARE(endpoint.redisDatabase, 6);
}
