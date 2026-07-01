#pragma once

#include <QJsonArray>
#include <QWidget>

class AiSettingsStore;
class CredentialStore;
class QLabel;
class OpenAiChatClient;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QTimer;
class QVBoxLayout;

class AiChatWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit AiChatWidget(AiSettingsStore *aiSettings,
                          CredentialStore *credentials,
                          QWidget *parent = nullptr);

private slots:
    void sendMessage();
    void stopGeneration();
    void clearConversation();

private:
    enum class BubbleRole { User, Bot };
    void appendWelcomeMessage();
    void appendBubble(BubbleRole role, const QString &text);
    void appendToBotBubble(const QString &text);
    void flushBotBubble();
    void showTypingIndicator();
    void hideTypingIndicator();
    void setBusy(bool busy);
    void setSelectedChip(int index);
    void scrollToBottom();

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    OpenAiChatClient *m_client = nullptr;
    QJsonArray m_messages;
    QString m_pendingAssistantText;
    QString m_streamingBuffer;

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_chatContainer = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
    QWidget *m_typingRow = nullptr;
    QLabel *m_currentBotBubble = nullptr;

    QPlainTextEdit *m_input = nullptr;
    QWidget *m_inputTopRow = nullptr;
    QLabel *m_inputHint = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QLabel *m_message = nullptr;
    QTimer *m_streamFlushTimer = nullptr;
    QList<QPushButton *> m_actionChips;
    int m_selectedChipIndex = -1;
};
