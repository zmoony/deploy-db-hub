#include "tools/OpenAiStreamParser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace {

QString extractDeltaContent(const QString &payload)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    const QJsonArray choices = document.object().value(QStringLiteral("choices")).toArray();
    if (choices.isEmpty()) {
        return {};
    }

    const QJsonObject choice = choices.at(0).toObject();
    const QJsonObject delta = choice.value(QStringLiteral("delta")).toObject();
    return delta.value(QStringLiteral("content")).toString();
}

}

OpenAiStreamParseResult parseOpenAiStreamChunk(QString *buffer, const QByteArray &chunk)
{
    OpenAiStreamParseResult result;
    if (buffer == nullptr || chunk.isEmpty()) {
        return result;
    }

    buffer->append(QString::fromUtf8(chunk));
    int newlineIndex = buffer->indexOf(QLatin1Char('\n'));
    while (newlineIndex >= 0) {
        QString line = buffer->left(newlineIndex).trimmed();
        *buffer = buffer->mid(newlineIndex + 1);
        newlineIndex = buffer->indexOf(QLatin1Char('\n'));

        if (line.isEmpty()) {
            continue;
        }
        if (line.startsWith(QStringLiteral("data:"))) {
            line = line.mid(5).trimmed();
        }
        if (line == QStringLiteral("[DONE]")) {
            result.done = true;
            continue;
        }

        const QString delta = extractDeltaContent(line);
        if (!delta.isEmpty()) {
            result.deltas.append(delta);
        }
    }

    return result;
}
