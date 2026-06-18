#include "GitSourceProviderTest.h"

#include "adapters/git/GitSourceProvider.h"

#include <QTest>

void GitSourceProviderTest::detectsCommitSha()
{
    QVERIFY(GitSourceProvider::isCommitSha(QStringLiteral("a1b2c3d")));
    QVERIFY(GitSourceProvider::isCommitSha(QStringLiteral("a1b2c3d4e5f6789012345678901234567890abcd")));
}

void GitSourceProviderTest::treatsBranchNamesAsNonSha()
{
    QVERIFY(!GitSourceProvider::isCommitSha(QStringLiteral("main")));
    QVERIFY(!GitSourceProvider::isCommitSha(QStringLiteral("release/v1")));
    QVERIFY(!GitSourceProvider::isCommitSha(QStringLiteral("v1.0.0")));
}
