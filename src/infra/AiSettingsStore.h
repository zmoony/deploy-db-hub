#pragma once

#include <QString>

struct AiSettings {
    QString apiBaseUrl;
    QString model = QStringLiteral("gpt-4o-mini");
    QString credentialRef = QStringLiteral("deploy-hub/ai-api-key");
    bool rememberKey = true;
};

class AiSettingsStore final
{
public:
    explicit AiSettingsStore(QString filePath);

    bool load(AiSettings *settings, QString *error) const;
    bool save(const AiSettings &settings, QString *error) const;

    static QString defaultSettingsFile();

private:
    QString m_filePath;
};
