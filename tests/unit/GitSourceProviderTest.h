#pragma once

#include <QObject>

class GitSourceProviderTest final : public QObject
{
    Q_OBJECT

private slots:
    void detectsCommitSha();
    void treatsBranchNamesAsNonSha();
};
