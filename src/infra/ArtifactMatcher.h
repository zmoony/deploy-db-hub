#pragma once

#include <QString>
#include <QStringList>

enum class ArtifactMatchPolicy {
    FailIfMultiple,
    Newest
};

struct ArtifactMatchResult {
    bool ok = false;
    QStringList paths;
    QString error;
};

class ArtifactMatcher final
{
public:
    static ArtifactMatchResult match(const QString &projectRoot, const QString &artifactPath, ArtifactMatchPolicy policy);
};
