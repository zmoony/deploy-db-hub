#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "infra/ServiceNodeConnection.h"

#include "adapters/services/RedisServiceClient.h"

#include <QJsonObject>
#include <QString>

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
