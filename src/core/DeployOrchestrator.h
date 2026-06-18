#pragma once

#include "core/DeployJob.h"
#include "infra/ConfigStore.h"
#include "infra/LogWriter.h"

#include <QJsonObject>
#include <memory>

class DeployOrchestrator final
{
public:
    explicit DeployOrchestrator(ConfigStore &store, QString dataDir);

    struct ActiveDeployment {
        explicit ActiveDeployment(FailureStrategy strategy);

        QString id;
        QString version;
        QString relativeLogPath;
        DeployJob job;
        QJsonObject record;
        std::unique_ptr<LogWriter> logWriter;
    };

    bool begin(const QString &projectId,
               const QString &serverId,
               FailureStrategy strategy,
               ActiveDeployment *deployment,
               QString *error);

    bool persist(const ActiveDeployment &deployment, QString *error) const;

    bool persistJobState(ActiveDeployment *deployment, QString *error) const;

    bool completeSuccess(ActiveDeployment *deployment,
                         const QString &remoteVersionDir,
                         const QStringList &artifactNames,
                         QString *error);

    bool markRolledBackFrom(ActiveDeployment *deployment, const QString &version, QString *error);

private:
    ConfigStore &m_store;
    QString m_dataDir;
};
