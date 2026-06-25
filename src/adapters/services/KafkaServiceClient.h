#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "infra/ServiceNodeConnection.h"

#include "adapters/services/RedisServiceClient.h"

#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVector>

struct KafkaTopicDashboard {
    bool ok = false;
    QString message;
    QVector<QJsonObject> topics;
    QVector<QJsonObject> groupPartitions;
    QHash<QString, qlonglong> latest;
    QHash<QString, qlonglong> earliest;
    QHash<QString, qlonglong> atToday;
    QHash<QString, qlonglong> atYesterday;
    QHash<QString, qlonglong> atWeek;
};

class KafkaServiceClient final
{
public:
    static ServiceResult ping(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote);
    static ServiceResult listTopics(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote);
    static ServiceResult describeTopic(const ServiceEndpoint &endpoint,
                                       const RemoteConnectionContext &remote,
                                       const QString &topic);
    static ServiceResult listConsumerGroups(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote);

    // Describe every topic in one call. Rows: {name, partitions, replication}.
    static ServiceResult describeAllTopics(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote);
    // Per-partition offsets at a point in time (timeMs: -1 latest, -2 earliest, otherwise epoch ms).
    // Rows: {topic, partition, offset}.
    static ServiceResult topicOffsets(const ServiceEndpoint &endpoint,
                                      const RemoteConnectionContext &remote,
                                      qint64 timeMs);
    // One SSH round-trip: describe topics, offset snapshots, and consumer groups.
    // If knownTopics is non-empty, the describe step is skipped (reuses quick-path data).
    static KafkaTopicDashboard loadTopicDashboard(const ServiceEndpoint &endpoint,
                                                 const RemoteConnectionContext &remote,
                                                 qint64 todayStartMs,
                                                 qint64 yesterdayStartMs,
                                                 qint64 weekStartMs,
                                                 const QVector<QJsonObject> &knownTopics = {});
    static QVector<QJsonObject> parseOffsetOutputLines(const QString &text);
    static QVector<QJsonObject> parseDescribeTopicsOutput(const QString &text);
    static QVector<QJsonObject> parseConsumerGroupRows(const QString &text);
    // Describe every consumer group in one call. Rows: {group, topic, partition, current, logEnd, lag}.
    static ServiceResult describeConsumerGroups(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote);
    // Produce a single message to a topic via kafka-console-producer.
    static ServiceResult produce(const ServiceEndpoint &endpoint,
                                 const RemoteConnectionContext &remote,
                                 const QString &topic,
                                 const QString &message);
    // Probe node runtime status. Rows: {status, dataDir, diskTotal}.
    static ServiceResult nodeStatus(const ServiceEndpoint &endpoint,
                                    const RemoteConnectionContext &remote,
                                    const QString &storagePath);
    // Read server.properties content.
    static ServiceResult viewConfig(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote);
    static ServiceResult createTopic(const ServiceEndpoint &endpoint,
                                     const RemoteConnectionContext &remote,
                                     const QString &topic,
                                     int partitions,
                                     int replication);
    static ServiceResult deleteTopic(const ServiceEndpoint &endpoint,
                                     const RemoteConnectionContext &remote,
                                     const QString &topic);
    static ServiceResult consumeLatest(const ServiceEndpoint &endpoint,
                                       const RemoteConnectionContext &remote,
                                       const QString &topic,
                                       int maxMessages = 5);
};
