#include "adapters/services/RedisServiceClient.h"

#include <QTcpSocket>

namespace {

class RedisRespSession
{
public:
    explicit RedisRespSession(const ServiceEndpoint &endpoint)
        : m_endpoint(endpoint)
    {
    }

    ServiceResult connectAndAuth()
    {
        m_socket.connectToHost(m_endpoint.host, static_cast<quint16>(m_endpoint.port));
        if (!m_socket.waitForConnected(5000)) {
            return {false, m_socket.errorString(), {}, {}};
        }
        if (!m_endpoint.password.isEmpty()) {
            return command({QStringLiteral("AUTH"), m_endpoint.password});
        }
        return {true, QStringLiteral("connected"), {}, {}};
    }

    ServiceResult command(const QStringList &args)
    {
        QByteArray payload;
        payload.append('*').append(QByteArray::number(args.size())).append("\r\n");
        for (const QString &arg : args) {
            const QByteArray bytes = arg.toUtf8();
            payload.append('$').append(QByteArray::number(bytes.size())).append("\r\n");
            payload.append(bytes).append("\r\n");
        }
        if (m_socket.write(payload) != payload.size() || !m_socket.waitForBytesWritten(3000)) {
            return {false, QStringLiteral("写入 Redis 命令失败"), {}, {}};
        }
        return readReply();
    }

private:
    bool waitReadable()
    {
        return m_socket.waitForReadyRead(5000);
    }

    QByteArray readLine()
    {
        return m_socket.readLine();
    }

    ServiceResult readBulkString()
    {
        if (!waitReadable()) {
            return {false, QStringLiteral("读取 Redis 响应超时"), {}, {}};
        }
        const QByteArray line = readLine();
        if (line.isEmpty() || line.at(0) != '$') {
            return {false, QStringLiteral("期望 bulk string"), {}, {}};
        }
        const int len = line.mid(1).trimmed().toInt();
        if (len < 0) {
            return {true, {}, {}, {}};
        }
        QByteArray bulk;
        while (bulk.size() < len) {
            if (!waitReadable()) {
                return {false, QStringLiteral("读取 bulk 超时"), {}, {}};
            }
            bulk.append(m_socket.read(len - bulk.size()));
        }
        m_socket.read(2);
        return {true, QString::fromUtf8(bulk), {}, {}};
    }

    ServiceResult readReply()
    {
        if (!waitReadable()) {
            return {false, QStringLiteral("读取 Redis 响应超时"), {}, {}};
        }
        const QByteArray line = readLine();
        if (line.isEmpty()) {
            return {false, QStringLiteral("空响应"), {}, {}};
        }

        const char prefix = line.at(0);
        if (prefix == '+') {
            return {true, QString::fromUtf8(line.mid(1).trimmed()), {}, {}};
        }
        if (prefix == '-') {
            return {false, QString::fromUtf8(line.mid(1).trimmed()), {}, {}};
        }
        if (prefix == ':') {
            return {true, QString::fromUtf8(line.mid(1).trimmed()), {}, {}};
        }
        if (prefix == '$') {
            const int len = line.mid(1).trimmed().toInt();
            if (len < 0) {
                return {true, {}, {}, {}};
            }
            QByteArray bulk;
            while (bulk.size() < len) {
                if (!waitReadable()) {
                    return {false, QStringLiteral("读取 bulk 超时"), {}, {}};
                }
                bulk.append(m_socket.read(len - bulk.size()));
            }
            m_socket.read(2);
            return {true, QString::fromUtf8(bulk), {}, {}};
        }
        if (prefix == '*') {
            const int count = line.mid(1).trimmed().toInt();
            ServiceResult result{true, {}, {}, {}};
            for (int i = 0; i < count; ++i) {
                const ServiceResult item = readReply();
                if (!item.ok) {
                    return item;
                }
                result.rows.append(QJsonObject{{QStringLiteral("value"), item.message}});
            }
            return result;
        }
        return {false, QStringLiteral("未知 RESP 类型"), {}, {}};
    }

    ServiceEndpoint m_endpoint;
    QTcpSocket m_socket;
};

QString formatTtl(const QString &ttlText)
{
    const qint64 ttl = ttlText.toLongLong();
    if (ttl < 0) {
        return QStringLiteral("永久");
    }
    return QStringLiteral("%1s").arg(ttl);
}

QStringList extractArrayValues(const ServiceResult &arrayReply)
{
    QStringList values;
    for (const QJsonObject &row : arrayReply.rows) {
        values.append(row.value(QStringLiteral("value")).toString());
    }
    return values;
}

}

ServiceResult RedisServiceClient::ping(const ServiceEndpoint &endpoint)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }
    return session.command({QStringLiteral("PING")});
}

ServiceResult RedisServiceClient::listKeys(const ServiceEndpoint &endpoint, const QString &pattern, int limit)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }

    const QString matchPattern = pattern.trimmed().isEmpty() ? QStringLiteral("*") : pattern.trimmed();
    const ServiceResult keys = session.command({QStringLiteral("KEYS"), matchPattern});
    if (!keys.ok) {
        return keys;
    }

    ServiceResult result{true, {}, {}, {}};
    int count = 0;
    for (const QJsonObject &item : keys.rows) {
        const QString key = item.value(QStringLiteral("value")).toString();
        const ServiceResult type = session.command({QStringLiteral("TYPE"), key});
        const ServiceResult ttl = session.command({QStringLiteral("TTL"), key});
        const ServiceResult memory = session.command({QStringLiteral("STRLEN"), key});
        result.rows.append(QJsonObject{
            {QStringLiteral("key"), key},
            {QStringLiteral("type"), type.message},
            {QStringLiteral("ttl"), formatTtl(ttl.message)},
            {QStringLiteral("size"), memory.message}
        });
        if (++count >= limit) {
            break;
        }
    }
    return result;
}

ServiceResult RedisServiceClient::readKey(const ServiceEndpoint &endpoint, const QString &key)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }

    const ServiceResult type = session.command({QStringLiteral("TYPE"), key});
    if (!type.ok) {
        return type;
    }

    if (type.message == QStringLiteral("string")) {
        return session.command({QStringLiteral("GET"), key});
    }
    if (type.message == QStringLiteral("list")) {
        const ServiceResult values = session.command({QStringLiteral("LRANGE"), key, QStringLiteral("0"), QStringLiteral("20")});
        if (!values.ok) {
            return values;
        }
        QStringList lines = extractArrayValues(values);
        return {true, lines.join(QStringLiteral("\n")), {}, {}};
    }
    if (type.message == QStringLiteral("hash")) {
        const ServiceResult all = session.command({QStringLiteral("HGETALL"), key});
        if (!all.ok) {
            return all;
        }
        QStringList lines;
        const QStringList values = extractArrayValues(all);
        for (int i = 0; i + 1 < values.size(); i += 2) {
            lines.append(QStringLiteral("%1 = %2").arg(values.at(i), values.at(i + 1)));
        }
        return {true, lines.join(QStringLiteral("\n")), {}, {}};
    }
    return {true, QStringLiteral("类型: %1").arg(type.message), {}, {}};
}
