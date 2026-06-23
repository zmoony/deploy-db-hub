#pragma once

#include <QByteArray>
#include <QObject>
#include <QUrl>

class QAbstractSocket;

// Minimal RFC6455 WebSocket client built on QTcpSocket / QSslSocket so the
// project does not need the optional Qt WebSockets module. Supports text
// frames, ping/pong and close handshake; fragmentation is reassembled for
// text/binary continuation frames.
class MiniWebSocket final : public QObject
{
    Q_OBJECT

public:
    explicit MiniWebSocket(QObject *parent = nullptr);
    ~MiniWebSocket() override;

    void open(const QUrl &url);
    void sendText(const QString &text);
    void close();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString &message);
    void errorOccurred(const QString &message);

private:
    void sendHandshake();
    void onReadyRead();
    void processFrames();
    void writeFrame(quint8 opcode, const QByteArray &payload);
    void cleanupSocket();

    QAbstractSocket *m_socket = nullptr;
    QByteArray m_buffer;
    QByteArray m_fragment;
    quint8 m_fragmentOpcode = 0;
    QString m_host;
    QString m_resource;
    QByteArray m_acceptKey;
    bool m_secure = false;
    bool m_handshakeDone = false;
};
