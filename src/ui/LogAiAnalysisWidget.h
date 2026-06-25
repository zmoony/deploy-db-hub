#pragma once

#include <QWidget>

class AiSettingsStore;
class AiStreamBuffer;
class CredentialStore;
class QLabel;
class OpenAiChatClient;
class QPlainTextEdit;
class QPushButton;

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
    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    OpenAiChatClient *m_client = nullptr;
    AiStreamBuffer *m_outputBuffer = nullptr;
    QString m_logContent;
    QPushButton *m_analyzeButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPlainTextEdit *m_output = nullptr;
    QLabel *m_message = nullptr;
};
