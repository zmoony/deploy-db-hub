#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

class ServiceInstanceStore
{
public:
    explicit ServiceInstanceStore(QString filePath);

    bool load(const QString &productKey, QVector<QJsonObject> *instances, QString *error) const;
    bool save(const QString &productKey, const QVector<QJsonObject> &instances, QString *error) const;

    static QString defaultFilePath();

private:
    bool loadAll(QJsonObject *document, QString *error) const;
    bool saveAll(const QJsonObject &document, QString *error) const;

    QString m_filePath;
};
