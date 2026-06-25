#pragma once

#include <QJsonArray>
#include <QWidget>

class AiSettingsStore;
class AiStreamBuffer;
class CredentialStore;
class QLabel;
class OpenAiChatClient;
class QPlainTextEdit;
class QPushButton;

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
    void appendHistoryLine(const QString &roleLabel, const QString &text);
    void setBusy(bool busy);

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    OpenAiChatClient *m_client = nullptr;
    AiStreamBuffer *m_historyBuffer = nullptr;
    QJsonArray m_messages;
    QString m_pendingAssistantText;
    QPlainTextEdit *m_history = nullptr;
    QPlainTextEdit *m_input = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QLabel *m_message = nullptr;
};
