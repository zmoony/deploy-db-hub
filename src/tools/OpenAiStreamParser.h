#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

struct OpenAiStreamParseResult {
    QStringList deltas;
    bool done = false;
};

OpenAiStreamParseResult parseOpenAiStreamChunk(QString *buffer, const QByteArray &chunk);
