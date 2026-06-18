#pragma once

#include <QObject>

class ConfigValidatorTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsValidLocalProject();
    void acceptsProjectWithCustomServiceCommands();
    void rejectsGithubProjectWithoutRef();
    void rejectsWindowsServerWithSshKeyAuth();
    void rejectsServerWithoutDefaultRemoteBaseDir();
    void rejectsInvalidWinrmFingerprint();
    void acceptsValidSuccessDeployment();
    void rejectsFailedDeploymentWithoutFailureStep();
    void rejectsDeploymentWithInvalidLogPath();
    void acceptsPendingDeploymentWithoutFinishedAt();
};
