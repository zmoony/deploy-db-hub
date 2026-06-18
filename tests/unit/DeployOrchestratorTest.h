#pragma once

#include <QObject>

class DeployOrchestratorTest final : public QObject
{
    Q_OBJECT

private slots:
    void beginCreatesLogFileAndPendingRecord();
    void persistJobStateUpdatesBuildingStatus();
    void completeSuccessPersistsTerminalRecord();
};
