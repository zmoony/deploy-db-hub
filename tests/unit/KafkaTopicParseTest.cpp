#include "KafkaTopicParseTest.h"

#include "adapters/services/KafkaServiceClient.h"

#include <QTest>

void KafkaTopicParseTest::parseOffsetLinesSupportsColonFormat()
{
    const QVector<QJsonObject> rows = KafkaServiceClient::parseOffsetOutputLines(
        QStringLiteral("wiscom.face120:0:601302\nwiscom.face120 1 1200"));
    QCOMPARE(rows.size(), 2);
    QCOMPARE(rows.at(0).value(QStringLiteral("topic")).toString(), QStringLiteral("wiscom.face120"));
    QCOMPARE(rows.at(0).value(QStringLiteral("partition")).toString(), QStringLiteral("0"));
    QCOMPARE(rows.at(0).value(QStringLiteral("offset")).toString(), QStringLiteral("601302"));
    QCOMPARE(rows.at(1).value(QStringLiteral("offset")).toString(), QStringLiteral("1200"));
}

void KafkaTopicParseTest::parseOffsetLinesSkipsNegativeOffsets()
{
    const QVector<QJsonObject> rows =
        KafkaServiceClient::parseOffsetOutputLines(QStringLiteral("demo:0:-1\ndemo:1:42"));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.at(0).value(QStringLiteral("partition")).toString(), QStringLiteral("1"));
}

void KafkaTopicParseTest::parseDescribeTopicsOutputExtractsSummary()
{
    const QVector<QJsonObject> rows = KafkaServiceClient::parseDescribeTopicsOutput(
        QStringLiteral("Topic: wiscom.face120\tTopicId: abc\n"
                       "Topic: wiscom.face120\tPartitionCount: 10\tReplicationFactor: 1"));
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.at(0).value(QStringLiteral("name")).toString(), QStringLiteral("wiscom.face120"));
    QCOMPARE(rows.at(0).value(QStringLiteral("partitions")).toString(), QStringLiteral("10"));
    QCOMPARE(rows.at(0).value(QStringLiteral("replication")).toString(), QStringLiteral("1"));
}
