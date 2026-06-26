#include "ui/AiChatWidget.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/AiAssistHelper.h"
#include "ui/AiStreamBuffer.h"
#include "ui/PageLayout.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QStyle>
#include <QTextCursor>
#include <QVBoxLayout>

namespace {

constexpr int kHistoryMinHeight = 220;
constexpr int kInputMaxHeight = 160;
constexpr int kIconButtonSize = 36;
constexpr int kChipIconSize = 16;
constexpr int kAvatarSize = 28;

QPushButton *makeIconButton(QWidget *parent,
                            const QString &objectName,
                            const QIcon &icon,
                            const QString &tooltip)
{
    auto *button = new QPushButton(parent);
    button->setObjectName(objectName);
    button->setIcon(icon);
    button->setIconSize(QSize(kIconButtonSize - 16, kIconButtonSize - 16));
    button->setFlat(false);
    button->setFixedSize(kIconButtonSize, kIconButtonSize);
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QPushButton *makeQuickChip(QWidget *parent, const QString &label, const QIcon &icon)
{
    auto *chip = new QPushButton(parent);
    chip->setObjectName(QStringLiteral("aiActionChip"));
    chip->setIcon(icon);
    chip->setIconSize(QSize(kChipIconSize, kChipIconSize));
    chip->setText(label);
    chip->setCursor(Qt::PointingHandCursor);
    chip->setToolTip(QStringLiteral("将示例前缀插入到输入框"));
    return chip;
}

}

AiChatWidget::AiChatWidget(AiSettingsStore *aiSettings,
                           CredentialStore *credentials,
                           QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
    , m_client(new OpenAiChatClient(this))
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(PageLayout::Space12);

    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(PageLayout::Space12);
    headerLayout->addStretch();

    m_newChatButton = new QPushButton(header);
    m_newChatButton->setObjectName(QStringLiteral("aiNewChatButton"));
    m_newChatButton->setIcon(QIcon(QStringLiteral(":/images/ai/quick.svg")));
    m_newChatButton->setIconSize(QSize(18, 18));
    m_newChatButton->setText(QStringLiteral("新对话"));
    m_newChatButton->setCursor(Qt::PointingHandCursor);
    m_newChatButton->setToolTip(QStringLiteral("清空当前对话，开始新一轮"));
    headerLayout->addWidget(m_newChatButton, 0, Qt::AlignTop);

    root->addWidget(header);

    // Chat history (read-only)
    m_history = new QPlainTextEdit(this);
    m_history->setObjectName(QStringLiteral("aiChatHistory"));
    m_history->setReadOnly(true);
    m_history->setPlaceholderText(QStringLiteral("对话记录"));
    m_history->setFrameShape(QFrame::NoFrame);
    m_history->setMinimumHeight(kHistoryMinHeight);
    m_history->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    root->addWidget(m_history, 1);

    // Input card (rounded)
    auto *inputCard = new QFrame(this);
    inputCard->setObjectName(QStringLiteral("aiInputCard"));
    auto *cardLayout = new QVBoxLayout(inputCard);
    cardLayout->setContentsMargins(PageLayout::Space12,
                                   PageLayout::Space10,
                                   PageLayout::Space12,
                                   PageLayout::Space10);
    cardLayout->setSpacing(PageLayout::Space8);

    // Input row: AI avatar + multiline input
    auto *inputRow = new QWidget(inputCard);
    auto *inputRowLayout = new QHBoxLayout(inputRow);
    inputRowLayout->setContentsMargins(0, 0, 0, 0);
    inputRowLayout->setSpacing(PageLayout::Space8);

    auto *avatar = new QLabel(inputRow);
    avatar->setObjectName(QStringLiteral("aiAvatar"));
    const QPixmap avatarPixmap(QStringLiteral(":/images/icon.png"));
    avatar->setPixmap(avatarPixmap.scaled(kAvatarSize,
                                          kAvatarSize,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation));
    avatar->setFixedSize(kAvatarSize, kAvatarSize);
    inputRowLayout->addWidget(avatar, 0, Qt::AlignTop);

    m_input = new QPlainTextEdit(inputRow);
    m_input->setObjectName(QStringLiteral("aiChatInput"));
    m_input->setPlaceholderText(QStringLiteral("输入消息，Ctrl+Enter 发送"));
    m_input->setFrameShape(QFrame::NoFrame);
    m_input->setMinimumHeight(56);
    m_input->setMaximumHeight(kInputMaxHeight);
    m_input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    inputRowLayout->addWidget(m_input, 1);

    // Ctrl+Enter / Ctrl+Return triggers send (without eating the Enter key,
    // so the user can still insert newlines with a plain Return).
    auto *sendShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")),
                                       m_input);
    sendShortcut->setContext(Qt::WidgetShortcut);
    connect(sendShortcut, &QShortcut::activated, this, &AiChatWidget::sendMessage);

    cardLayout->addWidget(inputRow);

    // Action bar: quick action chips on the left, send/stop on the right
    auto *actionBar = new QWidget(inputCard);
    auto *actionLayout = new QHBoxLayout(actionBar);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(PageLayout::Space4);

    struct ChipDef
    {
        QString label;
        QString iconPath;
        QString prompt;
    };
    const QList<ChipDef> chips = {
        {QStringLiteral("快速"), QStringLiteral(":/images/ai/quick.svg"),
         QStringLiteral("请用 3 句话回答：")},
        {QStringLiteral("图像"), QStringLiteral(":/images/ai/image.svg"),
         QStringLiteral("请为以下主题生成图像描述：")},
        {QStringLiteral("PPT"), QStringLiteral(":/images/ai/presentation.svg"),
         QStringLiteral("请为以下主题生成 PPT 大纲：")},
        {QStringLiteral("编程"), QStringLiteral(":/images/ai/code.svg"),
         QStringLiteral("请用代码实现：")},
        {QStringLiteral("视频"), QStringLiteral(":/images/ai/video.svg"),
         QStringLiteral("请为以下主题生成视频脚本：")},
        {QStringLiteral("翻译"), QStringLiteral(":/images/ai/translate.svg"),
         QStringLiteral("请翻译为英文：")},
        {QStringLiteral("更多"), QStringLiteral(":/images/ai/more.svg"),
         QStringLiteral("")}
    };
    for (const ChipDef &chip : chips) {
        auto *button = makeQuickChip(actionBar, chip.label, QIcon(chip.iconPath));
        actionLayout->addWidget(button);
        if (chip.prompt.isEmpty()) {
            connect(button, &QPushButton::clicked, this, [this]() {
                m_input->setFocus();
            });
        } else {
            connect(button, &QPushButton::clicked, this, [this, chip]() {
                m_input->setPlainText(chip.prompt);
                m_input->setFocus();
                m_input->moveCursor(QTextCursor::End);
            });
        }
    }
    actionLayout->addStretch();

    m_stopButton = makeIconButton(actionBar,
                                  QStringLiteral("aiStopButton"),
                                  style()->standardIcon(QStyle::SP_BrowserStop),
                                  QStringLiteral("停止"));
    m_stopButton->setEnabled(false);
    actionLayout->addWidget(m_stopButton);

    m_sendButton = makeIconButton(actionBar,
                                  QStringLiteral("aiSendButton"),
                                  style()->standardIcon(QStyle::SP_ArrowRight),
                                  QStringLiteral("发送 (Ctrl+Enter)"));
    actionLayout->addWidget(m_sendButton);

    cardLayout->addWidget(actionBar);

    root->addWidget(inputCard);

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    root->addWidget(m_message);

    m_historyBuffer = new AiStreamBuffer(m_history, this);

    // Wire up signals
    connect(m_sendButton, &QPushButton::clicked, this, &AiChatWidget::sendMessage);
    connect(m_stopButton, &QPushButton::clicked, this, &AiChatWidget::stopGeneration);
    connect(m_newChatButton, &QPushButton::clicked, this, &AiChatWidget::clearConversation);

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_sendButton != nullptr) {
            const bool hasText = !m_input->toPlainText().trimmed().isEmpty();
            m_sendButton->setEnabled(hasText && m_stopButton != nullptr && !m_stopButton->isEnabled());
        }
    });

    connect(m_client, &OpenAiChatClient::deltaReceived, this, [this](const QString &chunk) {
        m_pendingAssistantText += chunk;
        m_historyBuffer->append(chunk);
    });
    connect(m_client, &OpenAiChatClient::finished, this, [this]() {
        m_historyBuffer->flush();
        if (!m_pendingAssistantText.isEmpty()) {
            m_messages.append(QJsonObject{
                {QStringLiteral("role"), QStringLiteral("assistant")},
                {QStringLiteral("content"), m_pendingAssistantText}
            });
            m_pendingAssistantText.clear();
        }
        m_message->setText(QStringLiteral("回复完成"));
        setBusy(false);
    });
    connect(m_client, &OpenAiChatClient::failed, this, [this](const QString &error) {
        m_historyBuffer->flush();
        m_pendingAssistantText.clear();
        m_message->setText(QStringLiteral("请求失败：%1").arg(error));
        setBusy(false);
    });
}

void AiChatWidget::sendMessage()
{
    const QString userText = m_input->toPlainText().trimmed();
    if (userText.isEmpty()) {
        m_message->setText(QStringLiteral("请输入消息"));
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
    m_pendingAssistantText.clear();
    appendHistoryLine(QStringLiteral("你"), userText);
    m_messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), userText}
    });
    m_input->clear();
    appendHistoryLine(QStringLiteral("AI"), QString());
    setBusy(true);
    m_message->setText(QStringLiteral("生成中..."));

    m_client->streamChat(settings.apiBaseUrl, apiKey, settings.model, m_messages);
}

void AiChatWidget::stopGeneration()
{
    m_client->abort();
    m_historyBuffer->flush();
    if (!m_pendingAssistantText.isEmpty()) {
        m_messages.append(QJsonObject{
            {QStringLiteral("role"), QStringLiteral("assistant")},
            {QStringLiteral("content"), m_pendingAssistantText}
        });
        m_pendingAssistantText.clear();
    }
    m_message->setText(QStringLiteral("已停止"));
    setBusy(false);
}

void AiChatWidget::clearConversation()
{
    m_client->abort();
    m_messages = QJsonArray();
    m_pendingAssistantText.clear();
    m_historyBuffer->reset();
    m_history->clear();
    m_input->clear();
    m_message->setText(QStringLiteral("对话已清空"));
    setBusy(false);
}

void AiChatWidget::appendHistoryLine(const QString &roleLabel, const QString &text)
{
    if (!m_history->toPlainText().isEmpty()) {
        m_history->appendPlainText(QString());
    }
    if (text.isEmpty()) {
        m_history->appendPlainText(QStringLiteral("%1：").arg(roleLabel));
    } else {
        m_history->appendPlainText(QStringLiteral("%1：%2").arg(roleLabel, text));
    }
}

void AiChatWidget::setBusy(bool busy)
{
    m_sendButton->setEnabled(!busy && !m_input->toPlainText().trimmed().isEmpty());
    m_stopButton->setEnabled(busy);
    m_input->setEnabled(!busy);
}
