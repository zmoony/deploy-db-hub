#include "AiSettingsStoreTest.h"

#include "infra/AiSettingsStore.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>

void AiSettingsStoreTest::roundTripsSettings()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = QDir(dir.path()).filePath(QStringLiteral("ai-settings.json"));
    AiSettingsStore store(filePath);
    const AiSettings input{
        QStringLiteral("https://api.openai.com/v1"),
        QStringLiteral("gpt-4o-mini"),
        QStringLiteral("deploy-hub/ai-api-key"),
        true
    };
    QString error;
    QVERIFY2(store.save(input, &error), qPrintable(error));

    AiSettings loaded;
    QVERIFY2(store.load(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.apiBaseUrl, input.apiBaseUrl);
    QCOMPARE(loaded.model, input.model);
    QCOMPARE(loaded.credentialRef, input.credentialRef);
    QCOMPARE(loaded.rememberKey, input.rememberKey);
}
