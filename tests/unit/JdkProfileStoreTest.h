#pragma once

#include <QObject>

class JdkProfileStoreTest final : public QObject
{
    Q_OBJECT

private slots:
    void savesAndLoadsProfiles();
    void injectsSelectedJdkIntoBuildEnvironment();
    void leavesEnvironmentUntouchedForSystemJdk();
};
