#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

struct StoredRecord {
    QString id;
    QJsonObject config;
};

class ConfigStore final
{
public:
    explicit ConfigStore(QString databasePath);
    ~ConfigStore();

    ConfigStore(const ConfigStore &) = delete;
    ConfigStore &operator=(const ConfigStore &) = delete;

    bool open(QString *error);
    bool upsertProject(const QString &id, const QJsonObject &config, QString *error);
    bool upsertServer(const QString &id, const QJsonObject &config, QString *error);
    bool upsertDeployment(const QJsonObject &record, QString *error);

    bool listProjects(QVector<StoredRecord> *records, QString *error) const;
    bool listServers(QVector<StoredRecord> *records, QString *error) const;
    bool listDeployments(QVector<StoredRecord> *records, QString *error) const;
    bool getLatestDeploymentForProject(const QString &projectId, QJsonObject *record, QString *error) const;
    bool getProject(const QString &id, QJsonObject *config, QString *error) const;
    bool getServer(const QString &id, QJsonObject *config, QString *error) const;
    bool deleteProject(const QString &id, QString *error);
    bool deleteServer(const QString &id, QString *error);
    bool clearAllDeployments(int *removedCount, QString *error);

private:
    QString m_databasePath;
    QString m_connectionName;
};
