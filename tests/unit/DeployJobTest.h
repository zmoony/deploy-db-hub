#pragma once

#include <QObject>

class DeployJobTest final : public QObject
{
    Q_OBJECT

private slots:
    void successfulLifecycleEndsInSuccess();
    void restartFailureWithRollbackEndsRolledBack();
    void cancelFromUploadingEndsCanceled();
};
