#include "RemoteFileBrowserTest.h"

#include "adapters/remote/StubRemoteVfs.h"

#include <QJsonObject>
#include <QTest>

void RemoteFileBrowserTest::stubReadFileTailReturnsLastRequestedLines()
{
    StubRemoteVfs vfs(QJsonObject{});
    QStringList lines;
    for (int i = 1; i <= 150; ++i) {
        lines.append(QStringLiteral("line %1").arg(i));
    }

    const QString remotePath = QStringLiteral("/tmp/long.log");
    QVERIFY(vfs.uploadFile(__FILE__, remotePath).ok);
    QVERIFY(vfs.writeFile(remotePath, lines.join(QLatin1Char('\n'))).ok);

    const RemoteFileReadResult result = vfs.readFileTail(remotePath, 100);

    QVERIFY2(result.ok, qPrintable(result.error));
    const QStringList tailLines = result.content.split(QLatin1Char('\n'));
    QCOMPARE(tailLines.size(), 100);
    QCOMPARE(tailLines.first(), QStringLiteral("line 51"));
    QCOMPARE(tailLines.last(), QStringLiteral("line 150"));
}
