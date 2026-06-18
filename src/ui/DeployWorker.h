#pragma once

#include "adapters/remote/RemoteConnection.h"

#include <QObject>
#include <QString>

class DeployWorker final : public QObject
{
    Q_OBJECT

public:
    explicit DeployWorker(QString databasePath,
                          QString selectedJdkId,
                          RemoteConnectionContext connectionContext,
                          QObject *parent = nullptr);

public slots:
    void run(const QString &projectId, const QString &serverId);

signals:
    void logLine(const QString &stage, const QString &message);
    void progress(int value);
    void deploymentLogReady(const QString &logRelativePath);
    void finished(bool ok, const QString &summary, const QString &logRelativePath);

private:
    QString m_databasePath;
    QString m_selectedJdkId;
    RemoteConnectionContext m_connectionContext;
};
