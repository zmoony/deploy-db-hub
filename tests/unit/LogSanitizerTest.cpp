#include "LogSanitizerTest.h"

#include "infra/LogSanitizer.h"

#include <QTest>

void LogSanitizerTest::masksPasswordTokenAndAuthorization()
{
    const QString input = QStringLiteral("password=abc token: xyz Authorization: Bearer secret");

    const QString output = LogSanitizer::sanitize(input);

    QVERIFY(!output.contains(QStringLiteral("abc")));
    QVERIFY(!output.contains(QStringLiteral("xyz")));
    QVERIFY(!output.contains(QStringLiteral("Bearer secret")));
    QVERIFY(output.contains(QStringLiteral("***")));
}

void LogSanitizerTest::masksPrivateKeyBlock()
{
    const QString input = QStringLiteral("before\n-----BEGIN PRIVATE KEY-----\nabc\n-----END PRIVATE KEY-----\nafter");

    const QString output = LogSanitizer::sanitize(input);

    QVERIFY(output.contains(QStringLiteral("before")));
    QVERIFY(output.contains(QStringLiteral("after")));
    QVERIFY(!output.contains(QStringLiteral("abc")));
    QVERIFY(output.contains(QStringLiteral("***")));
}
