#include "ui/MiniWebSocket.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QTcpSocket>

#ifndef QT_NO_SSL
#include <QSslSocket>
#endif

MiniWebSocket::MiniWebSocket(QObject *parent)
    : QObject(parent)
{
}

MiniWebSocket::~MiniWebSocket()
{
    cleanupSocket();
}

bool MiniWebSocket::isConnected() const
{
    return m_handshakeDone && m_socket != nullptr;
}

void MiniWebSocket::open(const QUrl &url)
{
    cleanupSocket();
    m_buffer.clear();
    m_fragment.clear();
    m_fragmentOpcode = 0;
    m_handshakeDone = false;

    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("ws") && scheme != QStringLiteral("wss")) {
        emit errorOccurred(QStringLiteral("仅支持 ws:// 或 wss:// 地址"));
        return;
    }
    m_secure = (scheme == QStringLiteral("wss"));
    const int port = url.port(m_secure ? 443 : 80);
    m_host = url.host();
    if (m_host.isEmpty()) {
        emit errorOccurred(QStringLiteral("地址缺少主机名"));
        return;
    }
    QString path = url.path();
    if (path.isEmpty()) {
        path = QStringLiteral("/");
    }
    if (url.hasQuery()) {
        path += QLatin1Char('?') + url.query(QUrl::FullyEncoded);
    }
    m_resource = path;

    if (m_secure) {
#ifndef QT_NO_SSL
        if (!QSslSocket::supportsSsl()) {
            emit errorOccurred(QStringLiteral("当前环境不支持 TLS，无法连接 wss"));
            return;
        }
        auto *ssl = new QSslSocket(this);
        m_socket = ssl;
        connect(ssl, &QSslSocket::encrypted, this, &MiniWebSocket::sendHandshake);
#else
        emit errorOccurred(QStringLiteral("构建未启用 TLS，无法连接 wss"));
        return;
#endif
    } else {
        auto *tcp = new QTcpSocket(this);
        m_socket = tcp;
        connect(tcp, &QTcpSocket::connected, this, &MiniWebSocket::sendHandshake);
    }

    connect(m_socket, &QAbstractSocket::readyRead, this, &MiniWebSocket::onReadyRead);
    connect(m_socket, &QAbstractSocket::disconnected, this, [this]() {
        const bool wasConnected = m_handshakeDone;
        m_handshakeDone = false;
        if (wasConnected) {
            emit disconnected();
        }
    });
    connect(m_socket, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (m_socket != nullptr) {
            emit errorOccurred(m_socket->errorString());
        }
    });

#ifndef QT_NO_SSL
    if (m_secure) {
        static_cast<QSslSocket *>(m_socket)->connectToHostEncrypted(m_host, static_cast<quint16>(port));
    } else
#endif
    {
        m_socket->connectToHost(m_host, static_cast<quint16>(port));
    }
}

void MiniWebSocket::sendHandshake()
{
    if (m_socket == nullptr) {
        return;
    }
    QByteArray rawKey(16, 0);
    for (int i = 0; i < rawKey.size(); ++i) {
        rawKey[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    const QByteArray key = rawKey.toBase64();
    const QByteArray magic = key + QByteArrayLiteral("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    m_acceptKey = QCryptographicHash::hash(magic, QCryptographicHash::Sha1).toBase64();

    QByteArray request;
    request += "GET " + m_resource.toUtf8() + " HTTP/1.1\r\n";
    request += "Host: " + m_host.toUtf8() + "\r\n";
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Key: " + key + "\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += "\r\n";
    m_socket->write(request);
}

void MiniWebSocket::onReadyRead()
{
    if (m_socket == nullptr) {
        return;
    }
    m_buffer += m_socket->readAll();

    if (!m_handshakeDone) {
        const int headerEnd = m_buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) {
            return;
        }
        const QByteArray header = m_buffer.left(headerEnd);
        m_buffer.remove(0, headerEnd + 4);
        const QByteArray firstLine = header.left(header.indexOf("\r\n"));
        if (!firstLine.contains("101")) {
            emit errorOccurred(QStringLiteral("握手失败：%1").arg(QString::fromUtf8(firstLine)));
            close();
            return;
        }
        m_handshakeDone = true;
        emit connected();
    }

    processFrames();
}

void MiniWebSocket::processFrames()
{
    while (true) {
        if (m_buffer.size() < 2) {
            return;
        }
        const quint8 b0 = static_cast<quint8>(m_buffer.at(0));
        const quint8 b1 = static_cast<quint8>(m_buffer.at(1));
        const bool fin = (b0 & 0x80) != 0;
        const quint8 opcode = b0 & 0x0F;
        const bool masked = (b1 & 0x80) != 0;
        quint64 payloadLen = b1 & 0x7F;
        int offset = 2;

        if (payloadLen == 126) {
            if (m_buffer.size() < offset + 2) {
                return;
            }
            payloadLen = (static_cast<quint8>(m_buffer.at(2)) << 8)
                | static_cast<quint8>(m_buffer.at(3));
            offset += 2;
        } else if (payloadLen == 127) {
            if (m_buffer.size() < offset + 8) {
                return;
            }
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) | static_cast<quint8>(m_buffer.at(offset + i));
            }
            offset += 8;
        }

        QByteArray maskKey;
        if (masked) {
            if (m_buffer.size() < offset + 4) {
                return;
            }
            maskKey = m_buffer.mid(offset, 4);
            offset += 4;
        }

        if (static_cast<quint64>(m_buffer.size()) < offset + payloadLen) {
            return;
        }

        QByteArray payload = m_buffer.mid(offset, static_cast<int>(payloadLen));
        if (masked && maskKey.size() == 4) {
            for (int i = 0; i < payload.size(); ++i) {
                payload[i] = static_cast<char>(payload.at(i) ^ maskKey.at(i % 4));
            }
        }
        m_buffer.remove(0, offset + static_cast<int>(payloadLen));

        if (opcode == 0x8) { // close
            close();
            return;
        }
        if (opcode == 0x9) { // ping
            writeFrame(0x0A, payload);
            continue;
        }
        if (opcode == 0x0A) { // pong
            continue;
        }

        if (opcode == 0x0) {
            m_fragment += payload;
        } else {
            m_fragment = payload;
            m_fragmentOpcode = opcode;
        }

        if (fin) {
            if (m_fragmentOpcode == 0x1) {
                emit textMessageReceived(QString::fromUtf8(m_fragment));
            } else if (m_fragmentOpcode == 0x2) {
                emit textMessageReceived(QStringLiteral("[二进制消息 %1 字节]").arg(m_fragment.size()));
            }
            m_fragment.clear();
            m_fragmentOpcode = 0;
        }
    }
}

void MiniWebSocket::writeFrame(quint8 opcode, const QByteArray &payload)
{
    if (m_socket == nullptr) {
        return;
    }
    QByteArray frame;
    frame.append(static_cast<char>(0x80 | opcode));

    const int len = payload.size();
    if (len < 126) {
        frame.append(static_cast<char>(0x80 | len));
    } else if (len <= 0xFFFF) {
        frame.append(static_cast<char>(0x80 | 126));
        frame.append(static_cast<char>((len >> 8) & 0xFF));
        frame.append(static_cast<char>(len & 0xFF));
    } else {
        frame.append(static_cast<char>(0x80 | 127));
        for (int i = 7; i >= 0; --i) {
            frame.append(static_cast<char>((static_cast<quint64>(len) >> (i * 8)) & 0xFF));
        }
    }

    char mask[4];
    for (char &byte : mask) {
        byte = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    frame.append(mask, 4);

    QByteArray masked = payload;
    for (int i = 0; i < masked.size(); ++i) {
        masked[i] = static_cast<char>(masked.at(i) ^ mask[i % 4]);
    }
    frame.append(masked);

    m_socket->write(frame);
}

void MiniWebSocket::sendText(const QString &text)
{
    if (!isConnected()) {
        emit errorOccurred(QStringLiteral("未连接，无法发送"));
        return;
    }
    writeFrame(0x1, text.toUtf8());
}

void MiniWebSocket::close()
{
    if (m_socket == nullptr) {
        return;
    }
    if (m_handshakeDone) {
        writeFrame(0x8, QByteArray());
    }
    m_socket->disconnectFromHost();
}

void MiniWebSocket::cleanupSocket()
{
    if (m_socket != nullptr) {
        m_socket->disconnect(this);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_handshakeDone = false;
}
