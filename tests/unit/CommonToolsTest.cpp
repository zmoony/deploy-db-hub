#include "CommonToolsTest.h"

#include "tools/CommonTools.h"

#include <QByteArray>
#include <QDateTime>
#include <QStringList>
#include <QTimeZone>

void CommonToolsTest::testFormatJsonValid()
{
    QString error;
    const QString result = CommonTools::formatJson(QStringLiteral("{\"name\":\"deploy\"}"), &error);
    QVERIFY(error.isEmpty());
    QVERIFY(result.contains(QStringLiteral("\"name\"")));
    QVERIFY(result.contains(QStringLiteral("\"deploy\"")));
}

void CommonToolsTest::testFormatJsonInvalid()
{
    QString error;
    const QString result = CommonTools::formatJson(QStringLiteral("{invalid"), &error);
    QVERIFY(!error.isEmpty());
    QVERIFY(result.isEmpty());
}

void CommonToolsTest::testBase64RoundTrip()
{
    const QString original = QStringLiteral("Hello Deploy Hub 中文");
    const QString encoded = CommonTools::textToBase64(original);
    QVERIFY(!encoded.isEmpty());
    QString error;
    const QString decoded = CommonTools::base64ToText(encoded, &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(decoded, original);
}

void CommonToolsTest::testUrlEncodeDecode()
{
    const QString original = QStringLiteral("hello world?a=1&b=2");
    const QString encoded = CommonTools::urlEncode(original);
    QVERIFY(encoded != original);
    const QString decoded = CommonTools::urlDecode(encoded);
    QCOMPARE(decoded, original);
}

void CommonToolsTest::testComputeHashes()
{
    const QByteArray data = QByteArrayLiteral("hello");
    const CommonTools::HashResult result = CommonTools::computeHashes(data);
    QCOMPARE(result.md5, QStringLiteral("5d41402abc4b2a76b9719d911017c592"));
    QCOMPARE(result.sha1, QStringLiteral("aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d"));
    QVERIFY(!result.sha256.isEmpty());
}

void CommonToolsTest::testGenerateUuids()
{
    const QStringList uuids = CommonTools::generateUuids(3, false, false);
    QCOMPARE(uuids.size(), 3);
    for (const QString &uuid : uuids) {
        QCOMPARE(uuid.length(), 36);
        QVERIFY(uuid.contains(QLatin1Char('-')));
    }
}

void CommonToolsTest::testConvertNumberBase()
{
    QString error;
    const CommonTools::NumberBases result = CommonTools::convertNumberBase(QStringLiteral("255"), 10, &error);
    QVERIFY(error.isEmpty());
    QVERIFY(result.valid);
    QCOMPARE(result.decimal, QStringLiteral("255"));
    QCOMPARE(result.hex.toLower(), QStringLiteral("ff"));
    QCOMPARE(result.binary, QStringLiteral("11111111"));
}

void CommonToolsTest::testConvertCase()
{
    const CommonTools::CaseForms result = CommonTools::convertCase(QStringLiteral("hello world"));
    QCOMPARE(result.camel, QStringLiteral("helloWorld"));
    QCOMPARE(result.pascal, QStringLiteral("HelloWorld"));
    QCOMPARE(result.snake, QStringLiteral("hello_world"));
    QCOMPARE(result.kebab, QStringLiteral("hello-world"));
}

void CommonToolsTest::testAnalyzeCron()
{
    QString error;
    const QDateTime from(QDate(2026, 1, 1), QTime(0, 0), QTimeZone::systemTimeZone());
    const CommonTools::CronSchedule schedule = CommonTools::analyzeCron(QStringLiteral("0 9 * * *"), from, 3, &error);
    QVERIFY(error.isEmpty());
    QVERIFY(schedule.valid);
    QCOMPARE(schedule.nextRuns.size(), 3);
    QCOMPARE(schedule.expression, QStringLiteral("0 9 * * *"));
}

void CommonToolsTest::testDiffLineIndices()
{
    const CommonTools::LineDiff diff = CommonTools::diffLineIndices(QStringLiteral("a\nb\nc"), QStringLiteral("a\nx\nc"));
    QVERIFY(diff.diffCount > 0);
    QVERIFY(diff.leftChangedLines.contains(1));
    QVERIFY(diff.rightChangedLines.contains(1));
}

void CommonToolsTest::testHexToString()
{
    QString error;
    const QString result = CommonTools::hexToString(QStringLiteral("48656c6c6f"), &error);
    QVERIFY(error.isEmpty());
    QCOMPARE(result, QStringLiteral("Hello"));
}

void CommonToolsTest::testHtmlEncodeDecode()
{
    const QString original = QStringLiteral("<div>Hello & \"world\"</div>");
    const QString encoded = CommonTools::htmlEncode(original);
    QVERIFY(encoded != original);
    const QString decoded = CommonTools::htmlDecode(encoded);
    QCOMPARE(decoded, original);
}

