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

QString readStyleSheetSource()
{
    QDir dir(QDir::current());
    QString path;
    for (int i = 0; i < 4; ++i) {
        const QString candidate = dir.filePath(QStringLiteral("src/ui/style.qss"));
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

void AppStyleTest::sidebarStyleUsesSidebarPaletteSlots()
{
    const QString source = readStyleSheetSource();
    QVERIFY2(!source.isEmpty(), "failed to read style.qss");
    QVERIFY2(source.contains(QStringLiteral("QFrame#sidebarNavPanel {\n            background-color: @C16@;")),
             "Sidebar panel must use the sidebar background slot, not the danger color slot");
    QVERIFY2(source.contains(QStringLiteral("QListWidget#sidebarNav::item {\n            background-color: transparent;\n            color: @C17@;")),
             "Sidebar items must use the sidebar text slot");
    QVERIFY2(source.contains(QStringLiteral("QListWidget#sidebarNav::item:selected {\n            background-color: @C19@;\n            color: @C20@;")),
             "Sidebar selected state must use selected background and selected text slots");
    QVERIFY2(!source.contains(QRegularExpression(QStringLiteral("%1[0-9]"))),
             "style.qss must not use two-digit QString::arg placeholders");
}
