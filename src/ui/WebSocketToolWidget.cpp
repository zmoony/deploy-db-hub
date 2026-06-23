#include "ui/WebSocketToolWidget.h"

#include "ui/MiniWebSocket.h"
#include "ui/PageLayout.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

WebSocketToolWidget::WebSocketToolWidget(QWidget *parent)
    : QWidget(parent)
{
    m_socket = new MiniWebSocket(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
    layout->addWidget(PageLayout::makeHeaderBlock(
        QStringLiteral("WebSocket 测试"),
        QStringLiteral("连接 ws:// 或 wss:// 服务，发送与接收消息；支持连接、断开。"),
        this));

    auto *connectRow = new QWidget(this);
    auto *connectLayout = new QHBoxLayout(connectRow);
    connectLayout->setContentsMargins(0, 0, 0, 0);
    connectLayout->setSpacing(PageLayout::Space8);
    m_url = new QLineEdit(connectRow);
    m_url->setPlaceholderText(QStringLiteral("wss://echo.websocket.events"));
    m_url->setText(QStringLiteral("wss://echo.websocket.events"));
    PageLayout::configureFormInput(m_url);
    m_connectButton = new QPushButton(QStringLiteral("连接"), connectRow);
    m_connectButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    m_disconnectButton = new QPushButton(QStringLiteral("断开"), connectRow);
    m_disconnectButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    m_disconnectButton->setEnabled(false);
    connectLayout->addWidget(m_url, 1);
    connectLayout->addWidget(m_connectButton);
    connectLayout->addWidget(m_disconnectButton);
    layout->addWidget(connectRow);

    m_status = new QLabel(QStringLiteral("未连接"), this);
    m_status->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(m_status);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setPlaceholderText(QStringLiteral("消息日志"));
    m_log->setMinimumHeight(260);
    layout->addWidget(m_log, 1);

    auto *sendRow = new QWidget(this);
    auto *sendLayout = new QHBoxLayout(sendRow);
    sendLayout->setContentsMargins(0, 0, 0, 0);
    sendLayout->setSpacing(PageLayout::Space8);
    m_message = new QLineEdit(sendRow);
    m_message->setPlaceholderText(QStringLiteral("待发送消息，回车或点击发送"));
    PageLayout::configureFormInput(m_message);
    m_sendButton = new QPushButton(QStringLiteral("发送"), sendRow);
    m_sendButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    m_sendButton->setEnabled(false);
    sendLayout->addWidget(m_message, 1);
    sendLayout->addWidget(m_sendButton);
    layout->addWidget(sendRow);

    connect(m_connectButton, &QPushButton::clicked, this, [this]() {
        const QUrl url(m_url->text().trimmed());
        if (!url.isValid() || url.scheme().isEmpty()) {
            appendLog(QStringLiteral("错误"), QStringLiteral("URL 无效"));
            return;
        }
        appendLog(QStringLiteral("系统"), QStringLiteral("正在连接 %1").arg(url.toString()));
        m_status->setText(QStringLiteral("连接中..."));
        m_connectButton->setEnabled(false);
        m_socket->open(url);
    });
    connect(m_disconnectButton, &QPushButton::clicked, this, [this]() {
        m_socket->close();
    });

    auto sendMessage = [this]() {
        const QString text = m_message->text();
        if (text.isEmpty()) {
            return;
        }
        m_socket->sendText(text);
        appendLog(QStringLiteral("发送"), text);
        m_message->clear();
    };
    connect(m_sendButton, &QPushButton::clicked, this, sendMessage);
    connect(m_message, &QLineEdit::returnPressed, this, sendMessage);

    connect(m_socket, &MiniWebSocket::connected, this, [this]() {
        appendLog(QStringLiteral("系统"), QStringLiteral("已连接"));
        updateConnectionState(true);
    });
    connect(m_socket, &MiniWebSocket::disconnected, this, [this]() {
        appendLog(QStringLiteral("系统"), QStringLiteral("连接已断开"));
        updateConnectionState(false);
    });
    connect(m_socket, &MiniWebSocket::textMessageReceived, this, [this](const QString &message) {
        appendLog(QStringLiteral("接收"), message);
    });
    connect(m_socket, &MiniWebSocket::errorOccurred, this, [this](const QString &message) {
        appendLog(QStringLiteral("错误"), message);
        m_status->setText(QStringLiteral("错误：%1").arg(message));
        if (!m_socket->isConnected()) {
            updateConnectionState(false);
        }
    });
}

void WebSocketToolWidget::appendLog(const QString &tag, const QString &text)
{
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    m_log->appendPlainText(QStringLiteral("[%1] %2: %3").arg(stamp, tag, text));
}

void WebSocketToolWidget::updateConnectionState(bool connected)
{
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_sendButton->setEnabled(connected);
    m_status->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
}
