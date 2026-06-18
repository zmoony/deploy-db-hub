#include "core/DeployJob.h"

DeployJob::DeployJob(FailureStrategy strategy)
    : m_strategy(strategy)
{
}

DeploymentStatus DeployJob::status() const
{
    return m_status;
}

FailureStep DeployJob::failureStep() const
{
    return m_failureStep;
}

QString DeployJob::failureReason() const
{
    return m_failureReason;
}

bool DeployJob::transitionTo(DeploymentStatus next)
{
    if (!canTransition(next)) {
        return false;
    }

    m_status = next;
    return true;
}

bool DeployJob::fail(FailureStep step, const QString &reason, bool rollbackSucceeded)
{
    if (!isRunning(m_status)) {
        return false;
    }

    m_failureStep = step;
    m_failureReason = reason;

    if (m_strategy == FailureStrategy::Rollback && step == FailureStep::Restarting) {
        m_status = rollbackSucceeded ? DeploymentStatus::RolledBack : DeploymentStatus::RollbackFailed;
        return true;
    }

    m_status = DeploymentStatus::Failed;
    return true;
}

bool DeployJob::cancel()
{
    if (!isRunning(m_status)) {
        return false;
    }

    m_status = DeploymentStatus::Canceled;
    return true;
}

bool DeployJob::isRunning(DeploymentStatus status)
{
    return status == DeploymentStatus::Pending
        || status == DeploymentStatus::Building
        || status == DeploymentStatus::Uploading
        || status == DeploymentStatus::Restarting
        || status == DeploymentStatus::Rollbacking;
}

bool DeployJob::canTransition(DeploymentStatus next) const
{
    switch (m_status) {
    case DeploymentStatus::Pending:
        return next == DeploymentStatus::Building;
    case DeploymentStatus::Building:
        return next == DeploymentStatus::Uploading || next == DeploymentStatus::Failed || next == DeploymentStatus::Canceled;
    case DeploymentStatus::Uploading:
        return next == DeploymentStatus::Restarting || next == DeploymentStatus::Failed || next == DeploymentStatus::Canceled;
    case DeploymentStatus::Restarting:
        return next == DeploymentStatus::Success
            || next == DeploymentStatus::Failed
            || next == DeploymentStatus::Rollbacking
            || next == DeploymentStatus::Canceled;
    case DeploymentStatus::Rollbacking:
        return next == DeploymentStatus::RolledBack || next == DeploymentStatus::RollbackFailed;
    case DeploymentStatus::Success:
    case DeploymentStatus::Failed:
    case DeploymentStatus::RolledBack:
    case DeploymentStatus::RollbackFailed:
    case DeploymentStatus::Canceled:
        return false;
    }
    return false;
}
