#include "adapters/services/KafkaServiceClient.h"

#include "adapters/remote/RemoteExecutor.h"

#include <QDir>
#include <QJsonDocument>
#include <QRegularExpression>

namespace {

QString shellQuote(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'") + escaped + QLatin1Char('\'');
}

QString bootstrapServer(const ServiceEndpoint &endpoint)
{
    return QStringLiteral("127.0.0.1:%1").arg(endpoint.port);
}

QString kafkaScript(const ServiceEndpoint &endpoint, const QString &scriptName)
{
    if (endpoint.installPath.isEmpty()) {
        return scriptName;
    }
    return QDir(endpoint.installPath).filePath(QStringLiteral("bin/%1").arg(scriptName));
}

ServiceResult runRemoteKafka(const ServiceEndpoint &endpoint,
                             const RemoteConnectionContext &remote,
                             const QStringList &args,
                             int timeoutSec = 30)
{
    if (endpoint.installPath.isEmpty()) {
        return {false, QStringLiteral("Kafka 节点未配置安装路径"), {}, {}};
    }
    if (remote.serverConfig.value(QStringLiteral("os")).toString() != QStringLiteral("linux")) {
        return {false, QStringLiteral("Kafka 远程运维当前仅支持 Linux SSH 节点"), {}, {}};
    }

    auto executor = createRemoteExecutor(remote);
    if (!executor) {
        return {false, QStringLiteral("无法创建 SSH 执行器"), {}, {}};
    }

    const QString command = QStringLiteral("bash -lc %1")
                                .arg(shellQuote(args.join(QLatin1Char(' '))));
    const RemoteCommandResult commandResult = executor->execute(command, timeoutSec);
    if (!commandResult.ok) {
        return {false, commandResult.error.isEmpty() ? commandResult.stderrText : commandResult.error, {}, {}};
    }
    return {true, commandResult.stdoutText.trimmed(), {}, {}};
}

ServiceResult runRemoteCommand(const ServiceEndpoint &endpoint,
                               const RemoteConnectionContext &remote,
                               const QString &innerCommand,
                               int timeoutSec = 30)
{
    if (endpoint.installPath.isEmpty()) {
        return {false, QStringLiteral("Kafka 节点未配置安装路径"), {}, {}};
    }
    if (remote.serverConfig.value(QStringLiteral("os")).toString() != QStringLiteral("linux")) {
        return {false, QStringLiteral("Kafka 远程运维当前仅支持 Linux SSH 节点"), {}, {}};
    }

    auto executor = createRemoteExecutor(remote);
    if (!executor) {
        return {false, QStringLiteral("无法创建 SSH 执行器"), {}, {}};
    }

    const QString command = QStringLiteral("bash -lc %1").arg(shellQuote(innerCommand));
    const RemoteCommandResult commandResult = executor->execute(command, timeoutSec);
    if (!commandResult.ok) {
        return {false, commandResult.error.isEmpty() ? commandResult.stderrText : commandResult.error, {}, {}};
    }
    return {true, commandResult.stdoutText.trimmed(), {}, {}};
}

QStringList splitLines(const QString &text)
{
    return text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
}

}

ServiceResult KafkaServiceClient::ping(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote)
{
    const ServiceResult topics = listTopics(endpoint, remote);
    if (!topics.ok) {
        return topics;
    }
    return {true, QStringLiteral("Kafka 可达，主题数 %1").arg(topics.rows.size()), {}, {}};
}

ServiceResult KafkaServiceClient::listTopics(const ServiceEndpoint &endpoint, const RemoteConnectionContext &remote)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-topics.sh"));
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint),
         QStringLiteral("--list")},
        30);
    if (!result.ok) {
        return result;
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QString &line : splitLines(result.message)) {
        rows.rows.append(QJsonObject{{QStringLiteral("name"), line.trimmed()}});
    }
    return rows;
}

ServiceResult KafkaServiceClient::describeTopic(const ServiceEndpoint &endpoint,
                                                const RemoteConnectionContext &remote,
                                                const QString &topic)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-topics.sh"));
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint),
         QStringLiteral("--describe"),
         QStringLiteral("--topic"),
         topic},
        30);
    if (!result.ok) {
        return result;
    }

    int partitions = 0;
    int replication = 0;
    for (const QString &line : splitLines(result.message)) {
        if (line.contains(QStringLiteral("PartitionCount"))) {
            const QRegularExpression re(QStringLiteral("PartitionCount:\\s*(\\d+)"));
            const auto match = re.match(line);
            if (match.hasMatch()) {
                partitions = match.captured(1).toInt();
            }
            const QRegularExpression repRe(QStringLiteral("ReplicationFactor:\\s*(\\d+)"));
            const auto repMatch = repRe.match(line);
            if (repMatch.hasMatch()) {
                replication = repMatch.captured(1).toInt();
            }
        }
    }

    return {true,
            result.message,
            {},
            {QJsonObject{
                {QStringLiteral("name"), topic},
                {QStringLiteral("partitions"), QString::number(partitions)},
                {QStringLiteral("replication"), QString::number(replication)}
            }}};
}

ServiceResult KafkaServiceClient::listConsumerGroups(const ServiceEndpoint &endpoint,
                                                     const RemoteConnectionContext &remote)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-consumer-groups.sh"));
    const ServiceResult listed = runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint),
         QStringLiteral("--list")},
        30);
    if (!listed.ok) {
        return listed;
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QString &group : splitLines(listed.message)) {
        const ServiceResult described = runRemoteKafka(
            endpoint,
            remote,
            {script,
             QStringLiteral("--bootstrap-server"),
             bootstrapServer(endpoint),
             QStringLiteral("--describe"),
             QStringLiteral("--group"),
             group.trimmed()},
            30);
        QString topic;
        QString lag = QStringLiteral("-");
        if (described.ok) {
            for (const QString &line : splitLines(described.message)) {
                if (line.contains(group) && line.contains(QLatin1Char('\t'))) {
                    const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                    if (parts.size() >= 6) {
                        topic = parts.at(1);
                        lag = parts.at(5);
                    }
                }
            }
        }
        rows.rows.append(QJsonObject{
            {QStringLiteral("group"), group.trimmed()},
            {QStringLiteral("topic"), topic},
            {QStringLiteral("lag"), lag}
        });
    }
    return rows;
}

ServiceResult KafkaServiceClient::createTopic(const ServiceEndpoint &endpoint,
                                              const RemoteConnectionContext &remote,
                                              const QString &topic,
                                              int partitions,
                                              int replication)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-topics.sh"));
    return runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--create"),
         QStringLiteral("--topic"),
         topic,
         QStringLiteral("--partitions"),
         QString::number(partitions),
         QStringLiteral("--replication-factor"),
         QString::number(replication),
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint)},
        30);
}

ServiceResult KafkaServiceClient::deleteTopic(const ServiceEndpoint &endpoint,
                                              const RemoteConnectionContext &remote,
                                              const QString &topic)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-topics.sh"));
    return runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--delete"),
         QStringLiteral("--topic"),
         topic,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint)},
        30);
}

ServiceResult KafkaServiceClient::consumeLatest(const ServiceEndpoint &endpoint,
                                                const RemoteConnectionContext &remote,
                                                const QString &topic,
                                                int maxMessages)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-console-consumer.sh"));
    return runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint),
         QStringLiteral("--topic"),
         topic,
         QStringLiteral("--from-beginning"),
         QStringLiteral("--max-messages"),
         QString::number(maxMessages),
         QStringLiteral("--timeout-ms"),
         QStringLiteral("5000")},
        20);
}

ServiceResult KafkaServiceClient::describeAllTopics(const ServiceEndpoint &endpoint,
                                                    const RemoteConnectionContext &remote)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-topics.sh"));
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint),
         QStringLiteral("--describe")},
        45);
    if (!result.ok) {
        return result;
    }

    const QRegularExpression nameRe(QStringLiteral("Topic:\\s*(\\S+)"));
    const QRegularExpression partRe(QStringLiteral("PartitionCount:\\s*(\\d+)"));
    const QRegularExpression repRe(QStringLiteral("ReplicationFactor:\\s*(\\d+)"));

    ServiceResult rows{true, {}, {}, {}};
    for (const QString &line : splitLines(result.message)) {
        if (!line.contains(QStringLiteral("PartitionCount"))) {
            continue;
        }
        const auto nameMatch = nameRe.match(line);
        const auto partMatch = partRe.match(line);
        const auto repMatch = repRe.match(line);
        if (!nameMatch.hasMatch()) {
            continue;
        }
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), nameMatch.captured(1)},
            {QStringLiteral("partitions"), partMatch.hasMatch() ? partMatch.captured(1) : QStringLiteral("0")},
            {QStringLiteral("replication"), repMatch.hasMatch() ? repMatch.captured(1) : QStringLiteral("0")}
        });
    }
    return rows;
}

ServiceResult KafkaServiceClient::topicOffsets(const ServiceEndpoint &endpoint,
                                               const RemoteConnectionContext &remote,
                                               qint64 timeMs)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-run-class.sh"));
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("kafka.tools.GetOffsetShell"),
         QStringLiteral("--broker-list"),
         bootstrapServer(endpoint),
         QStringLiteral("--time"),
         QString::number(timeMs)},
        45);
    if (!result.ok) {
        return result;
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QString &line : splitLines(result.message)) {
        const QStringList parts = line.split(QLatin1Char(':'));
        if (parts.size() < 3) {
            continue;
        }
        bool ok = false;
        const qlonglong offset = parts.at(2).trimmed().toLongLong(&ok);
        if (!ok) {
            continue;
        }
        rows.rows.append(QJsonObject{
            {QStringLiteral("topic"), parts.at(0).trimmed()},
            {QStringLiteral("partition"), parts.at(1).trimmed()},
            {QStringLiteral("offset"), QString::number(offset)}
        });
    }
    return rows;
}

ServiceResult KafkaServiceClient::describeConsumerGroups(const ServiceEndpoint &endpoint,
                                                         const RemoteConnectionContext &remote)
{
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-consumer-groups.sh"));
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        {script,
         QStringLiteral("--bootstrap-server"),
         bootstrapServer(endpoint),
         QStringLiteral("--all-groups"),
         QStringLiteral("--describe")},
        60);
    if (!result.ok) {
        return result;
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QString &line : splitLines(result.message)) {
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() < 6) {
            continue;
        }
        if (parts.at(0) == QStringLiteral("GROUP")) {
            continue;
        }
        const QString partition = parts.at(2);
        bool partitionIsNumber = false;
        partition.toInt(&partitionIsNumber);
        if (!partitionIsNumber) {
            continue;
        }
        rows.rows.append(QJsonObject{
            {QStringLiteral("group"), parts.at(0)},
            {QStringLiteral("topic"), parts.at(1)},
            {QStringLiteral("partition"), partition},
            {QStringLiteral("current"), parts.at(3)},
            {QStringLiteral("logEnd"), parts.at(4)},
            {QStringLiteral("lag"), parts.at(5)}
        });
    }
    return rows;
}

ServiceResult KafkaServiceClient::produce(const ServiceEndpoint &endpoint,
                                          const RemoteConnectionContext &remote,
                                          const QString &topic,
                                          const QString &message)
{
    if (topic.isEmpty()) {
        return {false, QStringLiteral("请先选择主题"), {}, {}};
    }
    const QString script = kafkaScript(endpoint, QStringLiteral("kafka-console-producer.sh"));
    const QString inner = QStringLiteral("printf '%s\\n' %1 | %2 --bootstrap-server %3 --topic %4")
                              .arg(shellQuote(message),
                                   shellQuote(script),
                                   bootstrapServer(endpoint),
                                   shellQuote(topic));
    const ServiceResult result = runRemoteCommand(endpoint, remote, inner, 30);
    if (!result.ok) {
        return result;
    }
    return {true, QStringLiteral("已写入主题 %1").arg(topic), {}, {}};
}

ServiceResult KafkaServiceClient::nodeStatus(const ServiceEndpoint &endpoint,
                                             const RemoteConnectionContext &remote,
                                             const QString &storagePath)
{
    const QString dataDir = storagePath.isEmpty() ? endpoint.storagePath : storagePath;
    const QString inner = QStringLiteral(
                              "PORT=%1; DIR=%2; "
                              "if ss -ltn 2>/dev/null | grep -q \":$PORT \"; then echo status=running; "
                              "elif pgrep -f kafka.Kafka >/dev/null 2>&1; then echo status=running; "
                              "else echo status=stopped; fi; "
                              "echo dataDir=$DIR; "
                              "df -P -B1 \"$DIR\" 2>/dev/null | awk 'NR==2{print \"diskTotal=\"$2}'")
                              .arg(QString::number(endpoint.port), shellQuote(dataDir));
    const ServiceResult result = runRemoteCommand(endpoint, remote, inner, 30);
    if (!result.ok) {
        return result;
    }

    QString status = QStringLiteral("未知");
    QString resolvedDir = dataDir;
    QString diskTotal = QStringLiteral("-");
    for (const QString &line : splitLines(result.message)) {
        if (line.startsWith(QStringLiteral("status="))) {
            status = line.mid(QStringLiteral("status=").size()) == QStringLiteral("running")
                         ? QStringLiteral("运行中")
                         : QStringLiteral("已停止");
        } else if (line.startsWith(QStringLiteral("dataDir="))) {
            resolvedDir = line.mid(QStringLiteral("dataDir=").size());
        } else if (line.startsWith(QStringLiteral("diskTotal="))) {
            diskTotal = line.mid(QStringLiteral("diskTotal=").size());
        }
    }
    return {true, result.message, {}, {QJsonObject{
        {QStringLiteral("status"), status},
        {QStringLiteral("dataDir"), resolvedDir},
        {QStringLiteral("diskTotal"), diskTotal}
    }}};
}

ServiceResult KafkaServiceClient::viewConfig(const ServiceEndpoint &endpoint,
                                             const RemoteConnectionContext &remote)
{
    const QString configPath = endpoint.installPath.isEmpty()
                                   ? QStringLiteral("config/server.properties")
                                   : QDir(endpoint.installPath).filePath(QStringLiteral("config/server.properties"));
    const QString inner = QStringLiteral("cat %1").arg(shellQuote(configPath));
    return runRemoteCommand(endpoint, remote, inner, 20);
}
