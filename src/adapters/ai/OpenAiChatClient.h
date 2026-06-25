#pragma once

#include <QJsonArray>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class OpenAiChatClient final : public QObject
{
    Q_OBJECT

public:
    explicit OpenAiChatClient(QObject *parent = nullptr);
    ~OpenAiChatClient() override;

    void streamChat(const QString &apiBaseUrl,
                    const QString &apiKey,
                    const QString &model,
                    const QJsonArray &messages);
    void testConnection(const QString &apiBaseUrl, const QString &apiKey, const QString &model);
    void abort();
    bool isActive() const;

signals:
    void deltaReceived(const QString &chunk);
    void finished();
    void failed(const QString &error);

private slots:
    void onReadyRead();
    void onFinished();
    void onTimeout();

private:
    void startRequest(const QString &apiBaseUrl,
                      const QString &apiKey,
                      const QString &model,
                      const QJsonArray &messages,
                      bool stream,
                      int maxTokens);
    QString parseErrorMessage(const QByteArray &body, int httpStatus) const;
    void cleanupReply();

    QNetworkAccessManager *m_manager = nullptr;
    QNetworkReply *m_reply = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QString m_streamBuffer;
    bool m_streaming = false;
    // Set by onTimeout / abort paths so onFinished does not emit a duplicate
    // "Request timed out" right after we have already announced it.
    bool m_terminated = false;
};
