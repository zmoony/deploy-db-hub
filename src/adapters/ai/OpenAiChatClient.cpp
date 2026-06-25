#include "adapters/ai/OpenAiChatClient.h"

#include "tools/OpenAiStreamParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace {

// Streaming responses may sit idle for a while waiting on the model; 120s is
// generous for typical OpenAI-compatible endpoints on a stable network.
constexpr int kRequestTimeoutMs = 120000;

QString normalizeApiBaseUrl(const QString &apiBaseUrl)
{
    QString base = apiBaseUrl.trimmed();
    while (base.endsWith(QLatin1Char('/'))) {
        base.chop(1);
    }
    return base;
}

}

OpenAiChatClient::OpenAiChatClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &OpenAiChatClient::onTimeout);
}

OpenAiChatClient::~OpenAiChatClient()
{
    abort();
}

void OpenAiChatClient::streamChat(const QString &apiBaseUrl,
                                  const QString &apiKey,
                                  const QString &model,
                                  const QJsonArray &messages)
{
    startRequest(apiBaseUrl, apiKey, model, messages, true, 0);
}

void OpenAiChatClient::testConnection(const QString &apiBaseUrl, const QString &apiKey, const QString &model)
{
    const QJsonArray messages{
        QJsonObject{
            {QStringLiteral("role"), QStringLiteral("user")},
            {QStringLiteral("content"), QStringLiteral("ping")}
        }
    };
    startRequest(apiBaseUrl, apiKey, model, messages, false, 1);
}

void OpenAiChatClient::abort()
{
    if (m_reply == nullptr) {
        return;
    }
    m_timeoutTimer->stop();
    m_terminated = true;
    m_reply->abort();
    cleanupReply();
}

bool OpenAiChatClient::isActive() const
{
    return m_reply != nullptr;
}

void OpenAiChatClient::startRequest(const QString &apiBaseUrl,
                                    const QString &apiKey,
                                    const QString &model,
                                    const QJsonArray &messages,
                                    bool stream,
                                    int maxTokens)
{
    abort();

    const QString base = normalizeApiBaseUrl(apiBaseUrl);
    if (base.isEmpty()) {
        emit failed(QStringLiteral("API Base URL is empty"));
        return;
    }
    if (apiKey.trimmed().isEmpty()) {
        emit failed(QStringLiteral("API Key is empty"));
        return;
    }
    if (model.trimmed().isEmpty()) {
        emit failed(QStringLiteral("Model is empty"));
        return;
    }

    QJsonObject body{
        {QStringLiteral("model"), model.trimmed()},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("stream"), stream}
    };
    if (!stream && maxTokens > 0) {
        body.insert(QStringLiteral("max_tokens"), maxTokens);
    }

    QNetworkRequest request(QUrl(base + QStringLiteral("/chat/completions")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey.trimmed().toUtf8());

    m_streaming = stream;
    m_streamBuffer.clear();
    m_reply = m_manager->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    // Re-parent the reply to the client. QNetworkAccessManager owns the reply
    // by default; if we also deleteLater() it in cleanupReply() the manager's
    // destructor will try to delete the same object again, crashing the app
    // when the client itself is torn down.
    m_reply->setParent(this);
    connect(m_reply, &QNetworkReply::readyRead, this, &OpenAiChatClient::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &OpenAiChatClient::onFinished);
    m_timeoutTimer->start(kRequestTimeoutMs);
}

void OpenAiChatClient::onReadyRead()
{
    if (m_reply == nullptr || !m_streaming) {
        return;
    }

    const OpenAiStreamParseResult result = parseOpenAiStreamChunk(&m_streamBuffer, m_reply->readAll());
    for (const QString &delta : result.deltas) {
        emit deltaReceived(delta);
    }
}

void OpenAiChatClient::onFinished()
{
    if (m_reply == nullptr) {
        return;
    }

    m_timeoutTimer->stop();
    const bool wasTerminated = m_terminated;
    m_terminated = false;

    const int httpStatus = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = m_reply->readAll();

    if (m_reply->error() != QNetworkReply::NoError) {
        if (m_reply->error() == QNetworkReply::OperationCanceledError) {
            cleanupReply();
            return;
        }
        const QString message = parseErrorMessage(body, httpStatus);
        const QString fallback = m_reply->errorString();
        cleanupReply();
        if (!wasTerminated) {
            emit failed(message.isEmpty() ? fallback : message);
        }
        return;
    }

    if (m_streaming) {
        const OpenAiStreamParseResult result = parseOpenAiStreamChunk(&m_streamBuffer, body);
        for (const QString &delta : result.deltas) {
            emit deltaReceived(delta);
        }
    }

    cleanupReply();
    emit finished();
}

void OpenAiChatClient::onTimeout()
{
    if (m_reply == nullptr) {
        return;
    }
    m_terminated = true;
    m_reply->abort();
    emit failed(QStringLiteral("Request timed out (120s) — check network / API endpoint"));
}

QString OpenAiChatClient::parseErrorMessage(const QByteArray &body, int httpStatus) const
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        const QJsonObject errorObject = document.object().value(QStringLiteral("error")).toObject();
        const QString message = errorObject.value(QStringLiteral("message")).toString().trimmed();
        if (!message.isEmpty()) {
            return message;
        }
    }

    if (httpStatus > 0) {
        return QStringLiteral("HTTP %1").arg(httpStatus);
    }
    return {};
}

void OpenAiChatClient::cleanupReply()
{
    if (m_reply == nullptr) {
        return;
    }
    m_reply->deleteLater();
    m_reply = nullptr;
    m_streamBuffer.clear();
    m_streaming = false;
}
