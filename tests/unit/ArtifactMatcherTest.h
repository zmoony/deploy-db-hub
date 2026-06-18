#pragma once

#include <QObject>

class ArtifactMatcherTest final : public QObject
{
    Q_OBJECT

private slots:
    void failsWhenNoArtifactMatches();
    void failsWhenMultipleArtifactsMatchByDefault();
    void picksNewestWhenPolicyIsNewest();
};
