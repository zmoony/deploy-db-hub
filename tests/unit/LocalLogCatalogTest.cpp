#include "LocalLogCatalogTest.h"

#include "infra/LocalLogCatalog.h"

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
