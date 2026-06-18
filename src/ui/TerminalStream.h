#pragma once

#include <QColor>
#include <QString>

class QTextCharFormat;
class QTextCursor;
class QTextDocument;

namespace TerminalStream {

struct TerminalTheme {
    QColor defaultForeground{0xD8, 0xFF, 0xE4};
    QColor defaultBackground{0x06, 0x12, 0x1F};
};

QString stripAnsi(const QString &input);
void appendToCursor(QTextCursor &cursor, const QString &text, const TerminalTheme &theme = {});

QTextCharFormat formatAt(const QTextDocument *document, int position);

}
