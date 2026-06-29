#include "ui/LogAiAnalysisWidget.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/AiAssistHelper.h"
#include "ui/PageLayout.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr int kStreamFlushIntervalMs = 30;
constexpr int kMaxBubblePreviewChars = 400;

QPushButton *makeActionButton(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setMinimumHeight(36);
    return button;
}

QLabel *createBubbleLabel(QWidget *parent, const QString &objectName)
{
    auto *label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setScaledContents(false);
    return label;
}

}

LogAiAnalysisWidget::LogAiAnalysisWidget(AiSettingsStore *aiSettings,
                                         CredentialStore *credentials,
                                         QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
    , m_client(new OpenAiChatClient(this))
    , m_streamFlushTimer(new QTimer(this))
{
    m_streamFlushTimer->setSingleShot(true);
    m_streamFlushTimer->setInterval(kStreamFlushIntervalMs);
    connect(m_streamFlushTimer, &QTimer::timeout, this, &LogAiAnalysisWidget::flushBotBubble);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space8);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    m_analyzeButton = makeActionButton(QStringLiteral("AI 分析日志"), QStringLiteral("primaryButton"), toolbar);
    m_stopButton = makeActionButton(QStringLiteral("停止"), QStringLiteral("secondaryButton"), toolbar);
    m_stopButton->setEnabled(false);
    toolbarLayout->addWidget(m_analyzeButton);
    toolbarLayout->addWidget(m_stopButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName(QStringLiteral("aiChatScrollArea"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_chatContainer = new QWidget(m_scrollArea);
    m_chatContainer->setObjectName(QStringLiteral("aiChatBubbleContainer"));
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setContentsMargins(PageLayout::Space12,
                                     PageLayout::Space12,
                                     PageLayout::Space12,
                                     PageLayout::Space12);
    m_chatLayout->setSpacing(PageLayout::Space10);
    m_chatLayout->addStretch(1);

    m_scrollArea->setWidget(m_chatContainer);
    layout->addWidget(m_scrollArea, 1);

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(m_message);

    auto setBusy = [this](bool busy) {
        m_analyzeButton->setEnabled(!busy);
        m_stopButton->setEnabled(busy);
    };

    connect(m_client, &OpenAiChatClient::deltaReceived, this, [this](const QString &chunk) {
        appendToBotBubble(chunk);
    });
    connect(m_client, &OpenAiChatClient::finished, this, [this, setBusy]() {
        flushBotBubble();
        m_currentBotBubble = nullptr;
        m_message->setText(QStringLiteral("AI 分析完成"));
        setBusy(false);
    });
    connect(m_client, &OpenAiChatClient::failed, this, [this, setBusy](const QString &error) {
        flushBotBubble();
        m_currentBotBubble = nullptr;
        m_message->setText(QStringLiteral("AI 分析失败：%1").arg(error));
        setBusy(false);
    });

    connect(m_analyzeButton, &QPushButton::clicked, this, &LogAiAnalysisWidget::analyzeLog);
    connect(m_stopButton, &QPushButton::clicked, this, &LogAiAnalysisWidget::abortAnalysis);
}

void LogAiAnalysisWidget::setLogContent(const QString &logContent)
{
    m_logContent = logContent;
}

void LogAiAnalysisWidget::analyzeLog()
{
    if (m_logContent.trimmed().isEmpty()) {
        m_message->setText(QStringLiteral("日志内容为空，无法分析"));
        return;
    }

    AiSettings settings;
    QString apiKey;
    QString error;
    if (!AiAssistHelper::resolveCredentials(m_aiSettings, m_credentials, &settings, &apiKey, &error)) {
        m_message->setText(error);
        return;
    }

    clearConversation();

    const QString preview = m_logContent.size() > kMaxBubblePreviewChars
                                ? m_logContent.left(kMaxBubblePreviewChars)
                                      + QStringLiteral("\n…(已截断)")
                                : m_logContent;
    appendBubble(BubbleRole::User, QStringLiteral("请分析以下部署日志：\n\n%1").arg(preview));
    appendBubble(BubbleRole::Bot, QString());

    m_client->abort();
    m_streamFlushTimer->stop();
    m_streamingBuffer.clear();

    m_analyzeButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_message->setText(QStringLiteral("AI 分析中..."));

    const QString preparedLog = AiAssistHelper::prepareLogForAiAnalysis(m_logContent);
    const QJsonArray messages{
        QJsonObject{
            {QStringLiteral("role"), QStringLiteral("system")},
            {QStringLiteral("content"), AiAssistHelper::deploymentLogAnalysisSystemPrompt()}
        },
        QJsonObject{
            {QStringLiteral("role"), QStringLiteral("user")},
            {QStringLiteral("content"),
             QStringLiteral("请分析以下部署日志：\n\n") + preparedLog}
        }
    };
    m_client->streamChat(settings.apiBaseUrl, apiKey, settings.model, messages);
}

void LogAiAnalysisWidget::abortAnalysis()
{
    m_client->abort();
    m_streamFlushTimer->stop();
    flushBotBubble();
    m_currentBotBubble = nullptr;
    m_message->setText(QStringLiteral("已停止"));
    m_analyzeButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}

void LogAiAnalysisWidget::appendBubble(BubbleRole role, const QString &text)
{
    auto *row = new QWidget(m_chatContainer);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(PageLayout::Space8);

    if (role == BubbleRole::User) {
        rowLayout->addStretch();
        auto *bubble = createBubbleLabel(row, QStringLiteral("userBubble"));
        bubble->setText(text);
        rowLayout->addWidget(bubble, 0, Qt::AlignRight);
    } else {
        auto *bubble = createBubbleLabel(row, QStringLiteral("botBubble"));
        bubble->setText(text);
        m_currentBotBubble = bubble;
        rowLayout->addWidget(bubble, 0, Qt::AlignLeft);
        rowLayout->addStretch();
    }

    m_chatLayout->insertWidget(m_chatLayout->count() - 1, row);
    scrollToBottom();
}

void LogAiAnalysisWidget::appendToBotBubble(const QString &chunk)
{
    m_streamingBuffer.append(chunk);
    if (!m_streamFlushTimer->isActive()) {
        m_streamFlushTimer->start();
    }
}

void LogAiAnalysisWidget::flushBotBubble()
{
    m_streamFlushTimer->stop();
    if (m_streamingBuffer.isEmpty() || m_currentBotBubble == nullptr) {
        return;
    }
    const QString text = m_streamingBuffer;
    m_streamingBuffer.clear();

    const QString current = m_currentBotBubble->text();
    m_currentBotBubble->setText(current + text);
    scrollToBottom();
}

void LogAiAnalysisWidget::clearConversation()
{
    m_streamFlushTimer->stop();
    m_streamingBuffer.clear();
    m_currentBotBubble = nullptr;

    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_chatLayout->addStretch(1);
}

void LogAiAnalysisWidget::scrollToBottom()
{
    if (m_scrollArea != nullptr && m_scrollArea->verticalScrollBar() != nullptr) {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum());
    }
}
