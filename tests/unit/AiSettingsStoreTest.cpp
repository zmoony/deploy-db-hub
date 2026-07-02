#include "AiSettingsStoreTest.h"

#include "infra/AiSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
    QCOMPARE(loaded.connections.size(), 1);
    QCOMPARE(loaded.connections.first().apiBaseUrl, input.apiBaseUrl);
    QCOMPARE(loaded.activeConnectionId, loaded.connections.first().id);
}

void AiSettingsStoreTest::migratesLegacySingleConnection()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = QDir(dir.path()).filePath(QStringLiteral("ai-settings.json"));
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QJsonObject object{
            {QStringLiteral("schemaVersion"), 1},
            {QStringLiteral("apiBaseUrl"), QStringLiteral("https://api.example.com/v1")},
            {QStringLiteral("model"), QStringLiteral("custom-model")},
            {QStringLiteral("credentialRef"), QStringLiteral("deploy-hub/ai-api-key")},
            {QStringLiteral("rememberKey"), true}
        };
        file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    }

    AiSettingsStore store(filePath);
    AiSettings loaded;
    QString error;
    QVERIFY2(store.load(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.connections.size(), 1);
    QCOMPARE(loaded.connections.first().apiBaseUrl, QStringLiteral("https://api.example.com/v1"));
    QCOMPARE(loaded.connections.first().model, QStringLiteral("custom-model"));
    QCOMPARE(loaded.activeConnectionId, loaded.connections.first().id);
}

void AiSettingsStoreTest::persistsMultipleConnections()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString filePath = QDir(dir.path()).filePath(QStringLiteral("ai-settings.json"));
    AiSettingsStore store(filePath);

    AiSettings input;
    AiConnection a;
    a.id = QStringLiteral("conn-a");
    a.name = QStringLiteral("OpenAI 主账号");
    a.apiBaseUrl = QStringLiteral("https://api.openai.com/v1");
    a.model = QStringLiteral("gpt-4o-mini");
    a.credentialRef = AiSettingsStore::buildCredentialRef(a.id);
    AiConnection b;
    b.id = QStringLiteral("conn-b");
    b.name = QStringLiteral("内部代理");
    b.apiBaseUrl = QStringLiteral("https://proxy.example.com/v1");
    b.model = QStringLiteral("Qwen/Qwen2-7B");
    b.credentialRef = AiSettingsStore::buildCredentialRef(b.id);
    input.connections.append(a);
    input.connections.append(b);
    input.activeConnectionId = b.id;
    input.apiBaseUrl = b.apiBaseUrl;
    input.model = b.model;
    input.credentialRef = b.credentialRef;
    input.rememberKey = true;

    QString error;
    QVERIFY2(store.save(input, &error), qPrintable(error));

    AiSettings loaded;
    QVERIFY2(store.load(&loaded, &error), qPrintable(error));
    QCOMPARE(loaded.connections.size(), 2);
    QCOMPARE(loaded.activeConnectionId, b.id);
    QCOMPARE(loaded.connections.at(0).name, a.name);
    QCOMPARE(loaded.connections.at(1).apiBaseUrl, b.apiBaseUrl);
    QCOMPARE(loaded.connections.at(1).credentialRef, AiSettingsStore::buildCredentialRef(b.id));
}

void AiSettingsStoreTest::buildCredentialRef_isStable()
{
    const QString ref = AiSettingsStore::buildCredentialRef(QStringLiteral("conn-x"));
    QCOMPARE(ref, QStringLiteral("deploy-hub/ai/conn-x"));
}

void AiSettingsStoreTest::defaultModelPresets_returnsNonEmpty()
{
    const QStringList presets = AiSettingsStore::defaultModelPresets();
    QVERIFY(presets.size() > 0);
    QVERIFY(presets.contains(QStringLiteral("gpt-4o-mini")));
}
