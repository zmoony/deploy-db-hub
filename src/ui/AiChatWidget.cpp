#include "ui/AiChatWidget.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/AiAssistHelper.h"
#include "ui/PageLayout.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLayoutItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QStyle>
#include <QTextCursor>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// Pixso exact dimensions
constexpr int kAvatarSize = 36;
constexpr int kInputBadgeSize = 28;
constexpr int kSendButtonSize = 40;
constexpr int kChipHeight = 32;
constexpr int kChipIconSize = 12;
constexpr int kInputMinHeight = 28;
constexpr int kInputMaxHeight = 72;
constexpr int kStreamFlushIntervalMs = 30;

QPixmap tintedIconPixmap(const QIcon &icon, const QSize &size, const QColor &color)
{
    const QPixmap src = icon.pixmap(size, QIcon::Normal, QIcon::Off);
    if (src.isNull()) {
        return src;
    }
    QPixmap tinted(src.size());
    tinted.fill(Qt::transparent);
    QPainter painter(&tinted);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawPixmap(0, 0, src);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
    painter.end();
    return tinted;
}

QIcon whiteIcon(const QIcon &icon, const QSize &size)
{
    return QIcon(tintedIconPixmap(icon, size, QColor(Qt::white)));
}

QLabel *makeAvatar(QWidget *parent, const QString &objectName, const QString &text)
{
    auto *avatar = new QLabel(text, parent);
    avatar->setObjectName(objectName);
    avatar->setFixedSize(kAvatarSize, kAvatarSize);
    avatar->setAlignment(Qt::AlignCenter);
    return avatar;
}

QLabel *createBubble(QWidget *parent, const QString &objectName)
{
    auto *label = new QLabel(parent);
    label->setObjectName(objectName);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setScaledContents(false);
    return label;
}

void applyDropShadow(QWidget *widget, int blurRadius, int offsetY, const QColor &color)
{
    if (widget == nullptr || widget->graphicsEffect() != nullptr) {
        return;
    }
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, offsetY);
    shadow->setColor(color);
    widget->setGraphicsEffect(shadow);
}

QWidget *makeTypingBubble(QWidget *parent)
{
    auto *bubble = new QFrame(parent);
    bubble->setObjectName(QStringLiteral("aiTypingBubble"));
    auto *dotsLayout = new QHBoxLayout(bubble);
    dotsLayout->setContentsMargins(16, 12, 16, 12);
    dotsLayout->setSpacing(6);
    for (const QString &name : {QStringLiteral("aiTypingDot1"),
                                QStringLiteral("aiTypingDot2"),
                                QStringLiteral("aiTypingDot3")}) {
        auto *dot = new QLabel(bubble);
        dot->setObjectName(name);
        dot->setFixedSize(7, 7);
        dotsLayout->addWidget(dot);
    }
    return bubble;
}

}

AiChatWidget::AiChatWidget(AiSettingsStore *aiSettings,
                           CredentialStore *credentials,
                           QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
    , m_client(new OpenAiChatClient(this))
    , m_streamFlushTimer(new QTimer(this))
{
    setObjectName(QStringLiteral("aiChatPage"));
    setProperty("workspacePage", true);
    setAttribute(Qt::WA_StyledBackground, true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_streamFlushTimer->setSingleShot(true);
    m_streamFlushTimer->setInterval(kStreamFlushIntervalMs);
    connect(m_streamFlushTimer, &QTimer::timeout, this, &AiChatWidget::flushBotBubble);

    // Root: 20px padding all sides, 16px spacing — matches Pixso "聊天内容区"
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(PageLayout::Space20,
                             PageLayout::Space20,
                             PageLayout::Space20,
                             PageLayout::Space20);
    root->setSpacing(PageLayout::Space16);

    // ── Header (聊天标题) ──────────────────────────────────────────
    // Pixso: 940×64, rounded 12, blue gradient, padding-lr 20, spacing 12
    auto *header = new QFrame(this);
    header->setObjectName(QStringLiteral("aiChatHeader"));
    header->setFixedHeight(64);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(PageLayout::Space20, 0, PageLayout::Space20, 0);
    headerLayout->setSpacing(PageLayout::Space12);

    auto *headerIcon = new QLabel(header);
    headerIcon->setObjectName(QStringLiteral("aiChatHeaderIcon"));
    headerIcon->setFixedSize(22, 22);
    headerIcon->setPixmap(tintedIconPixmap(QIcon(QStringLiteral(":/images/ai/quick.svg")),
                                           QSize(22, 22),
                                           QColor(Qt::white)));
    headerIcon->setAlignment(Qt::AlignCenter);

    auto *titleBlock = new QWidget(header);
    titleBlock->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *titleLayout = new QVBoxLayout(titleBlock);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);
    auto *titleLabel = new QLabel(QStringLiteral("AI 智能助手"), titleBlock);
    titleLabel->setObjectName(QStringLiteral("aiChatHeaderTitle"));
    auto *subtitleLabel = new QLabel(QStringLiteral("基于大语言模型，智能回答您的问题"), titleBlock);
    subtitleLabel->setObjectName(QStringLiteral("aiChatHeaderSubtitle"));
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(subtitleLabel);

    auto *clearButton = new QPushButton(header);
    clearButton->setObjectName(QStringLiteral("aiChatClearButton"));
    clearButton->setIcon(whiteIcon(style()->standardIcon(QStyle::SP_TrashIcon), QSize(14, 14)));
    clearButton->setIconSize(QSize(14, 14));
    clearButton->setText(QStringLiteral("清空"));
    clearButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(headerIcon, 0, Qt::AlignVCenter);
    headerLayout->addWidget(titleBlock, 1, Qt::AlignVCenter);
    headerLayout->addWidget(clearButton, 0, Qt::AlignVCenter);
    root->addWidget(header);

    // ── Message area (消息区) ──────────────────────────────────────
    // Pixso: rounded 12, white, shadow, padding 20, spacing 20
    auto *historyCard = new QFrame(this);
    historyCard->setObjectName(QStringLiteral("aiChatContainer"));
    applyDropShadow(historyCard, 12, 2, QColor(30, 64, 175, 24));
    auto *cardLayout = new QVBoxLayout(historyCard);
    cardLayout->setContentsMargins(PageLayout::Space20,
                                   PageLayout::Space20,
                                   PageLayout::Space20,
                                   PageLayout::Space20);
    cardLayout->setSpacing(PageLayout::Space20);

    m_scrollArea = new QScrollArea(historyCard);
    m_scrollArea->setObjectName(QStringLiteral("aiChatScrollArea"));
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_chatContainer = new QWidget(m_scrollArea);
    m_chatContainer->setObjectName(QStringLiteral("aiChatBubbleContainer"));
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setContentsMargins(0, 0, 0, 0);
    m_chatLayout->setSpacing(0);
    m_chatLayout->addStretch(1);

    m_scrollArea->setWidget(m_chatContainer);
    cardLayout->addWidget(m_scrollArea, 1);
    root->addWidget(historyCard, 1);

    // ── Input area (输入区) ────────────────────────────────────────
    // Pixso: rounded 12, white, shadow, padding 16, spacing 12
    auto *inputCard = new QFrame(this);
    inputCard->setObjectName(QStringLiteral("aiInputCard"));
    inputCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    applyDropShadow(inputCard, 12, 2, QColor(30, 64, 175, 24));
    auto *cardInputLayout = new QVBoxLayout(inputCard);
    cardInputLayout->setContentsMargins(PageLayout::Space12,
                                        PageLayout::Space12,
                                        PageLayout::Space12,
                                        PageLayout::Space12);
    cardInputLayout->setSpacing(PageLayout::Space8);

    // Input top row: zap badge + placeholder (hidden once user starts typing)
    m_inputTopRow = new QWidget(inputCard);
    m_inputTopRow->setObjectName(QStringLiteral("aiInputTopRow"));
    auto *inputTopLayout = new QHBoxLayout(m_inputTopRow);
    inputTopLayout->setContentsMargins(0, 0, 0, 0);
    inputTopLayout->setSpacing(PageLayout::Space10);

    auto *inputBadge = new QLabel(m_inputTopRow);
    inputBadge->setObjectName(QStringLiteral("aiInputBadge"));
    inputBadge->setFixedSize(kInputBadgeSize, kInputBadgeSize);
    inputBadge->setPixmap(tintedIconPixmap(QIcon(QStringLiteral(":/images/ai/quick.svg")),
                                           QSize(14, 14),
                                           QColor(Qt::white)));
    inputBadge->setAlignment(Qt::AlignCenter);

    m_inputHint = new QLabel(QStringLiteral("输入消息，Ctrl+Enter 发送..."), m_inputTopRow);
    m_inputHint->setObjectName(QStringLiteral("aiInputHint"));

    inputTopLayout->addWidget(inputBadge, 0, Qt::AlignVCenter);
    inputTopLayout->addWidget(m_inputHint, 1, Qt::AlignVCenter);
    cardInputLayout->addWidget(m_inputTopRow);

    // Input content area: bordered box
    auto *inputContent = new QFrame(inputCard);
    inputContent->setObjectName(QStringLiteral("aiInputContent"));
    auto *inputContentLayout = new QVBoxLayout(inputContent);
    inputContentLayout->setContentsMargins(PageLayout::Space8,
                                           PageLayout::Space6,
                                           PageLayout::Space8,
                                           PageLayout::Space6);

    m_input = new QPlainTextEdit(inputContent);
    m_input->setObjectName(QStringLiteral("aiChatInput"));
    m_input->setFrameShape(QFrame::NoFrame);
    m_input->setMinimumHeight(kInputMinHeight);
    m_input->setMaximumHeight(kInputMaxHeight);
    m_input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_input->document()->setDocumentMargin(2);
    inputContentLayout->addWidget(m_input);
    cardInputLayout->addWidget(inputContent);

    // Bottom toolbar: scrollable chips + send button
    auto *actionBar = new QWidget(inputCard);
    auto *actionLayout = new QHBoxLayout(actionBar);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(PageLayout::Space8);

    auto *chipScroll = new QScrollArea(actionBar);
    chipScroll->setObjectName(QStringLiteral("aiChipScroll"));
    chipScroll->setWidgetResizable(true);
    chipScroll->setFrameShape(QFrame::NoFrame);
    chipScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chipScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chipScroll->setFixedHeight(kChipHeight + 2);

    auto *chipHost = new QWidget(chipScroll);
    chipHost->setObjectName(QStringLiteral("aiChipHost"));
    auto *chipLayout = new QHBoxLayout(chipHost);
    chipLayout->setContentsMargins(0, 0, 0, 0);
    chipLayout->setSpacing(PageLayout::Space8);

    struct ChipDef { QString label; QString iconPath; QString prompt; };
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
    for (int i = 0; i < chips.size(); ++i) {
        const ChipDef &chip = chips.at(i);
        auto *button = new QPushButton(chipHost);
        button->setObjectName(QStringLiteral("aiActionChip"));
        button->setIcon(QIcon(chip.iconPath));
        button->setIconSize(QSize(kChipIconSize, kChipIconSize));
        button->setText(chip.label);
        button->setCursor(Qt::PointingHandCursor);
        button->setFixedHeight(kChipHeight);
        button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        button->setProperty("selected", false);
        m_actionChips.append(button);
        chipLayout->addWidget(button);
        if (chip.prompt.isEmpty()) {
            connect(button, &QPushButton::clicked, this, [this, i]() {
                setSelectedChip(i);
                m_input->setFocus();
            });
        } else {
            connect(button, &QPushButton::clicked, this, [this, chip, i]() {
                setSelectedChip(i);
                m_input->setPlainText(chip.prompt);
                m_input->setFocus();
                m_input->moveCursor(QTextCursor::End);
            });
        }
    }
    chipLayout->addStretch();
    chipScroll->setWidget(chipHost);
    actionLayout->addWidget(chipScroll, 1);

    m_stopButton = new QPushButton(actionBar);
    m_stopButton->setObjectName(QStringLiteral("aiStopButton"));
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
    m_stopButton->setIconSize(QSize(18, 18));
    m_stopButton->setFixedSize(kSendButtonSize, kSendButtonSize);
    m_stopButton->setCursor(Qt::PointingHandCursor);
    m_stopButton->setVisible(false);
    actionLayout->addWidget(m_stopButton);

    m_sendButton = new QPushButton(actionBar);
    m_sendButton->setObjectName(QStringLiteral("aiSendButton"));
    m_sendButton->setIcon(whiteIcon(style()->standardIcon(QStyle::SP_ArrowForward), QSize(18, 18)));
    m_sendButton->setIconSize(QSize(18, 18));
    m_sendButton->setFixedSize(kSendButtonSize, kSendButtonSize);
    m_sendButton->setCursor(Qt::PointingHandCursor);
    applyDropShadow(m_sendButton, 12, 4, QColor(37, 99, 235, 64));
    actionLayout->addWidget(m_sendButton);

    cardInputLayout->addWidget(actionBar);
    root->addWidget(inputCard);

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    m_message->setVisible(false);
    root->addWidget(m_message);

    // ── Connections ────────────────────────────────────────────────
    auto *sendShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Return")), m_input);
    sendShortcut->setContext(Qt::WidgetShortcut);
    connect(sendShortcut, &QShortcut::activated, this, &AiChatWidget::sendMessage);
    connect(m_sendButton, &QPushButton::clicked, this, &AiChatWidget::sendMessage);
    connect(m_stopButton, &QPushButton::clicked, this, &AiChatWidget::stopGeneration);
    connect(clearButton, &QPushButton::clicked, this, &AiChatWidget::clearConversation);

    connect(m_input, &QPlainTextEdit::textChanged, this, [this]() {
        const bool empty = m_input->toPlainText().trimmed().isEmpty();
        if (m_inputTopRow != nullptr) {
            m_inputTopRow->setVisible(empty);
        }
        if (m_sendButton != nullptr) {
            m_sendButton->setEnabled(!empty && (m_stopButton == nullptr || !m_stopButton->isEnabled()));
        }
    });

    connect(m_client, &OpenAiChatClient::deltaReceived, this, [this](const QString &chunk) {
        if (m_typingRow != nullptr) {
            hideTypingIndicator();
            appendBubble(BubbleRole::Bot, QString());
        }
        m_pendingAssistantText += chunk;
        appendToBotBubble(chunk);
    });
    connect(m_client, &OpenAiChatClient::finished, this, [this]() {
        hideTypingIndicator();
        flushBotBubble();
        if (!m_pendingAssistantText.isEmpty()) {
            m_messages.append(QJsonObject{
                {QStringLiteral("role"), QStringLiteral("assistant")},
                {QStringLiteral("content"), m_pendingAssistantText}
            });
            m_pendingAssistantText.clear();
        }
        m_currentBotBubble = nullptr;
        setBusy(false);
    });
    connect(m_client, &OpenAiChatClient::failed, this, [this](const QString &error) {
        hideTypingIndicator();
        flushBotBubble();
        if (m_currentBotBubble != nullptr && m_currentBotBubble->text().trimmed().isEmpty()) {
            QWidget *row = m_currentBotBubble->parentWidget()->parentWidget();
            m_chatLayout->removeWidget(row);
            row->deleteLater();
            m_currentBotBubble = nullptr;
        }
        m_pendingAssistantText.clear();
        if (m_message != nullptr) {
            m_message->setText(QStringLiteral("请求失败：%1").arg(error));
            m_message->setVisible(true);
        }
        setBusy(false);
    });

    appendWelcomeMessage();
    setSelectedChip(3);
}

void AiChatWidget::appendWelcomeMessage()
{
    appendBubble(BubbleRole::Bot,
                 QStringLiteral("您好！我是 AI 智能助手，基于先进的大语言模型构建。"
                                "我可以帮助您解答技术问题、生成和优化代码、分析处理文本内容，"
                                "以及提供创意写作建议。请输入您的问题，我将尽力为您提供帮助。"));
    m_messages = QJsonArray();
    m_currentBotBubble = nullptr;
}

void AiChatWidget::sendMessage()
{
    const QString userText = m_input->toPlainText().trimmed();
    if (userText.isEmpty()) {
        return;
    }

    AiSettings settings;
    QString apiKey;
    QString error;
    if (!AiAssistHelper::resolveCredentials(m_aiSettings, m_credentials, &settings, &apiKey, &error)) {
        if (m_message != nullptr) {
            m_message->setText(error);
            m_message->setVisible(true);
        }
        return;
    }

    m_client->abort();
    m_streamFlushTimer->stop();
    m_streamingBuffer.clear();
    m_pendingAssistantText.clear();
    m_currentBotBubble = nullptr;
    hideTypingIndicator();

    appendBubble(BubbleRole::User, userText);
    m_messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), userText}
    });
    m_input->clear();

    showTypingIndicator();
    setBusy(true);

    m_client->streamChat(settings.apiBaseUrl, apiKey, settings.model, m_messages);
}

void AiChatWidget::stopGeneration()
{
    m_client->abort();
    m_streamFlushTimer->stop();
    hideTypingIndicator();
    flushBotBubble();
    if (!m_pendingAssistantText.isEmpty()) {
        m_messages.append(QJsonObject{
            {QStringLiteral("role"), QStringLiteral("assistant")},
            {QStringLiteral("content"), m_pendingAssistantText}
        });
        m_pendingAssistantText.clear();
    } else if (m_currentBotBubble != nullptr && m_currentBotBubble->text().trimmed().isEmpty()) {
        QWidget *row = m_currentBotBubble->parentWidget()->parentWidget();
        m_chatLayout->removeWidget(row);
        row->deleteLater();
    }
    m_currentBotBubble = nullptr;
    setBusy(false);
}

void AiChatWidget::clearConversation()
{
    m_client->abort();
    m_streamFlushTimer->stop();
    m_streamingBuffer.clear();
    m_pendingAssistantText.clear();
    m_currentBotBubble = nullptr;
    hideTypingIndicator();

    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_chatLayout->addStretch(1);

    m_input->clear();
    appendWelcomeMessage();
    setBusy(false);
}

void AiChatWidget::appendBubble(BubbleRole role, const QString &text)
{
    const QString timeStr = QTime::currentTime().toString(QStringLiteral("HH:mm"));

    // Row container: horizontal, spacing 12
    auto *row = new QWidget(m_chatContainer);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(PageLayout::Space12);

    // Content column: vertical, spacing 6
    auto *content = new QWidget(row);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(6);

    auto *senderLabel = new QLabel(content);
    senderLabel->setObjectName(QStringLiteral("aiSenderLabel"));

    auto *bubble = createBubble(content,
                                role == BubbleRole::User
                                    ? QStringLiteral("userBubble")
                                    : QStringLiteral("botBubble"));
    bubble->setText(text);

    auto *timeLabel = new QLabel(timeStr, content);
    timeLabel->setObjectName(QStringLiteral("aiTimeLabel"));

    if (role == BubbleRole::User) {
        // User: [stretch] [content(right-aligned)] [avatar]
        senderLabel->setText(QStringLiteral("你好"));
        contentLayout->addWidget(senderLabel, 0, Qt::AlignRight);
        contentLayout->addWidget(bubble, 0, Qt::AlignRight);
        contentLayout->addWidget(timeLabel, 0, Qt::AlignRight);

        rowLayout->addStretch();
        rowLayout->addWidget(content, 0, Qt::AlignTop);
        rowLayout->addWidget(makeAvatar(row, QStringLiteral("aiUserAvatar"), QStringLiteral("U")),
                             0, Qt::AlignTop);
    } else {
        // AI: [avatar] [content(left-aligned)] [stretch]
        senderLabel->setText(QStringLiteral("AI 助手"));
        contentLayout->addWidget(senderLabel);
        contentLayout->addWidget(bubble);
        contentLayout->addWidget(timeLabel);

        rowLayout->addWidget(makeAvatar(row, QStringLiteral("aiBotAvatar"), QStringLiteral("AI")),
                             0, Qt::AlignTop);
        rowLayout->addWidget(content, 0, Qt::AlignTop);
        rowLayout->addStretch();
        m_currentBotBubble = bubble;
    }

    // Add row before the bottom stretch
    m_chatLayout->insertWidget(m_chatLayout->count() - 1, row);
    // Row spacing via layout
    if (m_chatLayout->count() > 2) {
        // Insert a spacer widget for the 20px message spacing
        auto *spacer = new QWidget(m_chatContainer);
        spacer->setFixedHeight(PageLayout::Space20);
        spacer->setObjectName(QStringLiteral("aiMessageSpacer"));
        m_chatLayout->insertWidget(m_chatLayout->count() - 2, spacer);
    }
    scrollToBottom();
}

void AiChatWidget::showTypingIndicator()
{
    if (m_typingRow != nullptr) {
        return;
    }

    m_typingRow = new QWidget(m_chatContainer);
    auto *rowLayout = new QHBoxLayout(m_typingRow);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(PageLayout::Space12);
    rowLayout->addWidget(makeAvatar(m_typingRow, QStringLiteral("aiBotAvatar"), QStringLiteral("AI")),
                         0, Qt::AlignTop);
    rowLayout->addWidget(makeTypingBubble(m_typingRow), 0, Qt::AlignTop);
    rowLayout->addStretch();

    m_chatLayout->insertWidget(m_chatLayout->count() - 1, m_typingRow);
    scrollToBottom();
}

void AiChatWidget::hideTypingIndicator()
{
    if (m_typingRow == nullptr) {
        return;
    }
    m_chatLayout->removeWidget(m_typingRow);
    m_typingRow->deleteLater();
    m_typingRow = nullptr;
}

void AiChatWidget::appendToBotBubble(const QString &chunk)
{
    m_streamingBuffer.append(chunk);
    if (!m_streamFlushTimer->isActive()) {
        m_streamFlushTimer->start();
    }
}

void AiChatWidget::flushBotBubble()
{
    m_streamFlushTimer->stop();
    if (m_streamingBuffer.isEmpty() || m_currentBotBubble == nullptr) {
        return;
    }
    const QString text = m_streamingBuffer;
    m_streamingBuffer.clear();
    m_currentBotBubble->setText(m_currentBotBubble->text() + text);
    scrollToBottom();
}

void AiChatWidget::setSelectedChip(int index)
{
    if (index < 0 || index >= m_actionChips.size()) {
        return;
    }
    m_selectedChipIndex = index;
    for (int i = 0; i < m_actionChips.size(); ++i) {
        QPushButton *chip = m_actionChips.at(i);
        const bool selected = i == index;
        chip->setProperty("selected", selected);
        chip->style()->unpolish(chip);
        chip->style()->polish(chip);
        chip->update();
    }
}

void AiChatWidget::setBusy(bool busy)
{
    if (m_sendButton != nullptr) {
        m_sendButton->setVisible(!busy);
        m_sendButton->setEnabled(!busy && !m_input->toPlainText().trimmed().isEmpty());
    }
    if (m_stopButton != nullptr) {
        m_stopButton->setVisible(busy);
        m_stopButton->setEnabled(busy);
    }
    if (m_input != nullptr) {
        m_input->setEnabled(!busy);
    }
}

void AiChatWidget::scrollToBottom()
{
    if (m_scrollArea != nullptr && m_scrollArea->verticalScrollBar() != nullptr) {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum());
    }
}
