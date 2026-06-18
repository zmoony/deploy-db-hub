#pragma once

#include "core/DeploymentState.h"

#include <QString>

class DeployJob final
{
public:
    explicit DeployJob(FailureStrategy strategy);

    DeploymentStatus status() const;
    FailureStep failureStep() const;
    QString failureReason() const;

    bool transitionTo(DeploymentStatus next);
    bool fail(FailureStep step, const QString &reason, bool rollbackSucceeded);
    bool cancel();

private:
    static bool isRunning(DeploymentStatus status);
    bool canTransition(DeploymentStatus next) const;

    FailureStrategy m_strategy;
    DeploymentStatus m_status = DeploymentStatus::Pending;
    FailureStep m_failureStep = FailureStep::None;
    QString m_failureReason;
};
