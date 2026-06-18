#include "VersionTest.h"

#include "core/Version.h"

#include <QDateTime>
#include <QTest>
#include <QTimeZone>

void VersionTest::formatsVersionTimestamp()
{
    const QDateTime timestamp(QDate(2026, 6, 15), QTime(15, 30, 0), QTimeZone(8 * 3600));

    QCOMPARE(Version::fromDateTime(timestamp), QStringLiteral("20260615-153000"));
}

void VersionTest::rejectsInvalidVersionTimestamp()
{
    QVERIFY(Version::isValid(QStringLiteral("20260615-153000")));
    QVERIFY(!Version::isValid(QStringLiteral("2026-06-15T15:30:00")));
}
