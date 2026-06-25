#include "OpenAiStreamParserTest.h"

#include "tools/OpenAiStreamParser.h"

#include <QTest>

void OpenAiStreamParserTest::parsesDeltaChunks()
{
    QString buffer;
    const QByteArray chunk = QByteArrayLiteral(
        "data: {\"choices\":[{\"delta\":{\"content\":\"Hel\"}}]}\n\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\"lo\"}}]}\n");
    const OpenAiStreamParseResult result = parseOpenAiStreamChunk(&buffer, chunk);
    QCOMPARE(result.deltas.size(), 2);
    QCOMPARE(result.deltas.at(0), QStringLiteral("Hel"));
    QCOMPARE(result.deltas.at(1), QStringLiteral("lo"));
    QVERIFY(!result.done);
}

void OpenAiStreamParserTest::handlesDoneMarker()
{
    QString buffer;
    const QByteArray chunk = QByteArrayLiteral("data: [DONE]\n");
    const OpenAiStreamParseResult result = parseOpenAiStreamChunk(&buffer, chunk);
    QVERIFY(result.deltas.isEmpty());
    QVERIFY(result.done);
}

void OpenAiStreamParserTest::ignoresMalformedJson()
{
    QString buffer;
    const QByteArray chunk = QByteArrayLiteral("data: not-json\n\ndata: {\"choices\":[{\"delta\":{\"content\":\"ok\"}}]}\n");
    const OpenAiStreamParseResult result = parseOpenAiStreamChunk(&buffer, chunk);
    QCOMPARE(result.deltas.size(), 1);
    QCOMPARE(result.deltas.at(0), QStringLiteral("ok"));
}
