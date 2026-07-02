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
#include <QTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStyle>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

namespace {

QFont terminalMonoFont()
{
    static const QStringList preferredFamilies = {
        QStringLiteral("Cascadia Mono"),
        QStringLiteral("Cascadia Code"),
        QStringLiteral("JetBrains Mono"),
        QStringLiteral("Consolas"),
        QStringLiteral("SF Mono"),
        QStringLiteral("Menlo"),
        QStringLiteral("Courier New"),
    };
    QFont font;
    for (const QString &family : preferredFamilies) {
        if (QFontDatabase().hasFamily(family)) {
            font.setFamily(family);
            break;
        }
    }
    if (font.family().isEmpty()) {
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
    font.setPointSize(13);
    font.setStyleHint(QFont::Monospace);
    return font;
}

QTextBlockFormat terminalBlockFormat()
{
    QTextBlockFormat format;
    format.setLineHeight(160, QTextBlockFormat::ProportionalHeight);
    return format;
}

void trimTerminalDocument(QTextDocument *document, int maxBlocks)
{
    if (document == nullptr || document->blockCount() <= maxBlocks) {
        return;
    }
    const int blocksToRemove = document->blockCount() - maxBlocks;
    for (int i = 0; i < blocksToRemove; ++i) {
        QTextCursor cursor(document);
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        if (!cursor.atEnd()) {
            cursor.deleteChar();
        }
    }
}

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
    : QTextEdit(parent)
{
    setReadOnly(false);
    setAcceptRichText(true);
    setFocusPolicy(Qt::StrongFocus);
    setCursorWidth(2);
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
        QTextEdit::keyPressEvent(event);
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
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit inputBytes(QByteArray(1, '\r'));
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Backspace) {
        emit inputBytes(QByteArray(1, '\x7f'));
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Tab) {
        emit inputBytes(QByteArray(1, '\t'));
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Left) {
        emit inputBytes(QByteArray("\x1b[D", 3));
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Left);
        setTextCursor(cursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Right) {
        emit inputBytes(QByteArray("\x1b[C", 3));
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Right);
        setTextCursor(cursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Up) {
        emit inputBytes(QByteArray("\x1b[A", 3));
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Up);
        setTextCursor(cursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Down) {
        emit inputBytes(QByteArray("\x1b[B", 3));
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Down);
        setTextCursor(cursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Home) {
        emit inputBytes(QByteArray("\x1b[H", 3));
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine);
        setTextCursor(cursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_End) {
        emit inputBytes(QByteArray("\x1b[F", 3));
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::EndOfLine);
        setTextCursor(cursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete) {
        emit inputBytes(QByteArray("\x1b[3~", 4));
        event->accept();
        return;
    }
    const QString text = event->text();
    if (!text.isEmpty() && text.at(0).unicode() >= 0x20) {
        emit inputBytes(text.toUtf8());
        event->accept();
        return;
    }
    QTextEdit::keyPressEvent(event);
}

void TerminalTextEdit::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);
}

void TerminalTextEdit::mousePressEvent(QMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    QTextEdit::mousePressEvent(event);
}

void TerminalTextEdit::mouseReleaseEvent(QMouseEvent *event)
{
    QTextEdit::mouseReleaseEvent(event);
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
    m_output->setLineWrapMode(QTextEdit::WidgetWidth);
    m_output->setFont(terminalMonoFont());
    {
        QTextCursor cursor = m_output->textCursor();
        cursor.mergeBlockFormat(terminalBlockFormat());
        m_output->setTextCursor(cursor);
    }
    connect(m_output, &TerminalTextEdit::inputBytes, this, &RemoteTerminalWidget::writeToProcess);
    connect(m_output, &TerminalTextEdit::interruptRequested, this, &RemoteTerminalWidget::sendInterrupt);
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

void RemoteTerminalWidget::requestCurrentWorkingDirectory()
{
    if (m_process == nullptr || m_process->state() != QProcess::Running) {
        appendNotice(QStringLiteral("终端未连接，无法获取路径。"));
        return;
    }
    m_pendingPwdRequest = true;
    writeToProcess(QByteArray("echo __DH_PWD_S__$(pwd)__DH_PWD_E__\r"));
}

void RemoteTerminalWidget::checkForDirectoryMarker(const QString &rawText)
{
    if (!m_pendingPwdRequest) {
        return;
    }
    const QString plain = TerminalStream::stripAnsi(rawText);
    int searchFrom = 0;
    while (searchFrom < plain.size()) {
        const int startIdx = plain.indexOf(QStringLiteral("__DH_PWD_S__"), searchFrom);
        if (startIdx < 0) {
            return;
        }
        const int pathStart = startIdx + 12; // length of "__DH_PWD_S__"
        const int endIdx = plain.indexOf(QStringLiteral("__DH_PWD_E__"), pathStart);
        if (endIdx < 0) {
            return;
        }
        const QString path = plain.mid(pathStart, endIdx - pathStart).trimmed();
        // 跳过 shell 回显的命令行（包含 $( 等shell语法），只匹配执行结果
        if (!path.isEmpty() && !path.contains(QLatin1String("$("))) {
            m_pendingPwdRequest = false;
            emit currentDirectoryReceived(path);
            return;
        }
        searchFrom = endIdx + 12; // length of "__DH_PWD_E__"
    }
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
    checkForDirectoryMarker(text);
    QTextCursor cursor = m_output->textCursor();
    // 含 \n 的文本是新行输出（如命令结果、MOTD），追加到文末
    // 不含 \n 的文本是行内编辑（如 readline 回显），在当前光标位置插入
    // 但如果光标不在最后一行（如鼠标点击了历史区域），回退到文末
    if (text.contains(QLatin1Char('\n')) || cursor.block() != m_output->document()->lastBlock()) {
        cursor.movePosition(QTextCursor::End);
    }
    cursor.mergeBlockFormat(terminalBlockFormat());
    TerminalStream::appendToCursor(cursor, text);
    m_output->setTextCursor(cursor);
    trimTerminalDocument(m_output->document(), 5000);
    if (auto *bar = m_output->verticalScrollBar()) {
        bar->setValue(bar->maximum());
    }
}

void RemoteTerminalWidget::appendNotice(const QString &text)
{
    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.mergeBlockFormat(terminalBlockFormat());
    cursor.insertText(text.endsWith(QLatin1Char('\n')) ? text : text + QLatin1Char('\n'));
    trimTerminalDocument(m_output->document(), 5000);
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
