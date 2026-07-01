#include "adapters/services/ServiceBroker.h"

#include "adapters/services/ElasticsearchServiceClient.h"
#include "adapters/services/KafkaServiceClient.h"
#include "adapters/services/RedisServiceClient.h"
#include "adapters/services/SqlServiceClient.h"

#include <QDateTime>
#include <QHash>
#include <QJsonArray>
#include <QSet>
#include <QStringList>

namespace {

QJsonObject metadataObject(const QJsonObject &instance)
{
    return instance.value(QStringLiteral("metadata")).toObject();
}

qlonglong nonNegative(qlonglong value)
{
    return value < 0 ? 0 : value;
}

QString hostFromNode(const QJsonObject &node, const QString &fallback)
{
    const QString label = node.value(QStringLiteral("serverLabel")).toString();
    const int open = label.indexOf(QLatin1Char('('));
    if (open >= 0) {
        const int colon = label.indexOf(QLatin1Char(':'), open);
        if (colon > open) {
            return label.mid(open + 1, colon - open - 1).trimmed();
        }
    }
    return fallback;
}

QString metadataKeyForTab(ServiceBroker::TabKind tab)
{
    switch (tab) {
    case ServiceBroker::TabKind::Topic:
        return QStringLiteral("topics");
    case ServiceBroker::TabKind::Index:
        return QStringLiteral("indices");
    case ServiceBroker::TabKind::Table:
        return QStringLiteral("tables");
    default:
        return {};
    }
}

void applyRedisDatabaseOverride(ServiceEndpoint *endpoint, const QString &schema)
{
    if (endpoint == nullptr || schema.isEmpty()) {
        return;
    }
    bool ok = false;
    const int db = schema.toInt(&ok);
    if (ok && db >= 0 && db <= 15) {
        endpoint->redisDatabase = db;
    }
}

ServiceResult buildKafkaTopicRows(const QJsonObject &instance, const KafkaTopicDashboard &dashboard)
{
    const bool hasTimeStats = !dashboard.atToday.isEmpty();
    const bool hasOffsets = !dashboard.latest.isEmpty() && !dashboard.earliest.isEmpty();
    const bool hasGroups = !dashboard.groupPartitions.isEmpty();
    QHash<QString, qlonglong> totalByTopic;
    QHash<QString, qlonglong> todayByTopic;
    QHash<QString, qlonglong> yesterdayByTopic;
    QHash<QString, qlonglong> weekByTopic;
    for (auto it = dashboard.latest.constBegin(); it != dashboard.latest.constEnd(); ++it) {
        const QString key = it.key();
        const QString topic = key.section(QLatin1Char('\t'), 0, 0);
        const qlonglong latestOffset = it.value();
        const qlonglong earliestOffset = dashboard.earliest.value(key, 0);
        totalByTopic[topic] += nonNegative(latestOffset - earliestOffset);
        if (hasTimeStats) {
            const qlonglong todayOffset = dashboard.atToday.value(key, earliestOffset);
            const qlonglong yesterdayOffset = dashboard.atYesterday.value(key, earliestOffset);
            const qlonglong weekOffset = dashboard.atWeek.value(key, earliestOffset);
            todayByTopic[topic] += nonNegative(latestOffset - todayOffset);
            yesterdayByTopic[topic] += nonNegative(todayOffset - yesterdayOffset);
            weekByTopic[topic] += nonNegative(latestOffset - weekOffset);
        }
    }

    QHash<QString, QSet<QString>> groupsByTopic;
    if (hasGroups) {
        for (const QJsonObject &row : dashboard.groupPartitions) {
            const QString topic = row.value(QStringLiteral("topic")).toString();
            const QString group = row.value(QStringLiteral("group")).toString();
            if (!topic.isEmpty() && !group.isEmpty()) {
                groupsByTopic[topic].insert(group);
            }
        }
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QJsonObject &topic : dashboard.topics) {
        const QString name = topic.value(QStringLiteral("name")).toString();
        const QString metaKey = QStringLiteral("topics|%1").arg(name);
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), name},
            {QStringLiteral("description"), ServiceBroker::metadataDescription(instance, metaKey)},
            {QStringLiteral("partitions"), topic.value(QStringLiteral("partitions")).toString()},
            {QStringLiteral("replication"), topic.value(QStringLiteral("replication")).toString()},
            {QStringLiteral("groups"), hasGroups ? QString::number(groupsByTopic.value(name).size())
                                                 : QStringLiteral("-")},
            {QStringLiteral("week"), hasTimeStats ? QString::number(weekByTopic.value(name, 0))
                                                  : QStringLiteral("-")},
            {QStringLiteral("yesterday"), hasTimeStats ? QString::number(yesterdayByTopic.value(name, 0))
                                                       : QStringLiteral("-")},
            {QStringLiteral("today"), hasTimeStats ? QString::number(todayByTopic.value(name, 0))
                                                   : QStringLiteral("-")},
            {QStringLiteral("total"), hasOffsets ? QString::number(totalByTopic.value(name, 0))
                                                 : QStringLiteral("-")}
        });
    }
    return rows;
}

ServiceResult buildKafkaTopicRowsQuick(const QJsonObject &instance, const QVector<QJsonObject> &topics)
{
    ServiceResult rows{true, {}, {}, {}};
    for (const QJsonObject &topic : topics) {
        const QString name = topic.value(QStringLiteral("name")).toString();
        const QString metaKey = QStringLiteral("topics|%1").arg(name);
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), name},
            {QStringLiteral("description"), ServiceBroker::metadataDescription(instance, metaKey)},
            {QStringLiteral("partitions"), topic.value(QStringLiteral("partitions")).toString()},
            {QStringLiteral("replication"), topic.value(QStringLiteral("replication")).toString()},
            {QStringLiteral("groups"), QStringLiteral("-")},
            {QStringLiteral("week"), QStringLiteral("-")},
            {QStringLiteral("yesterday"), QStringLiteral("-")},
            {QStringLiteral("today"), QStringLiteral("-")},
            {QStringLiteral("total"), QStringLiteral("-")}
        });
    }
    return rows;
}

}

bool ServiceBroker::resolveContext(const QJsonObject &instance,
                                   const QJsonObject &server,
                                   const QString &productKey,
                                   ServiceEndpoint *endpoint,
                                   QString *error)
{
    return ServiceNodeConnection::resolvePrimaryNode(instance, server, productKey, endpoint, error);
}

ServiceBroker::TabKind ServiceBroker::tabKindFromTitle(const QString &title)
{
    if (title.contains(QStringLiteral("Topic")) || title.contains(QStringLiteral("主题"))) {
        return TabKind::Topic;
    }
    if (title.contains(QStringLiteral("Consumer")) || title.contains(QStringLiteral("消费者"))) {
        return TabKind::ConsumerGroup;
    }
    if (title.contains(QStringLiteral("Node")) || title.contains(QStringLiteral("节点"))) {
        return TabKind::Node;
    }
    if (title.contains(QStringLiteral("Index")) || title.contains(QStringLiteral("索引"))) {
        return TabKind::Index;
    }
    if (title.contains(QStringLiteral("Key"))) {
        return TabKind::Key;
    }
    if (title == QStringLiteral("SQL") || title.contains(QStringLiteral("SQL"))) {
        return TabKind::Sql;
    }
    if (title.contains(QStringLiteral("Table")) || title.contains(QStringLiteral("表管理"))) {
        return TabKind::Table;
    }
    return TabKind::Node;
}

QString ServiceBroker::metadataDescription(const QJsonObject &instance, const QString &key)
{
    const QStringList parts = key.split(QLatin1Char('|'));
    if (parts.size() != 2) {
        return {};
    }
    return metadataObject(instance)
        .value(parts.at(0))
        .toObject()
        .value(parts.at(1))
        .toString();
}

QJsonObject ServiceBroker::setMetadataDescription(QJsonObject instance, const QString &key, const QString &description)
{
    const QStringList parts = key.split(QLatin1Char('|'));
    if (parts.size() != 2) {
        return instance;
    }
    QJsonObject metadata = metadataObject(instance);
    QJsonObject bucket = metadata.value(parts.at(0)).toObject();
    bucket.insert(parts.at(1), description);
    metadata.insert(parts.at(0), bucket);
    instance.insert(QStringLiteral("metadata"), metadata);
    return instance;
}

ServiceResult ServiceBroker::testInstance(const QJsonObject &instance,
                                          const QJsonObject &server,
                                          const QString &productKey,
                                          const RemoteConnectionContext &remote)
{
    ServiceEndpoint endpoint;
    QString error;
    if (!resolveContext(instance, server, productKey, &endpoint, &error)) {
        return {false, error, {}, {}};
    }

    if (productKey == QStringLiteral("kafka")) {
        return KafkaServiceClient::ping(endpoint, remote);
    }
    if (productKey == QStringLiteral("redis")) {
        return RedisServiceClient::ping(endpoint);
    }
    if (productKey == QStringLiteral("elasticsearch")) {
        return ElasticsearchServiceClient::ping(endpoint);
    }
    if (productKey == QStringLiteral("oracle") || productKey == QStringLiteral("postgresql")) {
        return SqlServiceClient::ping(endpoint, productKey);
    }
    return {false, QStringLiteral("未知组件类型"), {}, {}};
}

ServiceResult ServiceBroker::loadTab(const QJsonObject &instance,
                                     const QJsonObject &server,
                                     const QString &productKey,
                                     TabKind tab,
                                     const RemoteConnectionContext &remote,
                                     const QString &schema)
{
    ServiceEndpoint endpoint;
    QString error;
    if (!resolveContext(instance, server, productKey, &endpoint, &error)) {
        return {false, error, {}, {}};
    }
    if (productKey == QStringLiteral("redis")) {
        applyRedisDatabaseOverride(&endpoint, schema);
    }

    if (tab == TabKind::Node && productKey == QStringLiteral("kafka")) {
        ServiceResult rows{true, {}, {}, {}};
        const QJsonArray nodes = instance.value(QStringLiteral("nodes")).toArray();
        for (const QJsonValue &value : nodes) {
            const QJsonObject node = value.toObject();
            ServiceEndpoint nodeEndpoint = endpoint;
            const QString info = node.value(QStringLiteral("info")).toString();
            const int port = info.toInt();
            if (port > 0) {
                nodeEndpoint.port = port;
            }
            nodeEndpoint.installPath = node.value(QStringLiteral("installPath")).toString();
            nodeEndpoint.storagePath = node.value(QStringLiteral("storagePath")).toString();

            const ServiceResult probe =
                KafkaServiceClient::nodeStatus(nodeEndpoint, remote, nodeEndpoint.storagePath);
            const QJsonObject detail = probe.rows.isEmpty() ? QJsonObject{} : probe.rows.first();
            rows.rows.append(QJsonObject{
                {QStringLiteral("ip"), hostFromNode(node, endpoint.host)},
                {QStringLiteral("port"), info},
                {QStringLiteral("status"), probe.ok ? detail.value(QStringLiteral("status")).toString()
                                                    : QStringLiteral("探测失败")},
                {QStringLiteral("dataDir"), probe.ok ? detail.value(QStringLiteral("dataDir")).toString()
                                                     : nodeEndpoint.storagePath},
                {QStringLiteral("disk"), probe.ok ? detail.value(QStringLiteral("diskTotal")).toString()
                                                  : QStringLiteral("-")}
            });
        }
        return rows;
    }

    if (tab == TabKind::Node) {
        ServiceResult rows{true, {}, {}, {}};
        const QJsonArray nodes = instance.value(QStringLiteral("nodes")).toArray();
        for (const QJsonValue &value : nodes) {
            const QJsonObject node = value.toObject();
            if (productKey == QStringLiteral("redis")) {
                ServiceEndpoint nodeEndpoint = endpoint;
                const QString info = node.value(QStringLiteral("info")).toString();
                ServiceNodeConnection::parseInfo(info, productKey, &nodeEndpoint, nullptr);
                const ServiceResult probe = RedisServiceClient::serverInfo(nodeEndpoint);
                const QJsonObject detail = probe.rows.isEmpty() ? QJsonObject{} : probe.rows.first();
                rows.rows.append(QJsonObject{
                    {QStringLiteral("server"), node.value(QStringLiteral("serverLabel")).toString()},
                    {QStringLiteral("info"), info},
                    {QStringLiteral("installPath"), node.value(QStringLiteral("installPath")).toString()},
                    {QStringLiteral("storagePath"), node.value(QStringLiteral("storagePath")).toString()},
                    {QStringLiteral("status"), probe.ok ? detail.value(QStringLiteral("status")).toString()
                                                       : QStringLiteral("探测失败")},
                    {QStringLiteral("version"), probe.ok ? detail.value(QStringLiteral("version")).toString()
                                                         : QStringLiteral("-")},
                    {QStringLiteral("memory"), probe.ok ? detail.value(QStringLiteral("memory")).toString()
                                                        : QStringLiteral("-")}
                });
                continue;
            }
            if (productKey == QStringLiteral("elasticsearch")) {
                ServiceEndpoint nodeEndpoint = endpoint;
                const QString info = node.value(QStringLiteral("info")).toString();
                ServiceNodeConnection::parseInfo(info, productKey, &nodeEndpoint, nullptr);
                const ServiceResult probe = ElasticsearchServiceClient::clusterHealth(nodeEndpoint);
                const QJsonObject detail = probe.rows.isEmpty() ? QJsonObject{} : probe.rows.first();
                rows.rows.append(QJsonObject{
                    {QStringLiteral("server"), node.value(QStringLiteral("serverLabel")).toString()},
                    {QStringLiteral("info"), info},
                    {QStringLiteral("installPath"), node.value(QStringLiteral("installPath")).toString()},
                    {QStringLiteral("storagePath"), node.value(QStringLiteral("storagePath")).toString()},
                    {QStringLiteral("status"), probe.ok ? detail.value(QStringLiteral("status")).toString()
                                                       : QStringLiteral("探测失败")},
                    {QStringLiteral("cluster"), probe.ok ? detail.value(QStringLiteral("cluster")).toString()
                                                         : QStringLiteral("-")}
                });
                continue;
            }
            rows.rows.append(QJsonObject{
                {QStringLiteral("server"), node.value(QStringLiteral("serverLabel")).toString()},
                {QStringLiteral("info"), node.value(QStringLiteral("info")).toString()},
                {QStringLiteral("installPath"), node.value(QStringLiteral("installPath")).toString()},
                {QStringLiteral("storagePath"), node.value(QStringLiteral("storagePath")).toString()}
            });
        }
        return rows;
    }

    if (productKey == QStringLiteral("kafka") && tab == TabKind::Topic) {
        return loadKafkaTopicStats(instance, server, remote);
    }

    if (productKey == QStringLiteral("kafka") && tab == TabKind::ConsumerGroup) {
        const ServiceResult described = KafkaServiceClient::describeConsumerGroups(endpoint, remote);
        if (!described.ok) {
            return described;
        }

        QStringList order;
        QHash<QString, qlonglong> totalByGroup;
        QHash<QString, qlonglong> currentByGroup;
        QHash<QString, qlonglong> lagByGroup;
        for (const QJsonObject &row : described.rows) {
            const QString group = row.value(QStringLiteral("group")).toString();
            if (group.isEmpty()) {
                continue;
            }
            if (!totalByGroup.contains(group)) {
                order.append(group);
            }
            totalByGroup[group] += row.value(QStringLiteral("logEnd")).toString().toLongLong();
            const QString currentText = row.value(QStringLiteral("current")).toString();
            const QString lagText = row.value(QStringLiteral("lag")).toString();
            currentByGroup[group] += currentText == QStringLiteral("-") ? 0 : currentText.toLongLong();
            lagByGroup[group] += lagText == QStringLiteral("-") ? 0 : lagText.toLongLong();
        }

        ServiceResult rows{true, {}, {}, {}};
        for (const QString &group : order) {
            rows.rows.append(QJsonObject{
                {QStringLiteral("group"), group},
                {QStringLiteral("total"), QString::number(totalByGroup.value(group, 0))},
                {QStringLiteral("current"), QString::number(currentByGroup.value(group, 0))},
                {QStringLiteral("lag"), QString::number(lagByGroup.value(group, 0))}
            });
        }
        rows.text = described.text;
        return rows;
    }

    if (productKey == QStringLiteral("redis") && tab == TabKind::Key) {
        return RedisServiceClient::listKeys(endpoint, QStringLiteral("*"));
    }

    if (productKey == QStringLiteral("elasticsearch") && tab == TabKind::Index) {
        const ServiceResult indices = ElasticsearchServiceClient::listIndices(endpoint);
        if (!indices.ok) {
            return indices;
        }
        const QVector<QJsonObject> organized = ElasticsearchServiceClient::organizeIndexRows(indices.rows);
        ServiceResult rows{true, {}, {}, {}};
        for (const QJsonObject &index : organized) {
            const QString name = index.value(QStringLiteral("name")).toString();
            const QString rowKind = index.value(QStringLiteral("rowKind")).toString();
            const QString metaKey = QStringLiteral("indices|%1").arg(name);
            QString description = metadataDescription(instance, metaKey);
            if (rowKind == QStringLiteral("group")) {
                const int childCount = index.value(QStringLiteral("childCount")).toInt();
                if (description.isEmpty()) {
                    description = QStringLiteral("%1 个子索引").arg(childCount);
                }
            }
            QJsonObject row = index;
            row.insert(QStringLiteral("description"), description);
            rows.rows.append(row);
        }
        return rows;
    }

    if ((productKey == QStringLiteral("oracle") || productKey == QStringLiteral("postgresql")) && tab == TabKind::Table) {
        const QString activeSchema = schema.isEmpty() ? (productKey == QStringLiteral("oracle") ? endpoint.username.toUpper()
                                                                                                 : QStringLiteral("public"))
                                                      : schema;
        const ServiceResult tables = SqlServiceClient::listTables(endpoint, productKey, activeSchema);
        if (!tables.ok) {
            return tables;
        }
        ServiceResult rows{true, {}, {}, {}};
        for (const QJsonObject &table : tables.rows) {
            const QString name = table.value(QStringLiteral("name")).toString();
            const QString metaKey = QStringLiteral("tables|%1.%2").arg(activeSchema, name);
            rows.rows.append(QJsonObject{
                {QStringLiteral("name"), name},
                {QStringLiteral("description"), ServiceBroker::metadataDescription(instance, metaKey)},
                {QStringLiteral("columns"), table.value(QStringLiteral("columns")).toString()},
                {QStringLiteral("partitioned"), table.value(QStringLiteral("partitioned")).toString()},
                {QStringLiteral("rows"), table.value(QStringLiteral("rows")).toString()}
            });
        }
        return rows;
    }

    return {false, QStringLiteral("当前 Tab 暂不支持加载"), {}, {}};
}

ServiceResult ServiceBroker::runAction(const QJsonObject &instance,
                                       const QJsonObject &server,
                                       const QString &productKey,
                                       TabKind tab,
                                       const QString &action,
                                       const QJsonObject &selection,
                                       const RemoteConnectionContext &remote,
                                       const QString &schema,
                                       const QString &input)
{
    ServiceEndpoint endpoint;
    QString error;
    if (!resolveContext(instance, server, productKey, &endpoint, &error)) {
        return {false, error, {}, {}};
    }
    if (productKey == QStringLiteral("redis")) {
        applyRedisDatabaseOverride(&endpoint, schema);
    }

    if (action.contains(QStringLiteral("Kibana"))) {
        return {true, ElasticsearchServiceClient::kibanaUrl(endpoint), {}, {}};
    }

    if (productKey == QStringLiteral("kafka") && tab == TabKind::Topic) {
        const QString topic = selection.value(QStringLiteral("name")).toString();
        if (action.contains(QStringLiteral("添加"))) {
            const QStringList parts = input.split(QLatin1Char(','));
            return KafkaServiceClient::createTopic(endpoint,
                                                 remote,
                                                 parts.value(0).trimmed(),
                                                 parts.value(1).trimmed().toInt() > 0 ? parts.value(1).trimmed().toInt() : 1,
                                                 parts.value(2).trimmed().toInt() > 0 ? parts.value(2).trimmed().toInt() : 1);
        }
        if (action.contains(QStringLiteral("删除"))) {
            return KafkaServiceClient::deleteTopic(endpoint, remote, topic);
        }
        if (action.contains(QStringLiteral("写入"))) {
            return KafkaServiceClient::produce(endpoint, remote, topic, input);
        }
        if (action.contains(QStringLiteral("最新数据"))) {
            return KafkaServiceClient::consumeLatest(endpoint, remote, topic);
        }
        if (action.contains(QStringLiteral("导出"))) {
            return KafkaServiceClient::listTopics(endpoint, remote);
        }
    }

    if (productKey == QStringLiteral("kafka") && tab == TabKind::Node) {
        return KafkaServiceClient::viewConfig(endpoint, remote);
    }

    if (productKey == QStringLiteral("kafka") && tab == TabKind::ConsumerGroup) {
        const QString group = selection.value(QStringLiteral("group")).toString();
        const ServiceResult described = KafkaServiceClient::describeConsumerGroups(endpoint, remote);
        if (!described.ok) {
            return described;
        }
        ServiceResult rows{true, {}, {}, {}};
        for (const QJsonObject &row : described.rows) {
            if (group.isEmpty() || row.value(QStringLiteral("group")).toString() == group) {
                rows.rows.append(row);
            }
        }
        return rows;
    }

    if (productKey == QStringLiteral("redis") && tab == TabKind::Key) {
        if (action.contains(QStringLiteral("搜索")) || action.contains(QStringLiteral("刷新"))) {
            QString pattern = input;
            QString typeFilter;
            const int sepIndex = input.indexOf(QLatin1Char('\t'));
            if (sepIndex >= 0) {
                pattern = input.left(sepIndex);
                typeFilter = input.mid(sepIndex + 1);
            }
            if (pattern.isEmpty()) {
                pattern = QStringLiteral("*");
            }
            return RedisServiceClient::listKeys(endpoint, pattern, typeFilter);
        }
        if (action.contains(QStringLiteral("写入"))) {
            const QString key = selection.value(QStringLiteral("key")).toString();
            if (key.isEmpty() && input.contains(QLatin1Char('='))) {
                const QStringList parts = input.split(QLatin1Char('='));
                return RedisServiceClient::writeKey(endpoint, parts.value(0).trimmed(), parts.mid(1).join(QLatin1Char('=')).trimmed());
            }
            return RedisServiceClient::writeKey(endpoint, key.isEmpty() ? input.section(QLatin1Char('='), 0, 0).trimmed() : key,
                                                input.section(QLatin1Char('='), 1).trimmed());
        }
        if (action.contains(QStringLiteral("删除"))) {
            return RedisServiceClient::deleteKey(endpoint, selection.value(QStringLiteral("key")).toString());
        }
        return RedisServiceClient::readKey(endpoint, selection.value(QStringLiteral("key")).toString());
    }

    if (productKey == QStringLiteral("elasticsearch") && tab == TabKind::Index) {
        const QString index = selection.value(QStringLiteral("queryIndex")).toString().isEmpty()
            ? selection.value(QStringLiteral("name")).toString()
            : selection.value(QStringLiteral("queryIndex")).toString();
        if (action.contains(QStringLiteral("快速查询")) || action.contains(QStringLiteral("查询"))
            || action.contains(QStringLiteral("最新"))) {
            const int from = selection.value(QStringLiteral("from")).toInt(0);
            const int size = selection.value(QStringLiteral("size")).toInt(10);
            return ElasticsearchServiceClient::searchIndex(endpoint, index, from, size, input);
        }
        if (action.contains(QStringLiteral("导出"))) {
            return ElasticsearchServiceClient::listIndices(endpoint);
        }
        if (action.contains(QStringLiteral("统计"))) {
            return ElasticsearchServiceClient::indexCount(endpoint, index);
        }
    }

    if ((productKey == QStringLiteral("oracle") || productKey == QStringLiteral("postgresql")) && tab == TabKind::Table) {
        const QString table = selection.value(QStringLiteral("name")).toString();
        const QString activeSchema = schema.isEmpty() ? (productKey == QStringLiteral("oracle") ? endpoint.username.toUpper()
                                                                                                 : QStringLiteral("public"))
                                                      : schema;
        if (action == QStringLiteral("SQL") || action.contains(QStringLiteral("查询"))) {
            const QString sql = input.isEmpty()
                ? SqlServiceClient::defaultSelectSql(productKey, activeSchema, table)
                : input;
            return SqlServiceClient::executeQuery(endpoint, productKey, sql);
        }
        if (action.contains(QStringLiteral("样例")) || action.contains(QStringLiteral("最新"))) {
            return SqlServiceClient::sampleRows(endpoint, productKey, activeSchema, table);
        }
        if (action.contains(QStringLiteral("导出"))) {
            return SqlServiceClient::listTables(endpoint, productKey, activeSchema);
        }
    }

    if (action == QStringLiteral("SQL") && !input.isEmpty()) {
        return SqlServiceClient::executeQuery(endpoint, productKey, input);
    }

    return {false, QStringLiteral("不支持的操作: %1").arg(action), {}, {}};
}

ServiceResult ServiceBroker::loadKafkaTopicsQuick(const QJsonObject &instance,
                                                  const QJsonObject &server,
                                                  const RemoteConnectionContext &remote)
{
    ServiceEndpoint endpoint;
    QString error;
    if (!resolveContext(instance, server, QStringLiteral("kafka"), &endpoint, &error)) {
        return {false, error, {}, {}};
    }

    const ServiceResult described = KafkaServiceClient::describeAllTopics(endpoint, remote);
    if (!described.ok) {
        return described;
    }
    return buildKafkaTopicRowsQuick(instance, described.rows);
}

ServiceResult ServiceBroker::loadKafkaTopicStats(const QJsonObject &instance,
                                                 const QJsonObject &server,
                                                 const RemoteConnectionContext &remote,
                                                 const QVector<QJsonObject> &knownTopics)
{
    ServiceEndpoint endpoint;
    QString error;
    if (!resolveContext(instance, server, QStringLiteral("kafka"), &endpoint, &error)) {
        return {false, error, {}, {}};
    }

    const qint64 todayStart = QDate::currentDate().startOfDay().toMSecsSinceEpoch();
    const qint64 dayMs = 24LL * 60LL * 60LL * 1000LL;
    const qint64 yesterdayStart = todayStart - dayMs;
    const qint64 weekStart = todayStart - 6LL * dayMs;

    const KafkaTopicDashboard dashboard =
        KafkaServiceClient::loadTopicDashboard(endpoint, remote, todayStart, yesterdayStart, weekStart, knownTopics);
    if (!dashboard.ok) {
        return {false, dashboard.message, {}, {}};
    }
    return buildKafkaTopicRows(instance, dashboard);
}
