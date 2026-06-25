#pragma once

#include <QObject>

class KafkaTopicParseTest final : public QObject
{
    Q_OBJECT

private slots:
    void parseOffsetLinesSupportsColonFormat();
    void parseOffsetLinesSkipsNegativeOffsets();
    void parseDescribeTopicsOutputExtractsSummary();
};
