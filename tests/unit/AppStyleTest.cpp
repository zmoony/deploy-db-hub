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

    const QRegularExpression comboBranch(
        QStringLiteral("if \\(control == QStyle::CC_ComboBox\\) \\{(?<body>.*?)\\n\\s*return;"),
        QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = comboBranch.match(source);
    QVERIFY2(match.hasMatch(), "failed to locate CC_ComboBox draw branch");
    QVERIFY2(!match.captured(QStringLiteral("body")).contains(QStringLiteral("CE_ComboBoxLabel")),
             "CC_ComboBox branch must not draw CE_ComboBoxLabel; Qt already paints the current text");
}
