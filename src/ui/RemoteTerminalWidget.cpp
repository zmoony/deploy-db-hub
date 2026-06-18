#include "ui/RemoteTerminalWidget.h"

#include "adapters/ssh/SshClient.h"
#include "ui/PageLayout.h"
#include "ui/TerminalStream.h"

#include <QFont>
#include <QFontDatabase>
#include <QFocusEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QClipboard>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyle>
#include <QTextCursor>
#include <QVBoxLayout>

namespace {

void applyStatusDotShadow(QLabel *dot, const QString &level)
{
    if (dot == nullptr) {
        return;
    }

    if (level == QStringLiteral("ok")) {
        auto *shadow = new QGraphicsDropShadowEffect(dot);
        shadow->setBlurRadius(8);
        shadow->setOffset(0, 0);
        shadow->setColor(QColor(34, 197, 94, 180));
        dot->setGraphicsEffect(shadow);
        return;
    }
    if (level == QStringLiteral("error")) {
        auto *shadow = new QGraphicsDropShadowEffect(dot);
        shadow->setBlurRadius(8);
        shadow->setOffset(0, 0);
        shadow->setColor(QColor(239, 68, 68, 180));
        dot->setGraphicsEffect(shadow);
        return;
    }
    dot->setGraphicsEffect(nullptr);
}

}

TerminalTextEdit::TerminalTextEdit(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setReadOnly(false);
    setFocusPolicy(Qt::StrongFocus);
    setCursorWidth(8);
    setUndoRedoEnabled(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewport()->setCursor(Qt::IBeamCursor);
}

void TerminalTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->matches(QKeySequence::Copy)) {
        if (!textCursor().hasSelection()) {
            emit interruptRequested();
            return;
        }
        QPlainTextEdit::keyPressEvent(event);
        return;
    }
    if (event->matches(QKeySequence::Paste)) {
        const QClipboard *clipboard = QGuiApplication::clipboard();
        if (clipboard != nullptr) {
            const QString text = clipboard->text();
            if (!text.isEmpty()) {
                emit inputBytes(text.toUtf8());
            }
        }
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit inputBytes(QByteArray(1, '\r'));
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        emit inputBytes(QByteArray(1, '\x7f'));
        return;
    }
    if (event->key() == Qt::Key_Tab) {
        emit inputBytes(QByteArray(1, '\t'));
        return;
    }
    if (event->key() == Qt::Key_Left) {
        emit inputBytes(QByteArray("\x1b[D", 3));
        return;
    }
    if (event->key() == Qt::Key_Right) {
        emit inputBytes(QByteArray("\x1b[C", 3));
        return;
    }
    if (event->key() == Qt::Key_Up) {
        emit inputBytes(QByteArray("\x1b[A", 3));
        return;
    }
    if (event->key() == Qt::Key_Down) {
        emit inputBytes(QByteArray("\x1b[B", 3));
        return;
    }
    const QString text = event->text();
    if (!text.isEmpty() && text.at(0).unicode() >= 0x20) {
        emit inputBytes(text.toUtf8());
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void TerminalTextEdit::focusInEvent(QFocusEvent *event)
{
    QPlainTextEdit::focusInEvent(event);
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void TerminalTextEdit::mousePressEvent(QMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    QPlainTextEdit::mousePressEvent(event);
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
    }
}

RemoteTerminalWidget::RemoteTerminalWidget(RemoteConnectionContext connectionContext, QWidget *parent)
    : QWidget(parent)
    , m_context(std::move(connectionContext))
    , m_client(std::make_unique<SshClient>(m_context))
{
    buildUi();
}

RemoteTerminalWidget::~RemoteTerminalWidget()
{
    stopProcess();
}

void RemoteTerminalWidget::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_statusDot = new QLabel;
    m_statusDot->setObjectName(QStringLiteral("remoteStatusDot"));
    m_statusDot->setFixedSize(8, 8);
    auto *title = new QLabel(QStringLiteral("终端"));
    title->setObjectName(QStringLiteral("terminalToolbarTitle"));
    title->setProperty("terminalToolbarTitle", true);
    m_statusText = new QLabel;
    m_statusText->setObjectName(QStringLiteral("remoteStatusLabel"));
    m_interruptButton = new QPushButton(QStringLiteral("Ctrl+C"));
    m_interruptButton->setObjectName(QStringLiteral("terminalToolbarButton"));
    m_reconnectButton = new QPushButton(QStringLiteral("重连"));
    m_reconnectButton->setObjectName(QStringLiteral("terminalToolbarButton"));
    connect(m_interruptButton, &QPushButton::clicked, this, &RemoteTerminalWidget::sendInterrupt);
    connect(m_reconnectButton, &QPushButton::clicked, this, &RemoteTerminalWidget::reconnect);
    title->setParent(this);

    m_output = new TerminalTextEdit;
    m_output->setObjectName(QStringLiteral("terminalOutput"));
    m_output->setMaximumBlockCount(5000);
    m_output->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    connect(m_output, &TerminalTextEdit::inputBytes, this, &RemoteTerminalWidget::writeToProcess);
    connect(m_output, &TerminalTextEdit::interruptRequested, this, &RemoteTerminalWidget::sendInterrupt);
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    m_output->setFont(monoFont);
    layout->addWidget(m_output, 1);

    setStatus(QStringLiteral("未连接"), QStringLiteral("pending"));
}

QList<QWidget *> RemoteTerminalWidget::takeToolbarWidgets()
{
    QList<QWidget *> widgets;
    if (m_statusDot != nullptr) {
        widgets.append(m_statusDot);
    }
    const auto labels = findChildren<QLabel *>();
    for (QLabel *label : labels) {
        if (label->property("terminalToolbarTitle").toBool()) {
            widgets.append(label);
            break;
        }
    }
    if (m_statusText != nullptr) {
        widgets.append(m_statusText);
    }
    if (m_interruptButton != nullptr) {
        widgets.append(m_interruptButton);
    }
    if (m_reconnectButton != nullptr) {
        widgets.append(m_reconnectButton);
    }
    for (QWidget *widget : widgets) {
        widget->setParent(nullptr);
    }
    return widgets;
}

void RemoteTerminalWidget::start()
{
    if (m_process != nullptr) {
        return;
    }

    SshInteractiveInvocation invocation;
    QString error;
    if (!m_client->buildInteractiveInvocation(&invocation, &error)) {
        appendNotice(QStringLiteral("无法启动终端：%1").arg(error));
        setStatus(QStringLiteral("启动失败"), QStringLiteral("error"));
        return;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->setProcessEnvironment(invocation.env);
    connect(m_process, &QProcess::readyReadStandardOutput, this, &RemoteTerminalWidget::onReadyRead);
    connect(m_process, &QProcess::errorOccurred, this, &RemoteTerminalWidget::onProcessError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RemoteTerminalWidget::onProcessFinished);

    setStatus(QStringLiteral("正在连接…"), QStringLiteral("pending"));
    m_process->start(invocation.program, invocation.args, QIODevice::ReadWrite | QIODevice::Unbuffered);
    m_output->setFocus();
}

void RemoteTerminalWidget::onReadyRead()
{
    if (m_process == nullptr) {
        return;
    }
    const QByteArray data = m_process->readAllStandardOutput();
    if (data.isEmpty()) {
        return;
    }
    if (m_statusText->text() != QStringLiteral("已连接")) {
        setStatus(QStringLiteral("已连接"), QStringLiteral("ok"));
    }
    appendOutput(QString::fromUtf8(data));
}

void RemoteTerminalWidget::onProcessError()
{
    if (m_process == nullptr) {
        return;
    }
    appendNotice(QStringLiteral("终端进程错误：%1").arg(m_process->errorString()));
    setStatus(QStringLiteral("错误"), QStringLiteral("error"));
}

void RemoteTerminalWidget::onProcessFinished()
{
    appendNotice(QStringLiteral("\n[会话已结束]"));
    setStatus(QStringLiteral("已断开"), QStringLiteral("error"));
    if (m_process != nullptr) {
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void RemoteTerminalWidget::sendCommand(const QString &command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }
    writeToProcess(command.toUtf8() + '\r');
}

void RemoteTerminalWidget::sendInterrupt()
{
    if (m_process != nullptr && m_process->state() == QProcess::Running) {
        m_process->write(QByteArray(1, '\x03'));
    }
    m_output->setFocus();
}

void RemoteTerminalWidget::reconnect()
{
    stopProcess();
    m_output->clear();
    start();
}

bool RemoteTerminalWidget::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}

void RemoteTerminalWidget::writeToProcess(const QByteArray &data)
{
    if (m_process == nullptr || m_process->state() != QProcess::Running) {
        appendNotice(QStringLiteral("终端未连接，点击「重连」。"));
        return;
    }
    m_process->write(data);
}

void RemoteTerminalWidget::appendOutput(const QString &text)
{
    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    TerminalStream::appendToCursor(cursor, text);
    m_output->setTextCursor(cursor);
    if (auto *bar = m_output->verticalScrollBar()) {
        bar->setValue(bar->maximum());
    }
}

void RemoteTerminalWidget::appendNotice(const QString &text)
{
    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text.endsWith(QLatin1Char('\n')) ? text : text + QLatin1Char('\n'));
    m_output->setTextCursor(cursor);
    if (auto *bar = m_output->verticalScrollBar()) {
        bar->setValue(bar->maximum());
    }
}

void RemoteTerminalWidget::setStatus(const QString &text, const QString &level)
{
    if (m_statusText != nullptr) {
        m_statusText->setText(text);
    }
    if (m_statusDot != nullptr) {
        m_statusDot->setProperty("level", level);
        m_statusDot->style()->unpolish(m_statusDot);
        m_statusDot->style()->polish(m_statusDot);
        applyStatusDotShadow(m_statusDot, level);
    }
}

void RemoteTerminalWidget::stopProcess()
{
    if (m_process == nullptr) {
        return;
    }
    QProcess *process = m_process;
    m_process = nullptr;
    process->disconnect(this);
    if (process->state() != QProcess::NotRunning) {
        process->terminate();
        if (!process->waitForFinished(2000)) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
    process->deleteLater();
}
