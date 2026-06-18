#include "JdkProfileStoreTest.h"

#include "infra/JdkProfileStore.h"

#include <QTemporaryDir>
#include <QTest>

void JdkProfileStoreTest::savesAndLoadsProfiles()
{
    QTemporaryDir temp;
    JdkProfileStore store(temp.filePath(QStringLiteral("jdk-profiles.json")));

    QVector<JdkProfile> profiles{
        {QStringLiteral("jdk17"), QStringLiteral("JDK 17"), QStringLiteral("D:/Java/jdk-17")},
        {QStringLiteral("jdk8"), QStringLiteral("JDK 8"), QStringLiteral("D:/Java/jdk8")}
    };
    QString error;
    QVERIFY2(store.save(profiles, &error), qPrintable(error));

    QVector<JdkProfile> loaded;
    QVERIFY2(store.load(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.size(), 2);
    QCOMPARE(loaded.at(0).id, QStringLiteral("jdk17"));
    QCOMPARE(loaded.at(0).version, QStringLiteral("JDK 17"));
    QCOMPARE(loaded.at(0).path, QStringLiteral("D:/Java/jdk-17"));
    QCOMPARE(loaded.at(1).id, QStringLiteral("jdk8"));
}

void JdkProfileStoreTest::injectsSelectedJdkIntoBuildEnvironment()
{
    QMap<QString, QString> environment;
    environment.insert(QStringLiteral("PATH"), QStringLiteral("C:/Windows/System32"));

    JdkProfile profile{QStringLiteral("jdk17"), QStringLiteral("JDK 17"), QStringLiteral("D:/Java/jdk-17")};
    JdkProfileStore::applyToEnvironment(profile, &environment);

    QCOMPARE(environment.value(QStringLiteral("JAVA_HOME")), QStringLiteral("D:/Java/jdk-17"));
    QVERIFY(environment.value(QStringLiteral("PATH")).startsWith(QStringLiteral("D:/Java/jdk-17/bin;")));
}

void JdkProfileStoreTest::leavesEnvironmentUntouchedForSystemJdk()
{
    QMap<QString, QString> environment;
    environment.insert(QStringLiteral("PATH"), QStringLiteral("C:/Windows/System32"));

    JdkProfileStore::applyToEnvironment(JdkProfileStore::systemProfile(), &environment);

    QVERIFY(!environment.contains(QStringLiteral("JAVA_HOME")));
    QCOMPARE(environment.value(QStringLiteral("PATH")), QStringLiteral("C:/Windows/System32"));
}
