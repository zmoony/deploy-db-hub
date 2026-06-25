#pragma once

#include <QObject>

class OpenAiStreamParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesDeltaChunks();
    void handlesDoneMarker();
    void ignoresMalformedJson();
};
