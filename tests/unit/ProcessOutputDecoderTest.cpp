#include "ProcessOutputDecoderTest.h"

#include "infra/ProcessOutputDecoder.h"

#include <QTest>

void ProcessOutputDecoderTest::preservesUtf8Text()
{
    QCOMPARE(decodeProcessOutput(QStringLiteral("构建失败").toUtf8()),
             QStringLiteral("构建失败"));
}

void ProcessOutputDecoderTest::decodesWindowsCmdOutput()
{
#ifdef Q_OS_WIN
    const QByteArray bytes = QStringLiteral("'mvn' 不是内部或外部命令").toLocal8Bit();
    const QString decoded = decodeProcessOutput(bytes);
    QVERIFY(decoded.contains(QStringLiteral("不是内部或外部命令")));
#endif
}
