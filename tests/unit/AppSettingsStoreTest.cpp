#include "AppSettingsStoreTest.h"

#include "infra/AppSettingsStore.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

void AppSettingsStoreTest::roundTripsSettings()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = QDir(dir.path()).filePath(QStringLiteral("settings.json"));
    AppSettingsStore store(filePath);
    const AppSettings input{
        QStringLiteral("D:/deploy-config"),
        QStringLiteral("D:/install/maven"),
        QStringLiteral("D:/m2/repository"),
        QStringLiteral("D:/drivers/postgresql-42.7.3.jar"),
        QStringLiteral("D:/drivers/ojdbc8.jar")
    };
    QString error;
    QVERIFY2(store.save(input, &error), qPrintable(error));

    AppSettings loaded;
    QVERIFY2(store.load(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.configDirOverride, input.configDirOverride);
    QCOMPARE(loaded.mavenHome, input.mavenHome);
    QCOMPARE(loaded.mavenRepository, input.mavenRepository);
    QCOMPARE(loaded.postgresDriverJar, input.postgresDriverJar);
    QCOMPARE(loaded.oracleDriverJar, input.oracleDriverJar);
}
