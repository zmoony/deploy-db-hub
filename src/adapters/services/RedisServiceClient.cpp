#include "adapters/services/RedisServiceClient.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QHash>
#include <QTcpSocket>
#include <QTimer>

namespace {

bool waitSocketReadable(QTcpSocket &socket, int timeoutMs)
{
    if (socket.bytesAvailable() > 0) {
        return true;
    }
    if (socket.state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&socket, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::errorOccurred, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    return socket.bytesAvailable() > 0;
}

bool waitSocketConnected(QTcpSocket &socket, int timeoutMs)
{
    if (socket.state() == QAbstractSocket::ConnectedState) {
        return true;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&socket, &QTcpSocket::connected, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::errorOccurred, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    return socket.state() == QAbstractSocket::ConnectedState;
}

bool waitSocketBytesWritten(QTcpSocket &socket, int timeoutMs)
{
    if (socket.bytesToWrite() == 0) {
        return true;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&socket, &QTcpSocket::bytesWritten, &loop, [&loop, &socket]() {
        if (socket.bytesToWrite() == 0) {
            loop.quit();
        }
    });
    QObject::connect(&socket, &QTcpSocket::errorOccurred, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    return socket.bytesToWrite() == 0;
}

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
        if (!waitSocketConnected(m_socket, 5000)) {
            return {false, m_socket.errorString(), {}, {}};
        }
        if (!m_endpoint.password.isEmpty()) {
            if (!m_endpoint.username.isEmpty()) {
                const ServiceResult auth =
                    command({QStringLiteral("AUTH"), m_endpoint.username, m_endpoint.password});
                if (!auth.ok) {
                    return auth;
                }
            } else {
                const ServiceResult auth = command({QStringLiteral("AUTH"), m_endpoint.password});
                if (!auth.ok) {
                    return auth;
                }
            }
        } else if (!m_endpoint.username.isEmpty()) {
            const ServiceResult auth = command({QStringLiteral("AUTH"), m_endpoint.username, QString()});
            if (!auth.ok) {
                return auth;
            }
        }
        if (m_endpoint.redisDatabase >= 0 && m_endpoint.redisDatabase <= 15) {
            const ServiceResult selected =
                command({QStringLiteral("SELECT"), QString::number(m_endpoint.redisDatabase)});
            if (!selected.ok) {
                return selected;
            }
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
        if (m_socket.write(payload) != payload.size() || !waitSocketBytesWritten(m_socket, 3000)) {
            return {false, QStringLiteral("写入 Redis 命令失败"), {}, {}};
        }
        return readReply(true);
    }

    ServiceResult scanKeys(const QString &pattern, int limit, QStringList *keysOut)
    {
        QString cursor = QStringLiteral("0");
        int collected = 0;
        do {
            const ServiceResult page = command(
                {QStringLiteral("SCAN"), cursor, QStringLiteral("MATCH"), pattern, QStringLiteral("COUNT"),
                 QStringLiteral("200")});
            if (!page.ok) {
                return page;
            }
            if (page.rows.size() < 2) {
                return {false, QStringLiteral("SCAN 响应格式异常"), {}, {}};
            }
            cursor = page.rows.at(0).value(QStringLiteral("value")).toString();
            for (int i = 1; i < page.rows.size(); ++i) {
                const QString key = page.rows.at(i).value(QStringLiteral("value")).toString();
                if (key.isEmpty()) {
                    continue;
                }
                keysOut->append(key);
                if (++collected >= limit) {
                    return {true, {}, {}, {}};
                }
            }
        } while (cursor != QStringLiteral("0"));
        return {true, {}, {}, {}};
    }

private:
    static constexpr int kIoTimeoutMs = 5000;

    bool appendSocketBytes(int timeoutMs)
    {
        if (m_socket.bytesAvailable() > 0) {
            m_readBuffer.append(m_socket.readAll());
            return true;
        }
        if (!waitSocketReadable(m_socket, timeoutMs)) {
            return false;
        }
        m_readBuffer.append(m_socket.readAll());
        return true;
    }

    bool readExactBytes(int size, int timeoutMs)
    {
        QElapsedTimer timer;
        timer.start();
        while (m_readBuffer.size() < size) {
            const int remaining = timeoutMs - static_cast<int>(timer.elapsed());
            if (remaining <= 0) {
                return false;
            }
            if (!appendSocketBytes(remaining)) {
                return false;
            }
        }
        return true;
    }

    QByteArray readLine(int timeoutMs, bool *ok)
    {
        QElapsedTimer timer;
        timer.start();
        while (true) {
            const int newline = m_readBuffer.indexOf('\n');
            if (newline >= 0) {
                QByteArray line = m_readBuffer.left(newline);
                m_readBuffer.remove(0, newline + 1);
                if (line.endsWith('\r')) {
                    line.chop(1);
                }
                if (ok != nullptr) {
                    *ok = true;
                }
                return line;
            }

            const int remaining = timeoutMs - static_cast<int>(timer.elapsed());
            if (remaining <= 0) {
                if (ok != nullptr) {
                    *ok = false;
                }
                return {};
            }
            if (!appendSocketBytes(remaining)) {
                if (ok != nullptr) {
                    *ok = false;
                }
                return {};
            }
        }
    }

    ServiceResult readReply(bool flattenNestedArrays)
    {
        bool ok = false;
        const QByteArray line = readLine(kIoTimeoutMs, &ok);
        if (!ok) {
            return {false, QStringLiteral("读取 Redis 响应超时"), {}, {}};
        }
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
            if (!readExactBytes(len + 2, kIoTimeoutMs)) {
                return {false, QStringLiteral("读取 bulk 超时"), {}, {}};
            }
            const QByteArray bulk = m_readBuffer.left(len);
            m_readBuffer.remove(0, len + 2);
            return {true, QString::fromUtf8(bulk), {}, {}};
        }
        if (prefix == '*') {
            const int count = line.mid(1).trimmed().toInt();
            if (count < 0) {
                return {true, {}, {}, {}};
            }
            ServiceResult result{true, {}, {}, {}};
            for (int i = 0; i < count; ++i) {
                const ServiceResult item = readReply(flattenNestedArrays);
                if (!item.ok) {
                    return item;
                }
                if (flattenNestedArrays && !item.rows.isEmpty()) {
                    result.rows.append(item.rows);
                } else {
                    result.rows.append(QJsonObject{{QStringLiteral("value"), item.message}});
                    if (!item.rows.isEmpty()) {
                        result.rows.append(item.rows);
                    }
                }
            }
            return result;
        }
        return {false, QStringLiteral("未知 RESP 类型"), {}, {}};
    }

    ServiceEndpoint m_endpoint;
    QTcpSocket m_socket;
    QByteArray m_readBuffer;
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

QString keySize(RedisRespSession &session, const QString &key, const QString &type)
{
    if (type == QStringLiteral("string")) {
        return session.command({QStringLiteral("STRLEN"), key}).message;
    }
    if (type == QStringLiteral("list")) {
        return session.command({QStringLiteral("LLEN"), key}).message;
    }
    if (type == QStringLiteral("hash")) {
        return session.command({QStringLiteral("HLEN"), key}).message;
    }
    if (type == QStringLiteral("set")) {
        return session.command({QStringLiteral("SCARD"), key}).message;
    }
    if (type == QStringLiteral("zset")) {
        return session.command({QStringLiteral("ZCARD"), key}).message;
    }
    const ServiceResult memory = session.command({QStringLiteral("MEMORY"), QStringLiteral("USAGE"), key});
    if (memory.ok && !memory.message.isEmpty()) {
        return memory.message;
    }
    return QStringLiteral("-");
}

QHash<QString, QString> parseInfoMap(const QString &infoText)
{
    QHash<QString, QString> map;
    for (const QString &line : infoText.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon <= 0) {
            continue;
        }
        map.insert(line.left(colon).trimmed(), line.mid(colon + 1).trimmed());
    }
    return map;
}

}

ServiceResult RedisServiceClient::ping(const ServiceEndpoint &endpoint)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }
    const ServiceResult pong = session.command({QStringLiteral("PING")});
    if (!pong.ok) {
        return pong;
    }
    const ServiceResult info = session.command({QStringLiteral("INFO"), QStringLiteral("server")});
    if (!info.ok) {
        return {true, QStringLiteral("Redis 可达 (%1)").arg(pong.message), {}, {}};
    }
    const QHash<QString, QString> fields = parseInfoMap(info.message);
    return {true,
            QStringLiteral("Redis %1 (%2)")
                .arg(fields.value(QStringLiteral("redis_version"), QStringLiteral("unknown")),
                     fields.value(QStringLiteral("os"), endpoint.host)),
            {},
            {}};
}

ServiceResult RedisServiceClient::serverInfo(const ServiceEndpoint &endpoint)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }
    const ServiceResult info = session.command({QStringLiteral("INFO")});
    if (!info.ok) {
        return info;
    }
    const QHash<QString, QString> fields = parseInfoMap(info.message);
    return {true,
            info.message,
            {},
            {QJsonObject{
                {QStringLiteral("version"), fields.value(QStringLiteral("redis_version"))},
                {QStringLiteral("mode"), fields.value(QStringLiteral("redis_mode"))},
                {QStringLiteral("memory"), fields.value(QStringLiteral("used_memory_human"))},
                {QStringLiteral("clients"), fields.value(QStringLiteral("connected_clients"))},
                {QStringLiteral("status"), QStringLiteral("运行中")}
            }}};
}

ServiceResult RedisServiceClient::listKeys(const ServiceEndpoint &endpoint, const QString &pattern, int limit)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }

    const QString matchPattern = pattern.trimmed().isEmpty() ? QStringLiteral("*") : pattern.trimmed();
    QStringList keys;
    const ServiceResult scanned = session.scanKeys(matchPattern, limit, &keys);
    if (!scanned.ok) {
        return scanned;
    }

    ServiceResult result{true, {}, {}, {}};
    for (const QString &key : keys) {
        const ServiceResult type = session.command({QStringLiteral("TYPE"), key});
        const ServiceResult ttl = session.command({QStringLiteral("TTL"), key});
        result.rows.append(QJsonObject{
            {QStringLiteral("key"), key},
            {QStringLiteral("type"), type.message},
            {QStringLiteral("ttl"), formatTtl(ttl.message)},
            {QStringLiteral("size"), keySize(session, key, type.message)}
        });
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
        return {true, extractArrayValues(values).join(QStringLiteral("\n")), {}, {}};
    }
    if (type.message == QStringLiteral("hash")) {
        const ServiceResult all = session.command({QStringLiteral("HGETALL"), key});
        if (!all.ok) {
            return all;
        }
        ServiceResult result{true, {}, {}, {}};
        const QStringList values = extractArrayValues(all);
        for (int i = 0; i + 1 < values.size(); i += 2) {
            result.rows.append(QJsonObject{
                {QStringLiteral("field"), values.at(i)},
                {QStringLiteral("value"), values.at(i + 1)}
            });
        }
        return result;
    }
    if (type.message == QStringLiteral("set")) {
        const ServiceResult values = session.command({QStringLiteral("SMEMBERS"), key});
        if (!values.ok) {
            return values;
        }
        ServiceResult result{true, {}, {}, {}};
        for (const QString &member : extractArrayValues(values)) {
            result.rows.append(QJsonObject{{QStringLiteral("value"), member}});
        }
        return result;
    }
    if (type.message == QStringLiteral("zset")) {
        const ServiceResult values = session.command({QStringLiteral("ZRANGE"), key, QStringLiteral("0"), QStringLiteral("20")});
        if (!values.ok) {
            return values;
        }
        ServiceResult result{true, {}, {}, {}};
        for (const QString &member : extractArrayValues(values)) {
            result.rows.append(QJsonObject{{QStringLiteral("value"), member}});
        }
        return result;
    }
    return {true, QStringLiteral("类型: %1").arg(type.message), {}, {}};
}

ServiceResult RedisServiceClient::writeKey(const ServiceEndpoint &endpoint,
                                           const QString &key,
                                           const QString &value,
                                           int ttlSec)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }
    ServiceResult result = session.command({QStringLiteral("SET"), key, value});
    if (!result.ok) {
        return result;
    }
    if (ttlSec > 0) {
        result = session.command({QStringLiteral("EXPIRE"), key, QString::number(ttlSec)});
        if (!result.ok) {
            return result;
        }
    }
    return {true, QStringLiteral("已写入 Key: %1").arg(key), {}, {}};
}

ServiceResult RedisServiceClient::deleteKey(const ServiceEndpoint &endpoint, const QString &key)
{
    RedisRespSession session(endpoint);
    const ServiceResult connected = session.connectAndAuth();
    if (!connected.ok) {
        return connected;
    }
    const ServiceResult result = session.command({QStringLiteral("DEL"), key});
    if (!result.ok) {
        return result;
    }
    return {true, QStringLiteral("已删除 Key: %1").arg(key), {}, {}};
}
