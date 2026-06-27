#include "ui/CommonToolsWidget.h"

#include "ui/AiChatWidget.h"
#include "adapters/ai/OpenAiChatClient.h"
#include "ui/AiAssistHelper.h"
#include "ui/tools/pages/AiConfigToolPage.h"
#include "ui/tools/pages/Base64TextToolPage.h"
#include "ui/tools/pages/CaseToolPage.h"
#include "ui/tools/pages/CronToolPage.h"
#include "ui/tools/pages/DiffToolPage.h"
#include "ui/tools/pages/HashToolPage.h"
#include "ui/tools/pages/HtmlEntityToolPage.h"
#include "ui/tools/pages/HttpRequestToolPage.h"
#include "ui/tools/pages/HttpStatusToolPage.h"
#include "ui/tools/pages/ImageBase64ToolPage.h"
#include "ui/tools/pages/JsonToolPage.h"
#include "ui/tools/pages/JwtToolPage.h"
#include "ui/tools/pages/NumberBaseToolPage.h"
#include "ui/tools/pages/RegexToolPage.h"
#include "ui/tools/pages/TimestampToolPage.h"
#include "ui/tools/pages/UrlCodecToolPage.h"
#include "ui/tools/pages/UuidToolPage.h"
#include "ui/tools/pages/WebSocketToolPage.h"
#include "ui/tools/pages/HexToStringToolPage.h"
#include "ui/tools/pages/MockDataToolPage.h"
#include "ui/tools/pages/DataMaskToolPage.h"
#include "ui/AiStreamBuffer.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"

#include <QAction>
#include <QBrush>
#include <QBuffer>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPair>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

CommonToolsWidget::CommonToolsWidget(AiSettingsStore *aiSettings,
                                     CredentialStore *credentials,
                                     QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new Ui::Tools::AiConfigToolPage(m_aiSettings, m_credentials, this));
    m_stack->addWidget(new AiChatWidget(m_aiSettings, m_credentials, this));
    m_stack->addWidget(new Ui::Tools::JsonToolPage(this));
    m_stack->addWidget(new Ui::Tools::DiffToolPage(this));
    m_stack->addWidget(new Ui::Tools::ImageBase64ToolPage(this));
    m_stack->addWidget(new Ui::Tools::RegexToolPage(this));
    m_stack->addWidget(new Ui::Tools::CronToolPage(this));
    m_stack->addWidget(new Ui::Tools::TimestampToolPage(this));
    m_stack->addWidget(new Ui::Tools::HtmlEntityToolPage(this));
    m_stack->addWidget(new Ui::Tools::HttpStatusToolPage(this));
    m_stack->addWidget(new Ui::Tools::HexToStringToolPage(this));
    m_stack->addWidget(new Ui::Tools::WebSocketToolPage(this));
    m_stack->addWidget(new Ui::Tools::HttpRequestToolPage(this));
    m_stack->addWidget(new Ui::Tools::MockDataToolPage(this));
    m_stack->addWidget(new Ui::Tools::DataMaskToolPage(this));
    m_stack->addWidget(new Ui::Tools::UuidToolPage(this));
    m_stack->addWidget(new Ui::Tools::HashToolPage(this));
    m_stack->addWidget(new Ui::Tools::UrlCodecToolPage(this));
    m_stack->addWidget(new Ui::Tools::Base64TextToolPage(this));
    m_stack->addWidget(new Ui::Tools::JwtToolPage(this));
    m_stack->addWidget(new Ui::Tools::NumberBaseToolPage(this));
    m_stack->addWidget(new Ui::Tools::CaseToolPage(this));

    layout->addWidget(m_stack, 1);
}

QStringList CommonToolsWidget::toolLabels() const
{
    return {
        QStringLiteral("AI 配置"),
        QStringLiteral("AI 聊天"),
        QStringLiteral("JSON 格式化"),
        QStringLiteral("文本比较"),
        QStringLiteral("图片/Base64"),
        QStringLiteral("正则表达式"),
        QStringLiteral("Cron 表达式"),
        QStringLiteral("时间戳"),
        QStringLiteral("HTML 特殊字符"),
        QStringLiteral("HTTP 状态码"),
        QStringLiteral("Hex 转字符串"),
        QStringLiteral("WebSocket 测试"),
        QStringLiteral("HTTP 请求调试"),
        QStringLiteral("契约 Mock 数据"),
        QStringLiteral("数据采样脱敏"),
        QStringLiteral("UUID 生成"),
        QStringLiteral("哈希计算"),
        QStringLiteral("URL 编解码"),
        QStringLiteral("Base64 文本"),
        QStringLiteral("JWT 解析"),
        QStringLiteral("进制转换"),
        QStringLiteral("命名转换")
    };
}

QWidget *CommonToolsWidget::takeToolPage(int index)
{
    if (m_stack == nullptr || index < 0 || index >= m_stack->count()) {
        return nullptr;
    }
    QWidget *page = m_stack->widget(index);
    m_stack->removeWidget(page);
    page->setParent(nullptr);
    return page;
}

int CommonToolsWidget::toolPageCount() const
{
    return m_stack != nullptr ? m_stack->count() : 0;
}

bool CommonToolsWidget::resolveAiCredentials(AiSettings *settings, QString *apiKey, QString *error) const
{
    return AiAssistHelper::resolveCredentials(m_aiSettings, m_credentials, settings, apiKey, error);
}

void CommonToolsWidget::wireAiAssist(QWidget *page,
                                     QPushButton *assistButton,
                                     QPushButton *stopButton,
                                     QPlainTextEdit *output,
                                     QLabel *message,
                                     const std::function<QString()> &userContentProvider,
                                     const QString &systemPrompt,
                                     const std::function<void()> &onStart)
{
    auto *client = new OpenAiChatClient(page);
    auto *buffer = new AiStreamBuffer(output, page);

    auto setBusy = [assistButton, stopButton](bool busy) {
        assistButton->setEnabled(!busy);
        stopButton->setEnabled(busy);
    };

    connect(client, &OpenAiChatClient::deltaReceived, page, [buffer](const QString &chunk) {
        buffer->append(chunk);
    });
    connect(client, &OpenAiChatClient::finished, page, [message, buffer, setBusy]() {
        buffer->flush();
        message->setText(QStringLiteral("AI 已完成"));
        setBusy(false);
    });
    connect(client, &OpenAiChatClient::failed, page, [message, buffer, setBusy](const QString &failedMessage) {
        buffer->flush();
        message->setText(QStringLiteral("AI 失败：%1").arg(failedMessage));
        setBusy(false);
    });

    connect(assistButton, &QPushButton::clicked, page, [this, client, output, message, buffer, userContentProvider, systemPrompt, onStart, setBusy]() {
        const QString userContent = userContentProvider().trimmed();
        if (userContent.isEmpty()) {
            message->setText(QStringLiteral("请先输入内容"));
            return;
        }

        AiSettings settings;
        QString apiKey;
        QString error;
        if (!resolveAiCredentials(&settings, &apiKey, &error)) {
            message->setText(error);
            return;
        }

        if (onStart) {
            onStart();
        }

        client->abort();
        buffer->reset();
        output->clear();
        setBusy(true);
        message->setText(QStringLiteral("AI 生成中..."));

        const QJsonArray messages{
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("system")},
                {QStringLiteral("content"), systemPrompt}
            },
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("user")},
                {QStringLiteral("content"), userContent}
            }
        };
        client->streamChat(settings.apiBaseUrl, apiKey, settings.model, messages);
    });

    connect(stopButton, &QPushButton::clicked, page, [client, message, buffer, setBusy]() {
        client->abort();
        buffer->flush();
        message->setText(QStringLiteral("已停止"));
        setBusy(false);
    });
}
