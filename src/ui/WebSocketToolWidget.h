#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class MiniWebSocket;

class WebSocketToolWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit WebSocketToolWidget(QWidget *parent = nullptr);

private:
    void appendLog(const QString &tag, const QString &text);
    void updateConnectionState(bool connected);

    MiniWebSocket *m_socket = nullptr;
    QLineEdit *m_url = nullptr;
    QLineEdit *m_message = nullptr;
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QLabel *m_status = nullptr;
};
