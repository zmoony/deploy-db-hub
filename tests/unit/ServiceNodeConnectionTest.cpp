#include "ServiceNodeConnectionTest.h"

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
    QCOMPARE(endpoint.installPath, QStringLiteral("/opt/kafka"));
    QCOMPARE(endpoint.storagePath, QStringLiteral("/data/kafka"));
}
