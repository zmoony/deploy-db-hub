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
#include <QSizePolicy>
#include <QTimer>
#include <QVBoxLayout>

namespace {

constexpr int kStreamFlushIntervalMs = 30;

QPushButton *makeActionButton(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setMinimumHeight(36);
    return button;
}

QLabel *createResultLabel(QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setObjectName(QStringLiteral("logAiAnalysisText"));
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setScaledContents(false);
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
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
    connect(m_streamFlushTimer, &QTimer::timeout, this, &LogAiAnalysisWidget::flushResult);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space10);

    auto *toolbar = new QFrame(this);
    toolbar->setObjectName(QStringLiteral("logAiAnalysisToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);

    auto *title = new QLabel(QStringLiteral("AI 日志分析"), toolbar);
    title->setObjectName(QStringLiteral("sectionLabel"));
    toolbarLayout->addWidget(title);
    toolbarLayout->addStretch();

    m_analyzeButton = makeActionButton(QStringLiteral("AI 分析日志"), QStringLiteral("primaryButton"), toolbar);
    m_stopButton = makeActionButton(QStringLiteral("停止"), QStringLiteral("secondaryButton"), toolbar);
    m_stopButton->setEnabled(false);
    toolbarLayout->addWidget(m_analyzeButton);
    toolbarLayout->addWidget(m_stopButton);
    layout->addWidget(toolbar);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName(QStringLiteral("logAiAnalysisScrollArea"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto *resultFrame = new QFrame(m_scrollArea);
    resultFrame->setObjectName(QStringLiteral("logAiAnalysisResult"));
    resultFrame->setMinimumHeight(220);
    auto *resultLayout = new QVBoxLayout(resultFrame);
    resultLayout->setContentsMargins(PageLayout::Space16,
                                     PageLayout::Space16,
                                     PageLayout::Space16,
                                     PageLayout::Space16);
    resultLayout->setSpacing(0);
    m_resultLabel = createResultLabel(resultFrame);
    m_resultLabel->setText(QStringLiteral("点击“AI 分析日志”后，这里会直接展示分析结果。"));
    resultLayout->addWidget(m_resultLabel, 1);

    m_scrollArea->setWidget(resultFrame);
    layout->addWidget(m_scrollArea, 1);

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(m_message);

    auto setBusy = [this](bool busy) {
        m_analyzeButton->setEnabled(!busy);
        m_stopButton->setEnabled(busy);
    };

    connect(m_client, &OpenAiChatClient::deltaReceived, this, [this](const QString &chunk) {
        appendToResult(chunk);
    });
    connect(m_client, &OpenAiChatClient::finished, this, [this, setBusy]() {
        flushResult();
        m_message->setText(QStringLiteral("AI 分析完成"));
        setBusy(false);
    });
    connect(m_client, &OpenAiChatClient::failed, this, [this, setBusy](const QString &error) {
        flushResult();
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

    clearResult();
    m_resultLabel->setText(QStringLiteral("正在生成分析结果..."));

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
    flushResult();
    m_message->setText(QStringLiteral("已停止"));
    m_analyzeButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}

void LogAiAnalysisWidget::appendToResult(const QString &chunk)
{
    m_streamingBuffer.append(chunk);
    if (!m_streamFlushTimer->isActive()) {
        m_streamFlushTimer->start();
    }
}

void LogAiAnalysisWidget::flushResult()
{
    m_streamFlushTimer->stop();
    if (m_streamingBuffer.isEmpty() || m_resultLabel == nullptr) {
        return;
    }
    const QString text = m_streamingBuffer;
    m_streamingBuffer.clear();

    QString current = m_resultLabel->text();
    if (current == QStringLiteral("正在生成分析结果...")) {
        current.clear();
    }
    m_resultLabel->setText(current + text);
    scrollToBottom();
}

void LogAiAnalysisWidget::clearResult()
{
    m_streamFlushTimer->stop();
    m_streamingBuffer.clear();
    if (m_resultLabel != nullptr) {
        m_resultLabel->clear();
    }
}

void LogAiAnalysisWidget::scrollToBottom()
{
    if (m_scrollArea != nullptr && m_scrollArea->verticalScrollBar() != nullptr) {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum());
    }
}
