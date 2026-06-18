#include "core/DeployOrchestrator.h"

#include "core/DeploymentRecord.h"
#include "core/Version.h"
#include "infra/LogPath.h"

#include <QDateTime>

DeployOrchestrator::ActiveDeployment::ActiveDeployment(FailureStrategy strategy)
    : job(strategy)
{
}

DeployOrchestrator::DeployOrchestrator(ConfigStore &store, QString dataDir)
    : m_store(store)
    , m_dataDir(std::move(dataDir))
{
}

bool DeployOrchestrator::begin(const QString &projectId,
                             const QString &serverId,
                             FailureStrategy strategy,
                             ActiveDeployment *deployment,
                             QString *error)
{
    if (!deployment) {
        if (error) {
            *error = QStringLiteral("deployment output is null");
        }
        return false;
    }

    const QDateTime startedAt = QDateTime::currentDateTime();
    const QString version = Version::fromDateTime(startedAt);
    const QString id = QStringLiteral("deploy-") + version;

    if (!LogPath::isValidDeployId(id)) {
        if (error) {
            *error = QStringLiteral("generated deploy id is invalid");
        }
        return false;
    }

    ActiveDeployment active(strategy);
    active.id = id;
    active.version = version;
    active.relativeLogPath = LogPath::relativePath(id);
    active.record = DeploymentRecord::createPending(id, projectId, serverId, version, startedAt);
    active.logWriter = std::make_unique<LogWriter>(LogPath::absolutePath(m_dataDir, active.relativeLogPath));

    if (!active.logWriter->open(error)) {
        return false;
    }

    if (!m_store.upsertDeployment(active.record, error)) {
        return false;
    }

    *deployment = std::move(active);
    return true;
}

bool DeployOrchestrator::persist(const ActiveDeployment &deployment, QString *error) const
{
    return m_store.upsertDeployment(deployment.record, error);
}

bool DeployOrchestrator::persistJobState(ActiveDeployment *deployment, QString *error) const
{
    if (!deployment) {
        if (error) {
            *error = QStringLiteral("deployment is null");
        }
        return false;
    }

    deployment->record = DeploymentRecord::applyJobState(deployment->record, deployment->job);
    return m_store.upsertDeployment(deployment->record, error);
}

bool DeployOrchestrator::completeSuccess(ActiveDeployment *deployment,
                                         const QString &remoteVersionDir,
                                         const QStringList &artifactNames,
                                         QString *error)
{
    if (!deployment) {
        if (error) {
            *error = QStringLiteral("deployment is null");
        }
        return false;
    }

    if (!deployment->job.transitionTo(DeploymentStatus::Success)) {
        if (error) {
            *error = QStringLiteral("invalid transition to success");
        }
        return false;
    }

    deployment->record = DeploymentRecord::withSuccessDetails(deployment->record, remoteVersionDir, artifactNames);
    deployment->record = DeploymentRecord::applyJobState(deployment->record, deployment->job);
    return m_store.upsertDeployment(deployment->record, error);
}

bool DeployOrchestrator::markRolledBackFrom(ActiveDeployment *deployment, const QString &version, QString *error)
{
    if (!deployment) {
        if (error) {
            *error = QStringLiteral("deployment is null");
        }
        return false;
    }

    deployment->record[QStringLiteral("rolledBackFrom")] = version;
    return m_store.upsertDeployment(deployment->record, error);
}
