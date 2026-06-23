#include "CommonToolsTest.h"

#include "tools/CommonTools.h"

#include <QTest>

void CommonToolsTest::formatsJsonObject()
{
    QString error;
    const QString formatted = CommonTools::formatJson(QStringLiteral("{\"name\":\"deploy\",\"enabled\":true}"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(formatted.contains(QStringLiteral("\"name\": \"deploy\"")));
    QVERIFY(formatted.contains(QStringLiteral("\"enabled\": true")));
}

void CommonToolsTest::reportsInvalidJson()
{
    QString error;
    const QString formatted = CommonTools::formatJson(QStringLiteral("{bad"), &error);
    QVERIFY(formatted.isEmpty());
    QVERIFY(!error.isEmpty());
}

void CommonToolsTest::convertsBase64Text()
{
    QString error;
    const QString encoded = CommonTools::textToBase64(QStringLiteral("Deploy Hub"));
    QCOMPARE(encoded, QStringLiteral("RGVwbG95IEh1Yg=="));
    QCOMPARE(CommonTools::base64ToText(encoded, &error), QStringLiteral("Deploy Hub"));
    QVERIFY(error.isEmpty());
}

void CommonToolsTest::decodesHexText()
{
    QString error;
    QCOMPARE(CommonTools::hexToString(QStringLiteral("48656c6c6f205174"), &error), QStringLiteral("Hello Qt"));
    QVERIFY(error.isEmpty());
}

void CommonToolsTest::convertsUnixTimestamp()
{
    QString error;
    const QString text = CommonTools::timestampToLocalText(QStringLiteral("1717200000"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(text.contains(QStringLiteral("2024")));
}

void CommonToolsTest::returnsReferenceRows()
{
    const auto statuses = CommonTools::httpStatusRows();
    QVERIFY(!statuses.isEmpty());
    QCOMPARE(statuses.first().code, QStringLiteral("200"));

    const auto entities = CommonTools::htmlEntityRows();
    QVERIFY(!entities.isEmpty());
    QCOMPARE(entities.first().code, QStringLiteral("&quot;"));
}

void CommonToolsTest::comparesTextLines()
{
    const QString diff = CommonTools::compareLines(QStringLiteral("a\nb\nc"), QStringLiteral("a\nx\nc"));
    QVERIFY(diff.contains(QStringLiteral("  a")));
    QVERIFY(diff.contains(QStringLiteral("- b")));
    QVERIFY(diff.contains(QStringLiteral("+ x")));
}

void CommonToolsTest::matchesRegularExpression()
{
    QString error;
    const QString result = CommonTools::matchRegularExpression(QStringLiteral("(deploy)-(\\d+)"),
                                                               QStringLiteral("deploy-42"),
                                                               &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(result.contains(QStringLiteral("匹配 1")));
    QVERIFY(result.contains(QStringLiteral("deploy-42")));
}

void CommonToolsTest::runsRegularExpressionWithGroups()
{
    QString error;
    CommonTools::RegexOptions options;
    const QVector<CommonTools::RegexMatch> matches =
        CommonTools::runRegularExpression(QStringLiteral("(deploy)-(?<id>\\d+)"),
                                          QStringLiteral("deploy-42 deploy-7"),
                                          options,
                                          &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(matches.size(), 2);
    QCOMPARE(matches.first().text, QStringLiteral("deploy-42"));
    QCOMPARE(matches.first().groups.size(), 2);
    QCOMPARE(matches.first().groups.at(0).text, QStringLiteral("deploy"));
    QCOMPARE(matches.first().groups.at(1).name, QStringLiteral("id"));
    QCOMPARE(matches.first().groups.at(1).text, QStringLiteral("42"));
}

void CommonToolsTest::masksSensitiveText()
{
    const QString masked = CommonTools::maskSensitiveText(QStringLiteral("phone=13812345678 email=a@example.com"));
    QVERIFY(masked.contains(QStringLiteral("138****5678")));
    QVERIFY(masked.contains(QStringLiteral("a***@example.com")));
}

void CommonToolsTest::generatesMockFromJsonExample()
{
    QString error;
    const QString mock = CommonTools::mockFromJsonExample(QStringLiteral("{\"name\":\"demo\",\"count\":1,\"enabled\":true}"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(mock.contains(QStringLiteral("\"name\": \"name_mock_1\"")));
    QVERIFY(mock.contains(QStringLiteral("\"count\": 101")));
    QVERIFY(mock.contains(QStringLiteral("\"enabled\": false")));
}

void CommonToolsTest::describesCronExpression()
{
    QString error;
    const QString text = CommonTools::describeCron(QStringLiteral("*/5 * * * *"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(text.contains(QStringLiteral("每 5 分钟")));
}

void CommonToolsTest::encodesAndDecodesHtmlEntities()
{
    const QString source = QStringLiteral("<a href=\"x\">A & B</a>");
    const QString encoded = CommonTools::htmlEncode(source);
    QVERIFY(encoded.contains(QStringLiteral("&lt;a")));
    QVERIFY(encoded.contains(QStringLiteral("&amp;")));
    QVERIFY(encoded.contains(QStringLiteral("&quot;")));
    QCOMPARE(CommonTools::htmlDecode(encoded), source);
    QCOMPARE(CommonTools::htmlDecode(QStringLiteral("&#65;&#x42;")), QStringLiteral("AB"));
}

void CommonToolsTest::convertsTimestampBothDirections()
{
    const qint64 ms = 1717200000000LL;
    const QString text = CommonTools::timestampToDateText(ms, true, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    QString error;
    const qint64 back = CommonTools::dateTextToTimestampMs(text, QStringLiteral("yyyy-MM-dd HH:mm:ss"), &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QCOMPARE(back, ms);

    const QString seconds = CommonTools::timestampToDateText(1717200000LL, false, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    QCOMPARE(seconds, text);
}

void CommonToolsTest::computesLineDiffIndices()
{
    const CommonTools::LineDiff diff = CommonTools::diffLineIndices(
        QStringLiteral("a\nb\nc"), QStringLiteral("a\nx\nc"));
    QVERIFY(diff.leftChangedLines.contains(1));
    QVERIFY(diff.rightChangedLines.contains(1));
    QVERIFY(!diff.leftChangedLines.contains(0));
    QVERIFY(!diff.leftChangedLines.contains(2));
    QCOMPARE(diff.sameCount, 2);
}

void CommonToolsTest::analyzesCronNextRuns()
{
    QString error;
    const QDateTime from(QDate(2026, 1, 1), QTime(11, 0, 0));
    const CommonTools::CronSchedule schedule =
        CommonTools::analyzeCron(QStringLiteral("0 0 12 * * ?"), from, 3, &error);
    QVERIFY2(schedule.valid, qPrintable(error));
    QCOMPARE(schedule.nextRuns.size(), 3);
    const QDateTime first = schedule.nextRuns.first();
    QCOMPARE(first, QDateTime(QDate(2026, 1, 1), QTime(12, 0, 0)));

    QString invalidError;
    const CommonTools::CronSchedule invalid =
        CommonTools::analyzeCron(QStringLiteral("bad expr"), from, 3, &invalidError);
    QVERIFY(!invalid.valid);
}

void CommonToolsTest::generatesUuids()
{
    const QStringList plain = CommonTools::generateUuids(3, false, false);
    QCOMPARE(plain.size(), 3);
    QCOMPARE(plain.first().count(QLatin1Char('-')), 4);

    const QStringList compact = CommonTools::generateUuids(1, true, true);
    QCOMPARE(compact.size(), 1);
    QVERIFY(!compact.first().contains(QLatin1Char('-')));
    QCOMPARE(compact.first(), compact.first().toUpper());
    QCOMPARE(compact.first().size(), 32);
}

void CommonToolsTest::computesHashes()
{
    const CommonTools::HashResult result = CommonTools::computeHashes(QByteArrayLiteral("abc"));
    QCOMPARE(result.md5, QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
    QCOMPARE(result.sha256,
             QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}

void CommonToolsTest::encodesAndDecodesUrl()
{
    const QString source = QStringLiteral("a b&c=测试");
    const QString encoded = CommonTools::urlEncode(source);
    QVERIFY(encoded.contains(QStringLiteral("%20")));
    QCOMPARE(CommonTools::urlDecode(encoded), source);
}

void CommonToolsTest::decodesJwt()
{
    const QString token = QStringLiteral(
        "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
        "eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIn0."
        "abc");
    QString error;
    const QString decoded = CommonTools::decodeJwt(token, &error);
    QVERIFY2(error.isEmpty(), qPrintable(error));
    QVERIFY(decoded.contains(QStringLiteral("HS256")));
    QVERIFY(decoded.contains(QStringLiteral("John Doe")));
}

void CommonToolsTest::convertsNumberBase()
{
    QString error;
    const CommonTools::NumberBases bases = CommonTools::convertNumberBase(QStringLiteral("255"), 10, &error);
    QVERIFY2(bases.valid, qPrintable(error));
    QCOMPARE(bases.hex, QStringLiteral("FF"));
    QCOMPARE(bases.binary, QStringLiteral("11111111"));
    QCOMPARE(bases.octal, QStringLiteral("377"));
}

void CommonToolsTest::convertsNamingCase()
{
    const CommonTools::CaseForms forms = CommonTools::convertCase(QStringLiteral("deploy hub service"));
    QCOMPARE(forms.camel, QStringLiteral("deployHubService"));
    QCOMPARE(forms.pascal, QStringLiteral("DeployHubService"));
    QCOMPARE(forms.snake, QStringLiteral("deploy_hub_service"));
    QCOMPARE(forms.kebab, QStringLiteral("deploy-hub-service"));
    QCOMPARE(forms.constantCase, QStringLiteral("DEPLOY_HUB_SERVICE"));

    const CommonTools::CaseForms fromCamel = CommonTools::convertCase(QStringLiteral("deployHubService"));
    QCOMPARE(fromCamel.snake, QStringLiteral("deploy_hub_service"));
}
