#pragma once

#include "adapters/remote/RemoteConnection.h"

#include <QByteArray>
#include <QList>
#include <QPlainTextEdit>
#include <QWidget>

#include <memory>

class QKeyEvent;
class QLabel;
class QMouseEvent;
class QFocusEvent;
class QProcess;
class QPushButton;
class SshClient;

class TerminalTextEdit final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit TerminalTextEdit(QWidget *parent = nullptr);

signals:
    void inputBytes(const QByteArray &bytes);
    void interruptRequested();

protected:
    void focusInEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

// 嵌入式 Linux 远程终端：常驻 `ssh -tt` 进程，实时流式输出，可持续输入。
// 复用 SshClient 的认证/连接参数（known_hosts、SSH_ASKPASS）。
class RemoteTerminalWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteTerminalWidget(RemoteConnectionContext connectionContext, QWidget *parent = nullptr);
    ~RemoteTerminalWidget() override;

    // 启动会话；若进程已在运行则忽略。
    void start();
    void sendCommand(const QString &command);
    QList<QWidget *> takeToolbarWidgets();

private slots:
    void onReadyRead();
    void onProcessError();
    void onProcessFinished();
    void sendInterrupt();
    void reconnect();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void buildUi();
    void appendOutput(const QString &text);
    void appendNotice(const QString &text);
    void writeToProcess(const QByteArray &data);
    void setStatus(const QString &text, const QString &level);
    void stopProcess();

    RemoteConnectionContext m_context;
    std::unique_ptr<SshClient> m_client;
    QProcess *m_process = nullptr;

    QLabel *m_statusDot = nullptr;
    QLabel *m_statusText = nullptr;
    TerminalTextEdit *m_output = nullptr;
    QPushButton *m_reconnectButton = nullptr;
    QPushButton *m_interruptButton = nullptr;
};
