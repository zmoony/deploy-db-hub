#include "infra/ProcessOutputDecoder.h"

#include <QStringDecoder>

QString decodeProcessOutput(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        return {};
    }

#ifdef Q_OS_WIN
    const QString systemText = QStringDecoder(QStringDecoder::System).decode(bytes);
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    const QString utf8Text = utf8Decoder.decode(bytes);
    if (!utf8Decoder.hasError() && utf8Text.toUtf8() == bytes) {
        return utf8Text;
    }
    return systemText;
#else
    QStringDecoder utf8Decoder(QStringDecoder::Utf8);
    const QString utf8Text = utf8Decoder.decode(bytes);
    if (!utf8Decoder.hasError()) {
        return utf8Text;
    }
    return QString::fromLocal8Bit(bytes);
#endif
}
