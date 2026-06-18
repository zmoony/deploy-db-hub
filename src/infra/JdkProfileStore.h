#pragma once

#include <QMap>
#include <QString>
#include <QVector>

struct JdkProfile {
    QString id;
    QString version;
    QString path;
};

class JdkProfileStore final
{
public:
    explicit JdkProfileStore(QString filePath);

    bool load(QVector<JdkProfile> *profiles, QString *error) const;
    bool save(const QVector<JdkProfile> &profiles, QString *error) const;

    static JdkProfile systemProfile();
    static void applyToEnvironment(const JdkProfile &profile, QMap<QString, QString> *environment);

private:
    QString m_filePath;
};
