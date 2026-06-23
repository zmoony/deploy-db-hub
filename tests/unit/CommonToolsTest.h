#pragma once

#include <QObject>

class CommonToolsTest final : public QObject
{
    Q_OBJECT

private slots:
    void formatsJsonObject();
    void reportsInvalidJson();
    void convertsBase64Text();
    void decodesHexText();
    void convertsUnixTimestamp();
    void returnsReferenceRows();
    void comparesTextLines();
    void matchesRegularExpression();
    void runsRegularExpressionWithGroups();
    void masksSensitiveText();
    void generatesMockFromJsonExample();
    void describesCronExpression();
    void encodesAndDecodesHtmlEntities();
    void convertsTimestampBothDirections();
    void computesLineDiffIndices();
    void analyzesCronNextRuns();
    void generatesUuids();
    void computesHashes();
    void encodesAndDecodesUrl();
    void decodesJwt();
    void convertsNumberBase();
    void convertsNamingCase();
};
