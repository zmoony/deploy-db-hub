#include "RemoteOutputCleanerTest.h"

#include "infra/RemoteOutputCleaner.h"

#include <QTest>

void RemoteOutputCleanerTest::stripSshBannerRemovesLoginWarning()
{
    const QString input = QStringLiteral(
        "Authorized users only. All activities may be monitored and reported.\n"
        "bash: /usr/local/kafka/bin/kafka-topics.sh: Permission denied");
    const QString cleaned = RemoteOutputCleaner::stripSshBanner(input);
    QVERIFY(!cleaned.contains(QStringLiteral("Authorized users only")));
    QVERIFY(cleaned.contains(QStringLiteral("Permission denied")));
}

void RemoteOutputCleanerTest::normalizeRemoteErrorMapsPermissionDenied()
{
    const QString message = RemoteOutputCleaner::normalizeRemoteError(
        QStringLiteral("kafka-topics.sh: Permission denied"));
    QCOMPARE(message, QStringLiteral("Kafka 脚本无执行权限，请在服务器上执行 chmod +x <安装目录>/bin/*.sh"));
}
