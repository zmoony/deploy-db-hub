#pragma once

#include <QString>

struct SourceResult {
    bool ok = false;
    QString workingTree;
    QString error;
};

class GitSourceProvider final
{
public:
    static bool isCommitSha(const QString &ref);

    SourceResult prepare(const QString &workspaceRoot, const QString &projectId, const QString &repoUrl, const QString &ref) const;
};
