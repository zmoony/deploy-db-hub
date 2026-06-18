#include "LogPathTest.h"

#include "infra/LogPath.h"

#include <QTest>

void LogPathTest::buildsRelativePath()
{
    QCOMPARE(LogPath::relativePath(QStringLiteral("deploy-20260615-001")),
             QStringLiteral("logs/deploy-20260615-001.log"));
}

void LogPathTest::resolvesAbsolutePath()
{
    const QString absolute = LogPath::absolutePath(QStringLiteral("C:/data/deploy-hub"),
                                                   QStringLiteral("logs/deploy-20260615-001.log"));
    QVERIFY(absolute.endsWith(QStringLiteral("logs/deploy-20260615-001.log")));
}

void LogPathTest::rejectsInvalidDeployId()
{
    QVERIFY(LogPath::relativePath(QStringLiteral("bad id")).isEmpty());
}

void LogPathTest::rejectsInvalidRelativePath()
{
    QVERIFY(!LogPath::isValidRelativePath(QStringLiteral("/tmp/deploy.log")));
    QVERIFY(!LogPath::isValidRelativePath(QStringLiteral("logs/bad name.log")));
}
