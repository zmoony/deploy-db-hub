#pragma once

#include <QTest>

class CommonToolsTest : public QObject {
    Q_OBJECT

private slots:
    void testFormatJsonValid();
    void testFormatJsonInvalid();

    void testBase64RoundTrip();

    void testUrlEncodeDecode();

    void testComputeHashes();

    void testGenerateUuids();

    void testConvertNumberBase();

    void testConvertCase();

    void testAnalyzeCron();

    void testDiffLineIndices();

    void testHexToString();

    void testHtmlEncodeDecode();
};
