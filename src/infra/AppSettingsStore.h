#pragma once

#include <QString>

struct AppSettings {
    QString configDirOverride;
    QString mavenHome;
    QString mavenRepository;
    QString postgresDriverJar;
    QString oracleDriverJar;
    QString themeMode = QStringLiteral("system");
};

class AppSettingsStore final
{
public:
    explicit AppSettingsStore(QString filePath);

    bool load(AppSettings *settings, QString *error) const;
    bool save(const AppSettings &settings, QString *error) const;

    static QString defaultSettingsFile();
    static QString bootstrapSettingsFile();

private:
    QString m_filePath;
};
