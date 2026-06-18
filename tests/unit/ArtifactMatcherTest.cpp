#include "ArtifactMatcherTest.h"

#include "infra/ArtifactMatcher.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

void ArtifactMatcherTest::failsWhenNoArtifactMatches()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const auto result = ArtifactMatcher::match(dir.path(), QStringLiteral("target/*.jar"), ArtifactMatchPolicy::FailIfMultiple);

    QVERIFY(!result.ok);
    QCOMPARE(result.error, QStringLiteral("未找到构建产物"));
}

void ArtifactMatcherTest::failsWhenMultipleArtifactsMatchByDefault()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("target")));
    QVERIFY(QFile(dir.path() + QStringLiteral("/target/a.jar")).open(QIODevice::WriteOnly));
    QVERIFY(QFile(dir.path() + QStringLiteral("/target/b.jar")).open(QIODevice::WriteOnly));

    const auto result = ArtifactMatcher::match(dir.path(), QStringLiteral("target/*.jar"), ArtifactMatchPolicy::FailIfMultiple);

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains(QStringLiteral("匹配到多个构建产物")));
}

void ArtifactMatcherTest::picksNewestWhenPolicyIsNewest()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("target")));

    QFile first(dir.path() + QStringLiteral("/target/a.jar"));
    QVERIFY(first.open(QIODevice::WriteOnly));
    first.write("a");
    first.close();

    QThread::msleep(20);

    QFile second(dir.path() + QStringLiteral("/target/b.jar"));
    QVERIFY(second.open(QIODevice::WriteOnly));
    second.write("b");
    second.close();

    const auto result = ArtifactMatcher::match(dir.path(), QStringLiteral("target/*.jar"), ArtifactMatchPolicy::Newest);

    QVERIFY(result.ok);
    QCOMPARE(result.paths.size(), 1);
    QVERIFY(result.paths.first().endsWith(QStringLiteral("b.jar")));
}
