#include "AiAssistHelperTest.h"

#include "ui/AiAssistHelper.h"

#include <QTest>

void AiAssistHelperTest::truncatesLongLogs()
{
    const QString longLog = QStringLiteral("x").repeated(30000);
    const QString prepared = AiAssistHelper::prepareLogForAiAnalysis(longLog);
    QVERIFY(prepared.contains(QStringLiteral("仅保留末尾")));
    QVERIFY(prepared.size() < longLog.size());
}

void AiAssistHelperTest::sanitizesSensitiveValues()
{
    const QString raw = QStringLiteral("password=secret123\nAuthorization: Bearer abc.def");
    const QString prepared = AiAssistHelper::prepareLogForAiAnalysis(raw);
    QVERIFY(!prepared.contains(QStringLiteral("secret123")));
    QVERIFY(!prepared.contains(QStringLiteral("abc.def")));
}
