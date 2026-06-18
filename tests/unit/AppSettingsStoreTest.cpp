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
        QStringLiteral("D:/m2/repository")
    };
    QString error;
    QVERIFY2(store.save(input, &error), qPrintable(error));

    AppSettings loaded;
    QVERIFY2(store.load(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.configDirOverride, input.configDirOverride);
    QCOMPARE(loaded.mavenHome, input.mavenHome);
    QCOMPARE(loaded.mavenRepository, input.mavenRepository);
}
