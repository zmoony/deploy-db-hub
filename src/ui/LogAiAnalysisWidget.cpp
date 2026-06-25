#include "ui/LogAiAnalysisWidget.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/AiAssistHelper.h"
#include "ui/AiStreamBuffer.h"
#include "ui/PageLayout.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *makeActionButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setMinimumHeight(PageLayout::DialogFieldHeight);
    return button;
}

}

LogAiAnalysisWidget::LogAiAnalysisWidget(AiSettingsStore *aiSettings,
                                         CredentialStore *credentials,
                                         QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
    , m_client(new OpenAiChatClient(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space8);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    m_analyzeButton = makeActionButton(QStringLiteral("AI 分析日志"), toolbar);
    m_stopButton = makeActionButton(QStringLiteral("停止"), toolbar);
    m_stopButton->setEnabled(false);
    toolbarLayout->addWidget(m_analyzeButton);
    toolbarLayout->addWidget(m_stopButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setPlaceholderText(QStringLiteral("AI 分析结果将显示在这里"));
    m_output->setMinimumHeight(140);
    layout->addWidget(m_output, 1);

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(m_message);

    m_outputBuffer = new AiStreamBuffer(m_output, this);

    auto setBusy = [this](bool busy) {
        m_analyzeButton->setEnabled(!busy);
        m_stopButton->setEnabled(busy);
    };

    connect(m_client, &OpenAiChatClient::deltaReceived, this, [this](const QString &chunk) {
        m_outputBuffer->append(chunk);
    });
    connect(m_client, &OpenAiChatClient::finished, this, [this, setBusy]() {
        m_outputBuffer->flush();
        m_message->setText(QStringLiteral("AI 分析完成"));
        setBusy(false);
    });
    connect(m_client, &OpenAiChatClient::failed, this, [this, setBusy](const QString &error) {
        m_outputBuffer->flush();
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

    m_client->abort();
    m_outputBuffer->reset();
    m_output->clear();
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
    m_outputBuffer->flush();
    m_message->setText(QStringLiteral("已停止"));
    m_analyzeButton->setEnabled(true);
    m_stopButton->setEnabled(false);
}
