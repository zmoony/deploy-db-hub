#pragma once

#include <QObject>

class ProcessOutputDecoderTest final : public QObject
{
    Q_OBJECT

private slots:
    void preservesUtf8Text();
    void decodesWindowsCmdOutput();
};
