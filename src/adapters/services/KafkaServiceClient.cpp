#include "adapters/services/KafkaServiceClient.h"

#include "adapters/services/KafkaAdminBridge.h"
#include "adapters/remote/RemoteExecutor.h"
#include "infra/RemoteOutputCleaner.h"

#include <QDir>
#include <QJsonArray>
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

QString kafkaBinDir(const ServiceEndpoint &endpoint)
{
    return endpoint.installPath.isEmpty() ? QString() : QDir(endpoint.installPath).filePath(QStringLiteral("bin"));
}

QString kafkaScript(const ServiceEndpoint &endpoint, const QString &scriptName)
{
    const QString binDir = kafkaBinDir(endpoint);
    if (binDir.isEmpty()) {
        return scriptName;
    }
    return QDir(binDir).filePath(scriptName);
}

QString mergedRemoteOutput(const RemoteCommandResult &commandResult)
{
    QString output = commandResult.stdoutText.trimmed();
    const QString stderrText = RemoteOutputCleaner::stripSshBanner(commandResult.stderrText.trimmed());
    if (!stderrText.isEmpty()) {
        if (!output.isEmpty()) {
            output += QLatin1Char('\n');
        }
        output += stderrText;
    }
    return output;
}

ServiceResult runRemoteKafka(const ServiceEndpoint &endpoint,
                             const RemoteConnectionContext &remote,
                             const QString &innerCommand,
                             int timeoutSec = 30,
                             bool acceptOutputOnFailure = false)
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

    const QString command = QStringLiteral("export LANG=en; bash --noprofile --norc -c %1").arg(shellQuote(innerCommand));
    const RemoteCommandResult commandResult = executor->execute(command, timeoutSec);
    const QString merged = mergedRemoteOutput(commandResult);
    const QString stdoutText = commandResult.stdoutText.trimmed();
    if (!commandResult.ok) {
        if (acceptOutputOnFailure && !stdoutText.isEmpty()) {
            return {true, stdoutText, {}, {}};
        }
        const QString detail = RemoteOutputCleaner::normalizeRemoteError(merged);
        return {false,
                commandResult.error.isEmpty() ? (detail.isEmpty() ? commandResult.stderrText : detail)
                                              : RemoteOutputCleaner::normalizeRemoteError(commandResult.error),
                {},
                {}};
    }
    if (merged.contains(QStringLiteral("Permission denied"), Qt::CaseInsensitive)) {
        return {false, RemoteOutputCleaner::normalizeRemoteError(merged), {}, {}};
    }
    return {true, stdoutText, {}, {}};
}

QStringList splitLines(const QString &text)
{
    return text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts);
}

bool isJvmJunkLine(const QString &line)
{
    if (line.isEmpty()) {
        return true;
    }
    // SLF4J / log4j noise
    if (line.startsWith(QStringLiteral("SLF4J"), Qt::CaseInsensitive)
        || line.startsWith(QStringLiteral("log4j"), Qt::CaseInsensitive)) {
        return true;
    }
    // Bracket-style log levels: [main], [INFO], [WARN], [ERROR], etc.
    if (line.startsWith(QLatin1Char('['))) {
        return true;
    }
    // Date-prefixed logs: 2025-... 
    if (line.size() >= 10 && line.at(4) == QLatin1Char('-') && line.at(7) == QLatin1Char('-')) {
        return true;
    }
    // Java classpath warnings / other common noise
    if (line.contains(QStringLiteral("Class path contains"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Processed a total of"), Qt::CaseInsensitive)
        || line.contains(QStringLiteral("Multiple bindings"))
        || line.contains(QStringLiteral("Failed to load class"), Qt::CaseInsensitive)) {
        return true;
    }
    const QString trimmed = line.trimmed();
    static const QRegularExpression processedTotalRe(
        QStringLiteral("^process(ed)?\\s+a\\s+total\\s+of\\s+\\d+\\s*messages?\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    if (processedTotalRe.match(trimmed).hasMatch()) {
        return true;
    }
    if (trimmed.contains(QStringLiteral("total of"), Qt::CaseInsensitive)
        && trimmed.contains(QStringLiteral("message"), Qt::CaseInsensitive)
        && trimmed.size() <= 96
        && !trimmed.contains(QLatin1Char('{'))
        && !trimmed.contains(QLatin1Char('['))) {
        return true;
    }
    return false;
}

QString topicOffsetRunnerFunction(const ServiceEndpoint &endpoint, const QString &topic)
{
    const QString binDir = shellQuote(kafkaBinDir(endpoint));
    const QString topicQ = shellQuote(topic);
    return QStringLiteral(
               "offsets() {\n"
               "  local T=\"$1\"\n"
               "  if [ -x %1/kafka-get-offsets.sh ]; then\n"
               "    bash %1/kafka-get-offsets.sh --bootstrap-server \"$BS\" --topic %2 --time \"$T\" 2>/dev/null\n"
               "    return $?\n"
               "  fi\n"
               "  if [ -x %1/kafka-run-class.sh ]; then\n"
               "    bash %1/kafka-run-class.sh kafka.tools.GetOffsetShell --bootstrap-server \"$BS\" --topic %2 --time \"$T\" 2>/dev/null\n"
               "    return $?\n"
               "  fi\n"
               "  return 1\n"
               "}\n")
        .arg(binDir, topicQ);
}

QString offsetRunnerFunction(const ServiceEndpoint &endpoint)
{
    const QString binDir = shellQuote(kafkaBinDir(endpoint));
    return QStringLiteral(
               "offsets() {\n"
               "  local T=\"$1\"\n"
               "  if [ -x %1/kafka-get-offsets.sh ]; then\n"
               "    bash %1/kafka-get-offsets.sh --bootstrap-server \"$BS\" --time \"$T\" 2>/dev/null\n"
               "    return $?\n"
               "  fi\n"
               "  if [ -x %1/kafka-run-class.sh ]; then\n"
               "    bash %1/kafka-run-class.sh kafka.tools.GetOffsetShell --bootstrap-server \"$BS\" --time \"$T\" 2>/dev/null\n"
               "    return $?\n"
               "  fi\n"
               "  return 1\n"
               "}\n")
        .arg(binDir);
}

QString singleOffsetCommand(const ServiceEndpoint &endpoint, qint64 timeMs)
{
    const QString binDir = shellQuote(kafkaBinDir(endpoint));
    const QString bs = shellQuote(bootstrapServer(endpoint));
    const QString time = QString::number(timeMs);
    return QStringLiteral(
               "BS=%1\n"
               "%2"
               "offsets %3")
        .arg(bs, offsetRunnerFunction(endpoint), time);
}

QString popMarkedSection(QString &rest, const QString &marker)
{
    const int markerIndex = rest.indexOf(marker);
    if (markerIndex < 0) {
        return {};
    }
    rest = rest.mid(markerIndex + marker.size());
    const int nextMarker = rest.indexOf(QStringLiteral("@DH_MARKER|"));
    const QString section = nextMarker < 0 ? rest : rest.left(nextMarker);
    rest = nextMarker < 0 ? QString() : rest.mid(nextMarker);
    return section.trimmed();
}

QHash<QString, qlonglong> offsetRowsToMap(const QVector<QJsonObject> &rows)
{
    QHash<QString, qlonglong> map;
    for (const QJsonObject &row : rows) {
        const QString key = row.value(QStringLiteral("topic")).toString() + QLatin1Char('\t')
            + row.value(QStringLiteral("partition")).toString();
        map.insert(key, row.value(QStringLiteral("offset")).toString().toLongLong());
    }
    return map;
}

QString kafkaShellCommand(const ServiceEndpoint &endpoint, const QString &scriptName, const QStringList &args)
{
    const QString scriptPath = shellQuote(kafkaScript(endpoint, scriptName));
    QStringList command;
    command << QStringLiteral("bash") << scriptPath;
    command.append(args);
    return command.join(QLatin1Char(' '));
}

}

QString KafkaServiceClient::sanitizeConsumerOutput(const QString &text)
{
    QStringList lines;
    for (const QString &line : splitLines(text)) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith(QStringLiteral("@DH_MARKER|")) || isJvmJunkLine(trimmed)) {
            continue;
        }
        lines.append(trimmed);
    }
    return lines.join(QStringLiteral("\n\n"));
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
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        kafkaShellCommand(endpoint,
                          QStringLiteral("kafka-topics.sh"),
                          {QStringLiteral("--bootstrap-server"),
                           bootstrapServer(endpoint),
                           QStringLiteral("--list")}),
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
    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        kafkaShellCommand(endpoint,
                          QStringLiteral("kafka-topics.sh"),
                          {QStringLiteral("--bootstrap-server"),
                           bootstrapServer(endpoint),
                           QStringLiteral("--describe"),
                           QStringLiteral("--topic"),
                           topic}),
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
    const ServiceResult listed = runRemoteKafka(
        endpoint,
        remote,
        kafkaShellCommand(endpoint,
                          QStringLiteral("kafka-consumer-groups.sh"),
                          {QStringLiteral("--bootstrap-server"),
                           bootstrapServer(endpoint),
                           QStringLiteral("--list")}),
        30);
    if (!listed.ok) {
        return listed;
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QString &group : splitLines(listed.message)) {
        const ServiceResult described = runRemoteKafka(
            endpoint,
            remote,
            kafkaShellCommand(endpoint,
                              QStringLiteral("kafka-consumer-groups.sh"),
                              {QStringLiteral("--bootstrap-server"),
                               bootstrapServer(endpoint),
                               QStringLiteral("--describe"),
                               QStringLiteral("--group"),
                               group.trimmed()}),
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
    return runRemoteKafka(
        endpoint,
        remote,
        kafkaShellCommand(endpoint,
                          QStringLiteral("kafka-topics.sh"),
                          {QStringLiteral("--create"),
                           QStringLiteral("--topic"),
                           topic,
                           QStringLiteral("--partitions"),
                           QString::number(partitions),
                           QStringLiteral("--replication-factor"),
                           QString::number(replication),
                           QStringLiteral("--bootstrap-server"),
                           bootstrapServer(endpoint)}),
        30);
}

ServiceResult KafkaServiceClient::deleteTopic(const ServiceEndpoint &endpoint,
                                              const RemoteConnectionContext &remote,
                                              const QString &topic)
{
    return runRemoteKafka(
        endpoint,
        remote,
        kafkaShellCommand(endpoint,
                          QStringLiteral("kafka-topics.sh"),
                          {QStringLiteral("--delete"),
                           QStringLiteral("--topic"),
                           topic,
                           QStringLiteral("--bootstrap-server"),
                           bootstrapServer(endpoint)}),
        30);
}

ServiceResult KafkaServiceClient::consumeLatest(const ServiceEndpoint &endpoint,
                                                const RemoteConnectionContext &remote,
                                                const QString &topic,
                                                int maxMessages)
{
    const int limit = qBound(1, maxMessages > 0 ? maxMessages : 10, 50);
    if (KafkaAdminBridge::isAvailable()) {
        const KafkaAdminPayload payload = KafkaAdminBridge::run(
            endpoint,
            remote,
            {QStringLiteral("--action"),
             QStringLiteral("consume-latest"),
             QStringLiteral("--topic"),
             topic,
             QStringLiteral("--max-messages"),
             QString::number(limit)},
            60);
        if (payload.ok) {
            const QString text = KafkaServiceClient::sanitizeConsumerOutput(payload.message.trimmed());
            if (text.isEmpty()) {
                return {true, QStringLiteral("该主题暂无可用消息。"), {}, {}};
            }
            return {true, text, {}, {}};
        }
        if (!payload.message.isEmpty()) {
            return {false, payload.message, {}, {}};
        }
    }

    const QString binDir = shellQuote(kafkaBinDir(endpoint));
    const QString bs = shellQuote(bootstrapServer(endpoint));
    const QString topicQ = shellQuote(topic);
    const QString script = QStringLiteral(
                               "BIN=%1\n"
                               "BS=%2\n"
                               "TOPIC=%3\n"
                               "LIMIT=%4\n"
                               "chmod +x \"$BIN\"/*.sh 2>/dev/null || true\n"
                               "bash \"$BIN/kafka-console-consumer.sh\" --bootstrap-server \"$BS\" --topic \"$TOPIC\" "
                               "--max-messages \"$LIMIT\" --timeout-ms 5000 2>/dev/null || true\n"
                               "exit 0\n")
                               .arg(binDir, bs, topicQ, QString::number(limit));

    const ServiceResult result = runRemoteKafka(endpoint, remote, script, 45, true);
    if (!result.ok) {
        return result;
    }

    const QString sanitized = KafkaServiceClient::sanitizeConsumerOutput(result.message);
    if (sanitized.isEmpty()) {
        return {true, QStringLiteral("该主题暂无可用消息。"), {}, {}};
    }
    return {true, sanitized, {}, {}};
}

ServiceResult KafkaServiceClient::describeAllTopics(const ServiceEndpoint &endpoint,
                                                    const RemoteConnectionContext &remote)
{
    if (KafkaAdminBridge::isAvailable()) {
        const KafkaAdminPayload payload =
            KafkaAdminBridge::run(endpoint, remote, {QStringLiteral("--action"), QStringLiteral("topics")}, 60);
        if (payload.ok) {
            ServiceResult rows{true, {}, {}, {}};
            rows.rows = payload.rows;
            return rows;
        }
    }

    // Try --describe first (gives partitions/replication), with increased timeout.
    // Automatically chmod +x the script to avoid "Permission denied" errors.
    const QString scriptPath = shellQuote(kafkaScript(endpoint, QStringLiteral("kafka-topics.sh")));
    const QString bs = shellQuote(bootstrapServer(endpoint));
    ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        QStringLiteral("chmod +x %1 2>/dev/null || true; bash %1 --bootstrap-server %2 --describe")
            .arg(scriptPath, bs),
        60);
    if (result.ok) {
        ServiceResult rows{true, {}, {}, {}};
        QVector<QJsonObject> parsed = parseDescribeTopicsOutput(result.message);
        if (!parsed.isEmpty()) {
            rows.rows = parsed;
            return rows;
        }
    }

    // Fallback: --list (topic names only) when --describe fails or returns no parseable rows.
    result = runRemoteKafka(
        endpoint,
        remote,
        QStringLiteral("chmod +x %1 2>/dev/null || true; bash %1 --bootstrap-server %2 --list")
            .arg(scriptPath, bs),
        30);
    if (!result.ok) {
        return result;
    }
    ServiceResult rows{true, {}, {}, {}};
    for (const QString &line : splitLines(result.message)) {
        rows.rows.append(QJsonObject{{QStringLiteral("name"), line.trimmed()},
                                     {QStringLiteral("partitions"), QStringLiteral("0")},
                                     {QStringLiteral("replication"), QStringLiteral("0")}});
    }
    return rows;
}

QVector<QJsonObject> KafkaServiceClient::parseDescribeTopicsOutput(const QString &text)
{
    const QRegularExpression nameRe(QStringLiteral("Topic:\\s*(\\S+)"));
    const QRegularExpression partRe(QStringLiteral("PartitionCount:\\s*(\\d+)"));
    const QRegularExpression repRe(QStringLiteral("ReplicationFactor:\\s*(\\d+)"));

    QVector<QJsonObject> rows;
    for (const QString &line : splitLines(text)) {
        if (!line.contains(QStringLiteral("PartitionCount"))) {
            continue;
        }
        if (isJvmJunkLine(line) || line.contains(QStringLiteral("Error"))) {
            continue;
        }
        const auto nameMatch = nameRe.match(line);
        const auto partMatch = partRe.match(line);
        const auto repMatch = repRe.match(line);
        if (!nameMatch.hasMatch()) {
            continue;
        }
        rows.append(QJsonObject{
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
    const ServiceResult result = runRemoteKafka(endpoint, remote, singleOffsetCommand(endpoint, timeMs), 45);
    if (!result.ok) {
        return result;
    }

    ServiceResult rows{true, {}, {}, {}};
    rows.rows = parseOffsetOutputLines(result.message);
    return rows;
}

QVector<QJsonObject> KafkaServiceClient::parseOffsetOutputLines(const QString &text)
{
    QVector<QJsonObject> rows;
    for (const QString &line : splitLines(text)) {
        if (line.startsWith(QStringLiteral("@DH_MARKER|"))
            || line.contains(QStringLiteral("Error"))
            || isJvmJunkLine(line)) {
            continue;
        }

        QString topic;
        QString partition;
        QString offsetText;
        if (line.contains(QLatin1Char(':'))) {
            const QStringList parts = line.split(QLatin1Char(':'));
            if (parts.size() < 3) {
                continue;
            }
            topic = parts.at(0).trimmed();
            partition = parts.at(1).trimmed();
            offsetText = parts.at(2).trimmed();
        } else {
            const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (parts.size() < 3) {
                continue;
            }
            topic = parts.at(0).trimmed();
            partition = parts.at(1).trimmed();
            offsetText = parts.at(2).trimmed();
        }

        bool ok = false;
        const qlonglong offset = offsetText.toLongLong(&ok);
        if (!ok || offset < 0) {
            continue;
        }
        rows.append(QJsonObject{
            {QStringLiteral("topic"), topic},
            {QStringLiteral("partition"), partition},
            {QStringLiteral("offset"), QString::number(offset)}
        });
    }
    return rows;
}

KafkaTopicDashboard KafkaServiceClient::loadTopicDashboard(const ServiceEndpoint &endpoint,
                                                         const RemoteConnectionContext &remote,
                                                         qint64 todayStartMs,
                                                         qint64 yesterdayStartMs,
                                                         qint64 weekStartMs,
                                                         const QVector<QJsonObject> &knownTopics)
{
    KafkaTopicDashboard dashboard;
    if (!knownTopics.isEmpty()) {
        dashboard.topics = knownTopics;
    }
    if (KafkaAdminBridge::isAvailable()) {
        const KafkaAdminPayload payload = KafkaAdminBridge::run(
            endpoint,
            remote,
            {QStringLiteral("--action"),
             QStringLiteral("dashboard"),
             QStringLiteral("--today-start"),
             QString::number(todayStartMs),
             QStringLiteral("--yesterday-start"),
             QString::number(yesterdayStartMs),
             QStringLiteral("--week-start"),
             QString::number(weekStartMs)},
            120);
        if (payload.ok) {
            if (!payload.rows.isEmpty()) {
                dashboard.topics = payload.rows;
            }
            for (const QJsonObject &row : payload.stats) {
                const QString key = row.value(QStringLiteral("topic")).toString() + QLatin1Char('\t')
                    + row.value(QStringLiteral("partition")).toString();
                dashboard.latest.insert(key, row.value(QStringLiteral("latest")).toString().toLongLong());
                dashboard.earliest.insert(key, row.value(QStringLiteral("earliest")).toString().toLongLong());
                dashboard.atToday.insert(key, row.value(QStringLiteral("today")).toString().toLongLong());
                dashboard.atYesterday.insert(key, row.value(QStringLiteral("yesterday")).toString().toLongLong());
                dashboard.atWeek.insert(key, row.value(QStringLiteral("week")).toString().toLongLong());
            }
            dashboard.groupPartitions = payload.partitions;
            dashboard.ok = !dashboard.topics.isEmpty();
            return dashboard;
        }
    }

    const QString binDir = shellQuote(kafkaBinDir(endpoint));
    const QString bs = shellQuote(bootstrapServer(endpoint));
    // Shell fallback: each step is guarded by || true for partial-failure tolerance.
    // offsetRunnerFunction now uses only kafka-get-offsets.sh (fast) or single JVM path.

    const bool skipDescribe = !dashboard.topics.isEmpty();
    QString script = QStringLiteral("BS=%1\nBIN=%2\nchmod +x \"$BIN\"/*.sh 2>/dev/null || true\n").arg(bs, binDir);
    script += offsetRunnerFunction(endpoint);
    if (!skipDescribe) {
        script += QStringLiteral(
            "bash \"$BIN/kafka-topics.sh\" --bootstrap-server \"$BS\" --describe 2>/dev/null || true\n");
    }
    script += QStringLiteral(
        "echo '@DH_MARKER|topics|end@'\n"
        // Run both offset queries in parallel to halve JVM startup time
        "TMP1=$(mktemp /tmp/dh_offset_XXXXXX)\n"
        "TMP2=$(mktemp /tmp/dh_offset_XXXXXX)\n"
        "offsets -1 > \"$TMP1\" 2>/dev/null &\n"
        "PID1=$!\n"
        "offsets -2 > \"$TMP2\" 2>/dev/null &\n"
        "PID2=$!\n"
        "wait $PID1 $PID2 2>/dev/null || true\n"
        "cat \"$TMP1\" 2>/dev/null || true\n"
        "echo '@DH_MARKER|offset|-1@'\n"
        "cat \"$TMP2\" 2>/dev/null || true\n"
        "echo '@DH_MARKER|offset|-2@'\n"
        "rm -f \"$TMP1\" \"$TMP2\"\n"
        "echo '@DH_MARKER|groups|start@'\n"
        "bash \"$BIN/kafka-consumer-groups.sh\" --bootstrap-server \"$BS\" --all-groups --describe 2>/dev/null || true\n"
        "exit 0\n");

    const ServiceResult result = runRemoteKafka(endpoint, remote, script, 120);
    if (!result.ok) {
        dashboard.message = result.message;
        return dashboard;
    }

    QString rest = result.message;
    const int firstMarker = rest.indexOf(QStringLiteral("@DH_MARKER|"));
    if (!skipDescribe) {
        const QString topicsText = firstMarker >= 0 ? rest.left(firstMarker).trimmed() : rest.trimmed();
        dashboard.topics = parseDescribeTopicsOutput(topicsText);
    }
    rest = firstMarker >= 0 ? rest.mid(firstMarker) : QString();
    popMarkedSection(rest, QStringLiteral("@DH_MARKER|topics|end@"));

    dashboard.latest = offsetRowsToMap(parseOffsetOutputLines(popMarkedSection(rest, QStringLiteral("@DH_MARKER|offset|-1@"))));
    dashboard.earliest = offsetRowsToMap(parseOffsetOutputLines(popMarkedSection(rest, QStringLiteral("@DH_MARKER|offset|-2@"))));
    dashboard.groupPartitions = parseConsumerGroupRows(popMarkedSection(rest, QStringLiteral("@DH_MARKER|groups|start@")));

    // Allow partial success: topics list is enough to render rows; offset/group
    // stats simply show "-" when their queries failed.
    if (dashboard.topics.isEmpty()) {
        dashboard.message = QStringLiteral("未能读取 Kafka 主题列表，请确认安装路径与 broker 端口");
        return dashboard;
    }

    dashboard.ok = true;
    return dashboard;
}

ServiceResult KafkaServiceClient::describeConsumerGroups(const ServiceEndpoint &endpoint,
                                                         const RemoteConnectionContext &remote)
{
    if (KafkaAdminBridge::isAvailable()) {
        const KafkaAdminPayload payload = KafkaAdminBridge::run(
            endpoint, remote, {QStringLiteral("--action"), QStringLiteral("consumer-groups")}, 60);
        if (payload.ok) {
            ServiceResult rows{true, {}, {}, {}};
            rows.rows = payload.partitions;
            QJsonArray partitionArray;
            for (const QJsonObject &row : payload.partitions) {
                partitionArray.append(row);
            }
            rows.text = QString::fromUtf8(QJsonDocument(partitionArray).toJson(QJsonDocument::Compact));
            return rows;
        }
    }

    const ServiceResult result = runRemoteKafka(
        endpoint,
        remote,
        kafkaShellCommand(endpoint,
                          QStringLiteral("kafka-consumer-groups.sh"),
                          {QStringLiteral("--bootstrap-server"),
                           bootstrapServer(endpoint),
                           QStringLiteral("--all-groups"),
                           QStringLiteral("--describe")}),
        60);
    if (!result.ok) {
        return result;
    }

    ServiceResult rows{true, {}, {}, {}};
    rows.rows = parseConsumerGroupRows(result.message);
    QJsonArray partitionArray;
    for (const QJsonObject &row : rows.rows) {
        partitionArray.append(row);
    }
    rows.text = QString::fromUtf8(QJsonDocument(partitionArray).toJson(QJsonDocument::Compact));
    return rows;
}

QVector<QJsonObject> KafkaServiceClient::parseConsumerGroupRows(const QString &text)
{
    QVector<QJsonObject> rows;
    for (const QString &line : splitLines(text)) {
        QStringList parts;
        if (line.contains(QLatin1Char('\t'))) {
            parts = line.split(QLatin1Char('\t'), Qt::KeepEmptyParts);
            for (QString &part : parts) {
                part = part.trimmed();
            }
        } else {
            parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        }
        if (parts.size() < 6) {
            continue;
        }
        if (parts.at(0).compare(QStringLiteral("GROUP"), Qt::CaseInsensitive) == 0) {
            continue;
        }
        const QString partition = parts.at(2);
        bool partitionIsNumber = false;
        partition.toInt(&partitionIsNumber);
        if (!partitionIsNumber) {
            continue;
        }
        rows.append(QJsonObject{
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
    const QString inner = QStringLiteral("printf '%s\\n' %1 | bash %2 --bootstrap-server %3 --topic %4")
                              .arg(shellQuote(message),
                                   shellQuote(script),
                                   bootstrapServer(endpoint),
                                   shellQuote(topic));
    const ServiceResult result = runRemoteKafka(endpoint, remote, inner, 30);
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
    const ServiceResult result = runRemoteKafka(endpoint, remote, inner, 30);
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
    return runRemoteKafka(endpoint, remote, inner, 20);
}
