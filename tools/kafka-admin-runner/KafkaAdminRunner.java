import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.clients.admin.AdminClient;
import org.apache.kafka.clients.admin.AdminClientConfig;
import org.apache.kafka.clients.admin.ConsumerGroupListing;
import org.apache.kafka.clients.admin.ListConsumerGroupOffsetsResult;
import org.apache.kafka.clients.admin.ListOffsetsResult;
import org.apache.kafka.clients.admin.OffsetSpec;
import org.apache.kafka.clients.admin.TopicDescription;
import org.apache.kafka.clients.consumer.OffsetAndMetadata;
import org.apache.kafka.common.TopicPartition;
import org.apache.kafka.common.TopicPartitionInfo;

public final class KafkaAdminRunner {
    private static final int TIMEOUT_SEC = 45;

    public static void main(String[] args) throws Exception {
        String action = null;
        String bootstrap = null;
        String group = null;
        String topic = null;
        int maxMessages = 10;
        long todayStart = 0L;
        long yesterdayStart = 0L;
        long weekStart = 0L;

        for (int i = 0; i < args.length; ++i) {
            final String arg = args[i];
            if ("--action".equals(arg) && i + 1 < args.length) {
                action = args[++i];
            } else if ("--bootstrap-server".equals(arg) && i + 1 < args.length) {
                bootstrap = args[++i];
            } else if ("--group".equals(arg) && i + 1 < args.length) {
                group = args[++i];
            } else if ("--topic".equals(arg) && i + 1 < args.length) {
                topic = args[++i];
            } else if ("--max-messages".equals(arg) && i + 1 < args.length) {
                maxMessages = Integer.parseInt(args[++i]);
            } else if ("--today-start".equals(arg) && i + 1 < args.length) {
                todayStart = Long.parseLong(args[++i]);
            } else if ("--yesterday-start".equals(arg) && i + 1 < args.length) {
                yesterdayStart = Long.parseLong(args[++i]);
            } else if ("--week-start".equals(arg) && i + 1 < args.length) {
                weekStart = Long.parseLong(args[++i]);
            }
        }

        if (action == null || bootstrap == null) {
            writeError("missing --action or --bootstrap-server");
            return;
        }

        final Properties props = new Properties();
        props.put(AdminClientConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrap);
        props.put(AdminClientConfig.REQUEST_TIMEOUT_MS_CONFIG, String.valueOf(TIMEOUT_SEC * 1000));
        props.put(AdminClientConfig.DEFAULT_API_TIMEOUT_MS_CONFIG, String.valueOf(TIMEOUT_SEC * 1000));

        try (AdminClient admin = AdminClient.create(props)) {
            switch (action) {
            case "topics":
                writeTopics(admin);
                break;
            case "topic-stats":
                writeTopicStats(admin, todayStart, yesterdayStart, weekStart);
                break;
            case "dashboard":
                writeDashboard(admin, todayStart, yesterdayStart, weekStart);
                break;
            case "consumer-groups":
                writeConsumerGroups(admin, null);
                break;
            case "consumer-group":
                if (group == null || group.isEmpty()) {
                    writeError("missing --group");
                    return;
                }
                writeConsumerGroups(admin, group);
                break;
            case "consume-latest":
                if (topic == null || topic.isEmpty()) {
                    writeError("missing --topic");
                    return;
                }
                writeConsumeLatest(admin, bootstrap, topic, maxMessages);
                break;
            default:
                writeError("unknown action: " + action);
                return;
            }
        } catch (Exception ex) {
            writeError(ex.getMessage() == null ? ex.toString() : ex.getMessage());
        }
    }

    private static void writeTopics(AdminClient admin) throws Exception {
        writePayload(true, "", collectTopicRows(admin), new ArrayList<>(), new ArrayList<>());
    }

    private static void writeTopicStats(AdminClient admin, long todayStart, long yesterdayStart, long weekStart)
            throws Exception {
        final List<Map<String, String>> topicRows = collectTopicRows(admin);
        final List<TopicPartition> partitions = collectTopicPartitions(admin, topicRows);
        final List<Map<String, String>> statRows = collectStatRows(admin, partitions, todayStart, yesterdayStart, weekStart);
        writePayload(true, "", topicRows, statRows, new ArrayList<>());
    }

    private static void writeDashboard(AdminClient admin, long todayStart, long yesterdayStart, long weekStart)
            throws Exception {
        final List<Map<String, String>> topicRows = collectTopicRows(admin);
        final List<TopicPartition> partitions = collectTopicPartitions(admin, topicRows);
        final List<Map<String, String>> statRows = collectStatRows(admin, partitions, todayStart, yesterdayStart, weekStart);
        final List<Map<String, String>> groupPartitions = collectConsumerGroupPartitions(admin, null);
        writePayload(true, "", topicRows, statRows, groupPartitions);
    }

    private static void writeConsumeLatest(AdminClient admin, String bootstrap, String topic, int maxMessages)
            throws Exception {
        const int limit = Math.max(1, Math.min(maxMessages, 50));
        final int maxParts = 2;

        final Map<String, TopicDescription> descriptions =
            admin.describeTopics(Collections.singleton(topic)).all().get(TIMEOUT_SEC, TimeUnit.SECONDS);
        final TopicDescription description = descriptions.get(topic);
        if (description == null || description.partitions().isEmpty()) {
            writePayload(true, "该主题暂无可用消息。",
                new ArrayList<>(), new ArrayList<>(), new ArrayList<>());
            return;
        }

        final List<TopicPartition> partitions = new ArrayList<>();
        for (TopicPartitionInfo partitionInfo : description.partitions()) {
            partitions.add(new TopicPartition(topic, partitionInfo.partition()));
        }

        final Map<TopicPartition, OffsetSpec> latestSpec = new HashMap<>();
        for (TopicPartition partition : partitions) {
            latestSpec.put(partition, OffsetSpec.latest());
        }
        final Map<TopicPartition, ListOffsetsResult.ListOffsetsResultInfo> endOffsets =
            admin.listOffsets(latestSpec).all().get(TIMEOUT_SEC, TimeUnit.SECONDS);

        final Properties props = new Properties();
        props.put(ConsumerConfig.BOOTSTRAP_SERVERS_CONFIG, bootstrap);
        props.put(ConsumerConfig.GROUP_ID_CONFIG, "deploy-hub-preview");
        props.put(ConsumerConfig.ENABLE_AUTO_COMMIT_CONFIG, "false");
        props.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, "earliest");
        props.put(ConsumerConfig.MAX_POLL_RECORDS_CONFIG, String.valueOf(limit));
        props.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG,
            "org.apache.kafka.common.serialization.ByteArrayDeserializer");
        props.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG,
            "org.apache.kafka.common.serialization.ByteArrayDeserializer");

        final StringBuilder messages = new StringBuilder();
        int partCount = 0;
        try (KafkaConsumer<byte[], byte[]> consumer = new KafkaConsumer<>(props)) {
            for (TopicPartition partition : partitions) {
                if (partCount >= maxParts) {
                    break;
                }
                final ListOffsetsResult.ListOffsetsResultInfo endInfo = endOffsets.get(partition);
                if (endInfo == null) {
                    continue;
                }
                final long end = endInfo.offset();
                final long start = Math.max(0L, end - limit);
                consumer.assign(Collections.singletonList(partition));
                consumer.seek(partition, start);
                final ConsumerRecords<byte[], byte[]> records =
                    consumer.poll(java.time.Duration.ofMillis(1500));
                for (ConsumerRecord<byte[], byte[]> record : records) {
                    if (messages.length() > 0) {
                        messages.append("\n\n");
                    }
                    final byte[] value = record.value();
                    messages.append(value == null ? "" : new String(value, StandardCharsets.UTF_8));
                }
                consumer.unassign();
                partCount++;
            }
        }

        final String text = messages.toString().trim();
        writePayload(true,
            text.isEmpty() ? "该主题暂无可用消息。" : text,
            new ArrayList<>(), new ArrayList<>(), new ArrayList<>());
    }

    private static List<Map<String, String>> collectTopicRows(AdminClient admin) throws Exception {
        final Set<String> names = admin.listTopics().names().get(TIMEOUT_SEC, TimeUnit.SECONDS);
        if (names.isEmpty()) {
            return new ArrayList<>();
        }
        final Map<String, TopicDescription> descriptions =
            admin.describeTopics(names).all().get(TIMEOUT_SEC, TimeUnit.SECONDS);
        final List<Map<String, String>> topicRows = new ArrayList<>();
        for (String name : names) {
            final TopicDescription description = descriptions.get(name);
            if (description == null) {
                continue;
            }
            int replication = 0;
            for (TopicPartitionInfo partitionInfo : description.partitions()) {
                replication = Math.max(replication, partitionInfo.replicas().size());
            }
            final Map<String, String> row = new LinkedHashMap<>();
            row.put("name", name);
            row.put("partitions", String.valueOf(description.partitions().size()));
            row.put("replication", String.valueOf(replication));
            topicRows.add(row);
        }
        return topicRows;
    }

    private static List<TopicPartition> collectTopicPartitions(AdminClient admin,
                                                               List<Map<String, String>> topicRows) throws Exception {
        final List<TopicPartition> partitions = new ArrayList<>();
        if (topicRows.isEmpty()) {
            return partitions;
        }
        final Set<String> names = new HashSet<>();
        for (Map<String, String> row : topicRows) {
            names.add(row.get("name"));
        }
        final Map<String, TopicDescription> descriptions =
            admin.describeTopics(names).all().get(TIMEOUT_SEC, TimeUnit.SECONDS);
        for (String name : names) {
            final TopicDescription description = descriptions.get(name);
            if (description == null) {
                continue;
            }
            for (TopicPartitionInfo partitionInfo : description.partitions()) {
                partitions.add(new TopicPartition(name, partitionInfo.partition()));
            }
        }
        return partitions;
    }

    private static List<Map<String, String>> collectStatRows(AdminClient admin,
                                                            List<TopicPartition> partitions,
                                                            long todayStart,
                                                            long yesterdayStart,
                                                            long weekStart) throws Exception {
        if (partitions.isEmpty()) {
            return new ArrayList<>();
        }
        final Map<TopicPartition, Long> latest = listOffsetValues(admin, partitions, OffsetSpec.latest());
        final Map<TopicPartition, Long> earliest = listOffsetValues(admin, partitions, OffsetSpec.earliest());
        final Map<TopicPartition, Long> atToday =
            listOffsetValues(admin, partitions, OffsetSpec.forTimestamp(todayStart));
        final Map<TopicPartition, Long> atYesterday =
            listOffsetValues(admin, partitions, OffsetSpec.forTimestamp(yesterdayStart));
        final Map<TopicPartition, Long> atWeek =
            listOffsetValues(admin, partitions, OffsetSpec.forTimestamp(weekStart));

        final List<Map<String, String>> statRows = new ArrayList<>();
        for (TopicPartition partition : partitions) {
            final Map<String, String> row = new LinkedHashMap<>();
            row.put("topic", partition.topic());
            row.put("partition", String.valueOf(partition.partition()));
            row.put("latest", String.valueOf(latest.getOrDefault(partition, 0L)));
            row.put("earliest", String.valueOf(earliest.getOrDefault(partition, 0L)));
            row.put("today", String.valueOf(atToday.getOrDefault(partition, 0L)));
            row.put("yesterday", String.valueOf(atYesterday.getOrDefault(partition, 0L)));
            row.put("week", String.valueOf(atWeek.getOrDefault(partition, 0L)));
            statRows.add(row);
        }
        return statRows;
    }

    private static List<Map<String, String>> collectConsumerGroupPartitions(AdminClient admin, String onlyGroup)
            throws Exception {
        final Collection<ConsumerGroupListing> listings =
            admin.listConsumerGroups().all().get(TIMEOUT_SEC, TimeUnit.SECONDS);
        final List<String> groupIds = new ArrayList<>();
        for (ConsumerGroupListing listing : listings) {
            if (onlyGroup == null || onlyGroup.equals(listing.groupId())) {
                groupIds.add(listing.groupId());
            }
        }

        final List<Map<String, String>> partitionRows = new ArrayList<>();
        final Set<TopicPartition> allPartitions = new HashSet<>();
        for (String groupId : groupIds) {
            final ListConsumerGroupOffsetsResult offsetsResult = admin.listConsumerGroupOffsets(groupId);
            final Map<TopicPartition, OffsetAndMetadata> committed =
                offsetsResult.partitionsToOffsetAndMetadata().get(TIMEOUT_SEC, TimeUnit.SECONDS);
            allPartitions.addAll(committed.keySet());
            for (Map.Entry<TopicPartition, OffsetAndMetadata> entry : committed.entrySet()) {
                final Map<String, String> row = new LinkedHashMap<>();
                row.put("group", groupId);
                row.put("topic", entry.getKey().topic());
                row.put("partition", String.valueOf(entry.getKey().partition()));
                row.put("current", String.valueOf(entry.getValue().offset()));
                row.put("logEnd", "0");
                row.put("lag", "0");
                row.put("consumerId", entry.getValue().metadata() == null ? "" : entry.getValue().metadata());
                partitionRows.add(row);
            }
        }

        final Map<TopicPartition, Long> endOffsets = listOffsetValues(admin, new ArrayList<>(allPartitions),
            OffsetSpec.latest());
        for (Map<String, String> row : partitionRows) {
            final TopicPartition partition = new TopicPartition(row.get("topic"),
                Integer.parseInt(row.get("partition")));
            final long current = Long.parseLong(row.get("current"));
            final long logEnd = endOffsets.getOrDefault(partition, current);
            row.put("logEnd", String.valueOf(logEnd));
            row.put("lag", String.valueOf(Math.max(0L, logEnd - current)));
        }
        return partitionRows;
    }

    private static Map<TopicPartition, Long> listOffsetValues(AdminClient admin,
                                                              List<TopicPartition> partitions,
                                                              OffsetSpec spec) throws Exception {
        if (partitions.isEmpty()) {
            return Collections.emptyMap();
        }
        final Map<TopicPartition, OffsetSpec> request = new HashMap<>();
        for (TopicPartition partition : partitions) {
            request.put(partition, spec);
        }
        final ListOffsetsResult result = admin.listOffsets(request);
        final Map<TopicPartition, org.apache.kafka.clients.admin.ListOffsetsResult.ListOffsetsResultInfo> values =
            result.all().get(TIMEOUT_SEC, TimeUnit.SECONDS);
        final Map<TopicPartition, Long> mapped = new HashMap<>();
        for (Map.Entry<TopicPartition, org.apache.kafka.clients.admin.ListOffsetsResult.ListOffsetsResultInfo> entry
                : values.entrySet()) {
            mapped.put(entry.getKey(), entry.getValue().offset());
        }
        return mapped;
    }

    private static void writeConsumerGroups(AdminClient admin, String onlyGroup) throws Exception {
        final List<Map<String, String>> partitionRows = collectConsumerGroupPartitions(admin, onlyGroup);
        final Map<String, Map<String, String>> summaryByGroup = new LinkedHashMap<>();
        for (Map<String, String> row : partitionRows) {
            final String groupId = row.get("group");
            if (!summaryByGroup.containsKey(groupId)) {
                summaryByGroup.put(groupId, newSummaryRow(groupId));
            }
            final Map<String, String> summary = summaryByGroup.get(groupId);
            summary.put("total", String.valueOf(Long.parseLong(summary.get("total")) + Long.parseLong(row.get("logEnd"))));
            summary.put("current",
                         String.valueOf(Long.parseLong(summary.get("current")) + Long.parseLong(row.get("current"))));
            summary.put("lag", String.valueOf(Long.parseLong(summary.get("lag")) + Long.parseLong(row.get("lag"))));
        }
        writePayload(true, "", new ArrayList<>(summaryByGroup.values()), new ArrayList<>(), partitionRows);
    }

    private static Map<String, String> newSummaryRow(String groupId) {
        final Map<String, String> row = new LinkedHashMap<>();
        row.put("group", groupId);
        row.put("total", "0");
        row.put("current", "0");
        row.put("lag", "0");
        return row;
    }

    private static void writePayload(boolean ok,
                                     String message,
                                     List<Map<String, String>> rows,
                                     List<Map<String, String>> stats,
                                     List<Map<String, String>> partitions) {
        final PrintWriter writer =
            new PrintWriter(new OutputStreamWriter(System.out, StandardCharsets.UTF_8), true);
        writer.print("{\"ok\":");
        writer.print(ok ? "true" : "false");
        writer.print(",\"message\":");
        writer.print(jsonString(message));
        writer.print(",\"rows\":");
        writer.print(jsonArray(rows));
        writer.print(",\"stats\":");
        writer.print(jsonArray(stats));
        writer.print(",\"partitions\":");
        writer.print(jsonArray(partitions));
        writer.print('}');
    }

    private static void writeError(String message) {
        writePayload(false, message == null ? "unknown error" : message, new ArrayList<>(), new ArrayList<>(),
            new ArrayList<>());
    }

    private static String jsonArray(List<Map<String, String>> rows) {
        final StringBuilder builder = new StringBuilder();
        builder.append('[');
        for (int i = 0; i < rows.size(); ++i) {
            if (i > 0) {
                builder.append(',');
            }
            builder.append(jsonObject(rows.get(i)));
        }
        builder.append(']');
        return builder.toString();
    }

    private static String jsonObject(Map<String, String> row) {
        final StringBuilder builder = new StringBuilder();
        builder.append('{');
        int index = 0;
        for (Map.Entry<String, String> entry : row.entrySet()) {
            if (index++ > 0) {
                builder.append(',');
            }
            builder.append(jsonString(entry.getKey()));
            builder.append(':');
            builder.append(jsonString(entry.getValue()));
        }
        builder.append('}');
        return builder.toString();
    }

    private static String jsonString(String value) {
        if (value == null) {
            return "null";
        }
        final StringBuilder builder = new StringBuilder();
        builder.append('\"');
        for (int i = 0; i < value.length(); ++i) {
            final char ch = value.charAt(i);
            switch (ch) {
            case '\\':
                builder.append("\\\\");
                break;
            case '"':
                builder.append("\\\"");
                break;
            case '\n':
                builder.append("\\n");
                break;
            case '\r':
                builder.append("\\r");
                break;
            case '\t':
                builder.append("\\t");
                break;
            default:
                builder.append(ch);
                break;
            }
        }
        builder.append('\"');
        return builder.toString();
    }
}
