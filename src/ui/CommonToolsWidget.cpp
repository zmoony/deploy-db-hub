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
#include "ui/AiStreamBuffer.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "tools/CommonTools.h"
#include "ui/PageLayout.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
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

namespace {

QPushButton *makeActionButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolBarButton"));
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

QPlainTextEdit *makeEditor(const QString &placeholder, QWidget *parent)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setPlaceholderText(placeholder);
    editor->setMinimumHeight(220);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    return editor;
}

void copyTextToClipboard(const QString &text)
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }
}

}

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
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("Hex 转字符串"),
        QStringLiteral("将十六进制字节转为 UTF-8 字符串。"),
        QStringLiteral("转换"),
        QString(),
        QStringLiteral("48656c6c6f205174")));
    m_stack->addWidget(new Ui::Tools::WebSocketToolPage(this));
    m_stack->addWidget(new Ui::Tools::HttpRequestToolPage(this));
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("契约 Mock 数据"),
        QStringLiteral("粘贴 Swagger/Thrift 定义或 JSON 示例，生成多组 mock 数据。"),
        QStringLiteral("生成 Mock"),
        QString(),
        QStringLiteral("{\"name\":\"demo\",\"count\":1,\"enabled\":true}"),
        true,
        QStringLiteral("You generate mock JSON data. Reply with a JSON array only, no markdown or explanation.")));
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("数据采样脱敏"),
        QStringLiteral("从样本数据中抽样并按规则脱敏导出。"),
        QStringLiteral("脱敏"),
        QString(),
        QStringLiteral("phone=13812345678 email=a@example.com")));
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

QWidget *CommonToolsWidget::buildTextToolPage(const QString &title,
                                              const QString &subtitle,
                                              const QString &primaryAction,
                                              const QString &secondaryAction,
                                              const QString &placeholder,
                                              bool enableAiAssist,
                                              const QString &aiSystemPrompt)
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *primaryButton = makeActionButton(primaryAction, toolbar);
    toolbarLayout->addWidget(primaryButton);
    QPushButton *secondaryButton = nullptr;
    if (!secondaryAction.isEmpty()) {
        secondaryButton = makeActionButton(secondaryAction, toolbar);
        toolbarLayout->addWidget(secondaryButton);
    }
    QPushButton *aiAssistButton = nullptr;
    QPushButton *aiStopButton = nullptr;
    if (enableAiAssist) {
        aiAssistButton = makeActionButton(QStringLiteral("AI 辅助"), toolbar);
        aiStopButton = makeActionButton(QStringLiteral("停止"), toolbar);
        aiStopButton->setEnabled(false);
        toolbarLayout->addWidget(aiAssistButton);
        toolbarLayout->addWidget(aiStopButton);
    }
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(placeholder, editors);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(primaryButton, &QPushButton::clicked, this, [this, title, input, output, message]() {
        QString error;
        QString result;
        if (title == QStringLiteral("JSON 格式化")) {
            result = CommonTools::formatJson(input->toPlainText(), &error);
        } else if (title == QStringLiteral("文件差异")) {
            const QStringList parts = input->toPlainText().split(QStringLiteral("\n---\n"));
            if (parts.size() != 2) {
                error = QStringLiteral("请用单独一行 --- 分隔两段文本");
            } else {
                result = CommonTools::compareLines(parts.at(0), parts.at(1));
            }
        } else if (title == QStringLiteral("图片/Base64")) {
            result = CommonTools::textToBase64(input->toPlainText());
        } else if (title == QStringLiteral("正则表达式")) {
            const QStringList parts = input->toPlainText().split(QStringLiteral("\n---\n"));
            if (parts.size() != 2) {
                error = QStringLiteral("请用单独一行 --- 分隔正则和样本文本");
            } else {
                result = CommonTools::matchRegularExpression(parts.at(0), parts.at(1), &error);
            }
        } else if (title == QStringLiteral("Cron 表达式")) {
            result = CommonTools::describeCron(input->toPlainText(), &error);
        } else if (title == QStringLiteral("时间戳")) {
            result = CommonTools::timestampToLocalText(input->toPlainText(), &error);
        } else if (title == QStringLiteral("Hex 转字符串")) {
            result = CommonTools::hexToString(input->toPlainText(), &error);
        } else if (title == QStringLiteral("契约 Mock 数据")) {
            result = CommonTools::mockFromJsonExample(input->toPlainText(), &error);
        } else if (title == QStringLiteral("数据采样脱敏")) {
            result = CommonTools::maskSensitiveText(input->toPlainText());
        }
        setOutput(output, message, result, error);
    });

    if (secondaryButton != nullptr) {
        connect(secondaryButton, &QPushButton::clicked, this, [this, input, output, message]() {
            QString error;
            const QString result = CommonTools::base64ToText(input->toPlainText(), &error);
            setOutput(output, message, result, error);
        });
    }

    if (enableAiAssist && aiAssistButton != nullptr && aiStopButton != nullptr) {
        wireAiAssist(page,
                     aiAssistButton,
                     aiStopButton,
                     output,
                     message,
                     [input]() { return input->toPlainText(); },
                     aiSystemPrompt);
    }

    return page;
}

void CommonToolsWidget::setOutput(QPlainTextEdit *output, QLabel *message, const QString &text, const QString &error)
{
    if (!error.isEmpty()) {
        output->clear();
        message->setText(error);
        return;
    }
    output->setPlainText(text);
    message->setText(QStringLiteral("已完成"));
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
