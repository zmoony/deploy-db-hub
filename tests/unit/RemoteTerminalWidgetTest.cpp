#include "RemoteTerminalWidgetTest.h"

#include "ui/RemoteTerminalWidget.h"
#include "ui/TerminalStream.h"

#include <QSignalSpy>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

void RemoteTerminalWidgetTest::terminalTextEditEmitsInteractiveInputBytes()
{
    TerminalTextEdit edit;
    edit.show();
    edit.setFocus();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    QVERIFY(edit.hasFocus());
    QVERIFY(!edit.isReadOnly());
    QCOMPARE(edit.viewport()->cursor().shape(), Qt::IBeamCursor);
    QVERIFY(edit.cursorWidth() >= 6);

    QSignalSpy inputSpy(&edit, &TerminalTextEdit::inputBytes);
    QSignalSpy interruptSpy(&edit, &TerminalTextEdit::interruptRequested);

    QTest::keyClicks(&edit, QStringLiteral("ls"));
    QTest::keyClick(&edit, Qt::Key_Return);
    QTest::keyClick(&edit, Qt::Key_Backspace);
    QTest::keyClick(&edit, Qt::Key_Up);
    QTest::keyClick(&edit, Qt::Key_C, Qt::ControlModifier);

    QCOMPARE(inputSpy.size(), 5);
    QCOMPARE(inputSpy.at(0).at(0).toByteArray(), QByteArray("l"));
    QCOMPARE(inputSpy.at(1).at(0).toByteArray(), QByteArray("s"));
    QCOMPARE(inputSpy.at(2).at(0).toByteArray(), QByteArray(1, '\r'));
    QCOMPARE(inputSpy.at(3).at(0).toByteArray(), QByteArray(1, '\x7f'));
    QCOMPARE(inputSpy.at(4).at(0).toByteArray(), QByteArray("\x1b[A", 3));
    QCOMPARE(interruptSpy.size(), 1);
    QVERIFY(edit.toPlainText().isEmpty());
}

void RemoteTerminalWidgetTest::terminalStreamPreservesCrlfOutput()
{
    QTextDocument document;
    QTextCursor cursor(&document);
    TerminalStream::appendToCursor(cursor, QStringLiteral("bin\r\netc\r\nhome\r\n"));
    QCOMPARE(document.toPlainText(), QStringLiteral("bin\netc\nhome\n"));
}

void RemoteTerminalWidgetTest::terminalStreamOverwritesPromptOnBareCarriageReturn()
{
    QTextDocument document;
    QTextCursor cursor(&document);
    TerminalStream::appendToCursor(cursor, QStringLiteral("[root@host ~]# "));
    TerminalStream::appendToCursor(cursor, QStringLiteral("\r[root@host ~]# "));
    QCOMPARE(document.toPlainText(), QStringLiteral("[root@host ~]# "));
}

void RemoteTerminalWidgetTest::terminalStreamAppliesAnsiColors()
{
    QTextDocument document;
    QTextCursor cursor(&document);
    TerminalStream::appendToCursor(cursor, QStringLiteral("\x1B[31merror\x1B[0m normal"));
    QCOMPARE(document.toPlainText(), QStringLiteral("error normal"));

    const QTextCharFormat errorFormat = TerminalStream::formatAt(&document, 0);
    const QTextCharFormat normalFormat = TerminalStream::formatAt(&document, 6);
    QCOMPARE(errorFormat.foreground().color().red(), 241);
    QCOMPARE(normalFormat.foreground().color(), QColor(0xD8, 0xFF, 0xE4));
}

void RemoteTerminalWidgetTest::terminalStreamAppliesTrueColor()
{
    QTextDocument document;
    QTextCursor cursor(&document);
    TerminalStream::appendToCursor(cursor, QStringLiteral("\x1B[38;2;255;128;0morange\x1B[0m"));
    const QTextCharFormat format = TerminalStream::formatAt(&document, 0);
    QCOMPARE(format.foreground().color(), QColor(255, 128, 0));
}

void RemoteTerminalWidgetTest::terminalStreamStripsOscWindowTitle()
{
    QTextDocument document;
    QTextCursor cursor(&document);
    TerminalStream::appendToCursor(cursor,
                                 QStringLiteral("\x1B]0;root@localhost:~\x07[root@localhost ~]# "));
    QCOMPARE(document.toPlainText(), QStringLiteral("[root@localhost ~]# "));
}
