#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct AiConnection {
    QString id;
    QString name;
    QString apiBaseUrl;
    QString model;
    QString credentialRef;
};

struct AiSettings {
    QString apiBaseUrl;
    QString model = QStringLiteral("gpt-4o-mini");
    QString credentialRef = QStringLiteral("deploy-hub/ai-api-key");
    bool rememberKey = true;

    QVector<AiConnection> connections;
    QString activeConnectionId;

    static QString generateConnectionId();
};

class AiSettingsStore final
{
public:
    explicit AiSettingsStore(QString filePath);

    bool load(AiSettings *settings, QString *error) const;
    bool save(const AiSettings &settings, QString *error) const;

    static QString defaultSettingsFile();

    static QString buildCredentialRef(const QString &connectionId);
    static QStringList defaultModelPresets();

private:
    QString m_filePath;
};
