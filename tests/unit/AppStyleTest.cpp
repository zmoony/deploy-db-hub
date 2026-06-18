#include "AppStyleTest.h"

#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTest>

namespace {

QString readAppStyleSource()
{
    QDir dir(QDir::current());
    QString path;
    for (int i = 0; i < 4; ++i) {
        const QString candidate = dir.filePath(QStringLiteral("src/ui/AppStyle.cpp"));
        if (QFile::exists(candidate)) {
            path = candidate;
            break;
        }
        dir.cdUp();
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

}

void AppStyleTest::comboBoxTextIsNotPaintedTwice()
{
    const QString source = readAppStyleSource();
    QVERIFY2(!source.isEmpty(), "failed to read AppStyle.cpp");
    QVERIFY2(!source.contains(QStringLiteral("lineEdit->hide()")),
             "ComboBox lineEdit must stay visible");
    QVERIFY2(!source.contains(QStringLiteral("CE_ComboBoxLabel")),
             "ComboBox text must come from lineEdit only to avoid ghosting");
    QVERIFY2(source.contains(QStringLiteral("setReadOnly")),
             "non-manual ComboBox lineEdit should be read-only");
}
