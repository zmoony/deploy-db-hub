#pragma once

#include <QObject>

class DeployLogPathOptionsTest final : public QObject
{
    Q_OBJECT

private slots:
    void includesProjectDeployLogDirGlob();
    void includesRemoteBaseDirLogCandidates();
    void parsesRemoteLogGlobPath();
    void detectsRemoteLogPaths();
};
