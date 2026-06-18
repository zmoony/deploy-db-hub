#include "core/DeploymentState.h"

QString toString(DeploymentStatus status)
{
    switch (status) {
    case DeploymentStatus::Pending:
        return QStringLiteral("pending");
    case DeploymentStatus::Building:
        return QStringLiteral("building");
    case DeploymentStatus::Uploading:
        return QStringLiteral("uploading");
    case DeploymentStatus::Restarting:
        return QStringLiteral("restarting");
    case DeploymentStatus::Success:
        return QStringLiteral("success");
    case DeploymentStatus::Failed:
        return QStringLiteral("failed");
    case DeploymentStatus::Rollbacking:
        return QStringLiteral("rollbacking");
    case DeploymentStatus::RolledBack:
        return QStringLiteral("rolled_back");
    case DeploymentStatus::RollbackFailed:
        return QStringLiteral("rollback_failed");
    case DeploymentStatus::Canceled:
        return QStringLiteral("canceled");
    }
    return QStringLiteral("unknown");
}

QString toString(FailureStep step)
{
    switch (step) {
    case FailureStep::None:
        return QString();
    case FailureStep::Validate:
        return QStringLiteral("validate");
    case FailureStep::Building:
        return QStringLiteral("building");
    case FailureStep::Uploading:
        return QStringLiteral("uploading");
    case FailureStep::CurrentSwitch:
        return QStringLiteral("current_switch");
    case FailureStep::Restarting:
        return QStringLiteral("restarting");
    case FailureStep::Rollback:
        return QStringLiteral("rollback");
    }
    return QStringLiteral("unknown");
}

QString toString(FailureStrategy strategy)
{
    switch (strategy) {
    case FailureStrategy::Keep:
        return QStringLiteral("keep");
    case FailureStrategy::Rollback:
        return QStringLiteral("rollback");
    }
    return QStringLiteral("unknown");
}
