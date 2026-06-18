#pragma once

#include <QObject>

class DeployLogPathOptionsTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsConfiguredLogPathAsDefault();
    void normalizesConfiguredDirectoryLogDir();
    void buildsRemoteLogFileDiscoveryCommand();
    void parsesRemoteLogGlobPath();
    void detectsRemoteLogPaths();
};
