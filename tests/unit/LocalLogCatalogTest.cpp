#include "LocalLogCatalogTest.h"

#include "infra/LocalLogCatalog.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

void LocalLogCatalogTest::acceptsLogsRelativePath()
{
    QVERIFY(LocalLogCatalog::isValidRelativePath(QStringLiteral("logs/deploy-20260615-153000.log")));
}

void LocalLogCatalogTest::rejectsInvalidRelativePath()
{
    QVERIFY(!LocalLogCatalog::isValidRelativePath(QStringLiteral("/abs/deploy.log")));
    QVERIFY(!LocalLogCatalog::isValidRelativePath(QStringLiteral("logs/nested/a.log")));
}

void LocalLogCatalogTest::clearsLogFilesInDirectory()
{
    QTemporaryDir temp;
    const QString logPath = temp.filePath(QStringLiteral("deploy-demo.log"));
    QFile file(logPath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("demo");
    file.close();

    const LocalLogCleanupResult result = LocalLogCatalog::clearDirectory(temp.path());
    QCOMPARE(result.removedFiles, 1);
    QCOMPARE(result.failedFiles, 0);
    QVERIFY(result.freedBytes >= 4);
    QVERIFY(!QFile::exists(logPath));
}
