#pragma once

#include <QString>
#include <QWidget>

class AiSettingsStore;
class CredentialStore;
class QLabel;
class OpenAiChatClient;
class QPushButton;
class QScrollArea;
class QTimer;
class QVBoxLayout;

class LogAiAnalysisWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit LogAiAnalysisWidget(AiSettingsStore *aiSettings,
                                 CredentialStore *credentials,
                                 QWidget *parent = nullptr);

    void setLogContent(const QString &logContent);
    void analyzeLog();
    void abortAnalysis();

private:
    enum class BubbleRole { User, Bot };

    void appendBubble(BubbleRole role, const QString &text);
    void appendToBotBubble(const QString &chunk);
    void flushBotBubble();
    void clearConversation();
    void scrollToBottom();

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    OpenAiChatClient *m_client = nullptr;
    QString m_logContent;
    QString m_streamingBuffer;

    QPushButton *m_analyzeButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QLabel *m_message = nullptr;

    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_chatContainer = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
    QLabel *m_currentBotBubble = nullptr;

    QTimer *m_streamFlushTimer = nullptr;
};
