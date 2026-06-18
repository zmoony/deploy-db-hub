#pragma once

#include <QObject>

class ConfigStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsInvalidProject();
    void rejectsInvalidServer();
    void upsertsPendingDeployment();
    void rejectsDeploymentWithInvalidLogPath();
    void listsDeploymentsInStartedAtOrder();
    void getsLatestDeploymentForProject();
    void clearsAllDeployments();
    void opensSameDatabaseFromWorkerThread();
};
