#pragma once

#include <QString>

enum class DeploymentStatus {
    Pending,
    Building,
    Uploading,
    Restarting,
    Success,
    Failed,
    Rollbacking,
    RolledBack,
    RollbackFailed,
    Canceled
};

enum class FailureStep {
    None,
    Validate,
    Building,
    Uploading,
    CurrentSwitch,
    Restarting,
    Rollback
};

enum class FailureStrategy {
    Keep,
    Rollback
};

QString toString(DeploymentStatus status);
QString toString(FailureStep step);
QString toString(FailureStrategy strategy);
