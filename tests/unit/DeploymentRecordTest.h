#pragma once

#include <QObject>

class DeploymentRecordTest final : public QObject
{
    Q_OBJECT

private slots:
    void createPendingUsesRelativeLogPath();
    void applyJobStateSetsFinishedAtOnFailure();
};
