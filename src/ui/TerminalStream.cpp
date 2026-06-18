#include "ui/TerminalStream.h"

#include <QFont>
#include <QRegularExpression>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

namespace TerminalStream {
namespace {

QColor ansiStandardColor(int index, bool bright)
{
    static const QColor standard[] = {
        QColor(0x4D, 0x4D, 0x4D),
        QColor(0xF1, 0x4C, 0x4C),
        QColor(0x23, 0xD1, 0x8B),
        QColor(0xF5, 0xF5, 0x43),
        QColor(0x3B, 0x8E, 0xEA),
        QColor(0xD6, 0x70, 0xD6),
        QColor(0x29, 0xB8, 0xDB),
        QColor(0xE5, 0xE5, 0xE5),
    };
    static const QColor brightPalette[] = {
        QColor(0x66, 0x66, 0x66),
        QColor(0xFF, 0x6B, 0x6B),
        QColor(0x50, 0xFA, 0x7B),
        QColor(0xFF, 0xF5, 0x6D),
        QColor(0x69, 0xB4, 0xFF),
        QColor(0xFF, 0x79, 0xFF),
        QColor(0x6E, 0xFF, 0xFF),
        QColor(0xFF, 0xFF, 0xFF),
    };
    const int slot = qBound(0, index, 7);
    return bright ? brightPalette[slot] : standard[slot];
}

QColor ansi256Color(int index)
{
    index = qBound(0, index, 255);
    if (index < 8) {
        return ansiStandardColor(index, false);
    }
    if (index < 16) {
        return ansiStandardColor(index - 8, true);
    }
    if (index < 232) {
        const int cubeIndex = index - 16;
        const int r = (cubeIndex / 36) % 6;
        const int g = (cubeIndex / 6) % 6;
        const int b = cubeIndex % 6;
        const auto channel = [](int step) {
            return step == 0 ? 0 : 55 + step * 40;
        };
        return QColor(channel(r), channel(g), channel(b));
    }
    const int gray = 8 + (index - 232) * 10;
    return QColor(gray, gray, gray);
}

struct FormatState {
    QColor foreground;
    QColor background;
    bool bold = false;
    bool underline = false;
    bool defaultBackground = true;

    explicit FormatState(const TerminalTheme &theme)
        : foreground(theme.defaultForeground)
        , background(theme.defaultBackground)
    {
    }

    void reset(const TerminalTheme &theme)
    {
        foreground = theme.defaultForeground;
        background = theme.defaultBackground;
        bold = false;
        underline = false;
        defaultBackground = true;
    }

    QColor resolveForeground(int index) const
    {
        return ansiStandardColor(index, bold || index >= 8);
    }

    QColor resolveBackground(int index) const
    {
        return ansiStandardColor(index, index >= 8);
    }

    QTextCharFormat toCharFormat() const
    {
        QTextCharFormat format;
        format.setForeground(foreground);
        if (!defaultBackground) {
            format.setBackground(background);
        }
        if (bold) {
            format.setFontWeight(QFont::Bold);
        }
        if (underline) {
            format.setFontUnderline(true);
        }
        return format;
    }
};

void applySgrParameter(FormatState *state, const TerminalTheme &theme, int code, const QList<int> &params, int *index)
{
    switch (code) {
    case 0:
        state->reset(theme);
        break;
    case 1:
        state->bold = true;
        break;
    case 3:
        break;
    case 4:
        state->underline = true;
        break;
    case 22:
        state->bold = false;
        break;
    case 24:
        state->underline = false;
        break;
    case 39:
        state->foreground = theme.defaultForeground;
        break;
    case 49:
        state->background = theme.defaultBackground;
        state->defaultBackground = true;
        break;
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
        state->foreground = state->resolveForeground(code - 30);
        break;
    case 90:
    case 91:
    case 92:
    case 93:
    case 94:
    case 95:
    case 96:
    case 97:
        state->foreground = ansiStandardColor(code - 90, true);
        break;
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
        state->background = state->resolveBackground(code - 40);
        state->defaultBackground = false;
        break;
    case 100:
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
        state->background = ansiStandardColor(code - 100, true);
        state->defaultBackground = false;
        break;
    case 38:
        if (*index + 1 < params.size() && params.at(*index + 1) == 5 && *index + 2 < params.size()) {
            state->foreground = ansi256Color(params.at(*index + 2));
            *index += 2;
        } else if (*index + 1 < params.size() && params.at(*index + 1) == 2 && *index + 4 < params.size()) {
            state->foreground = QColor(params.at(*index + 2), params.at(*index + 3), params.at(*index + 4));
            *index += 4;
        }
        break;
    case 48:
        if (*index + 1 < params.size() && params.at(*index + 1) == 5 && *index + 2 < params.size()) {
            state->background = ansi256Color(params.at(*index + 2));
            state->defaultBackground = false;
            *index += 2;
        } else if (*index + 1 < params.size() && params.at(*index + 1) == 2 && *index + 4 < params.size()) {
            state->background = QColor(params.at(*index + 2), params.at(*index + 3), params.at(*index + 4));
            state->defaultBackground = false;
            *index += 4;
        }
        break;
    default:
        break;
    }
}

void applySgr(FormatState *state, const TerminalTheme &theme, const QString &parameterText)
{
    QStringList parts = parameterText.split(QLatin1Char(';'), Qt::KeepEmptyParts);
    QList<int> params;
    params.reserve(parts.size());
    for (const QString &part : parts) {
        bool ok = false;
        const int value = part.toInt(&ok);
        params.append(ok ? value : 0);
    }
    if (params.isEmpty()) {
        params.append(0);
    }

    for (int i = 0; i < params.size(); ++i) {
        applySgrParameter(state, theme, params.at(i), params, &i);
    }
}

bool parseCsiSequence(const QString &text, int start, int *endOut, FormatState *state, const TerminalTheme &theme)
{
    if (start + 1 >= text.size() || text.at(start) != QLatin1Char('\x1B') || text.at(start + 1) != QLatin1Char('[')) {
        return false;
    }

    int index = start + 2;
    while (index < text.size()) {
        const QChar ch = text.at(index);
        if (ch >= QLatin1Char('@') && ch <= QLatin1Char('~')) {
            if (ch == QLatin1Char('m')) {
                applySgr(state, theme, text.mid(start + 2, index - start - 2));
            }
            *endOut = index + 1;
            return true;
        }
        ++index;
    }
    return false;
}

bool parseOscSequence(const QString &text, int start, int *endOut)
{
    if (start + 1 >= text.size() || text.at(start) != QLatin1Char('\x1B') || text.at(start + 1) != QLatin1Char(']')) {
        return false;
    }

    for (int index = start + 2; index < text.size(); ++index) {
        const QChar ch = text.at(index);
        if (ch == QLatin1Char('\x07')) {
            *endOut = index + 1;
            return true;
        }
        if (ch == QLatin1Char('\x1B') && index + 1 < text.size() && text.at(index + 1) == QLatin1Char('\\')) {
            *endOut = index + 2;
            return true;
        }
    }

    *endOut = text.size();
    return true;
}

bool parseCharsetSequence(const QString &text, int start, int *endOut)
{
    if (start + 1 >= text.size() || text.at(start) != QLatin1Char('\x1B')) {
        return false;
    }
    const QChar marker = text.at(start + 1);
    if (marker != QLatin1Char('(') && marker != QLatin1Char(')') && marker != QLatin1Char('#')
        && marker != QLatin1Char('%')) {
        return false;
    }
    if (start + 2 >= text.size()) {
        *endOut = text.size();
        return true;
    }
    *endOut = start + 3;
    return true;
}

bool parseSimpleEscape(const QString &text, int start, int *endOut)
{
    if (start + 1 >= text.size() || text.at(start) != QLatin1Char('\x1B')) {
        return false;
    }
    const QChar marker = text.at(start + 1);
    if (marker == QLatin1Char('[') || marker == QLatin1Char(']')) {
        return false;
    }
    *endOut = start + 2;
    return true;
}

bool parseEscapeSequence(const QString &text, int start, int *endOut, FormatState *state, const TerminalTheme &theme)
{
    if (parseCsiSequence(text, start, endOut, state, theme)) {
        return true;
    }
    if (parseOscSequence(text, start, endOut)) {
        return true;
    }
    if (parseCharsetSequence(text, start, endOut)) {
        return true;
    }
    if (parseSimpleEscape(text, start, endOut)) {
        return true;
    }
    return false;
}

bool isRetainedControlCharacter(QChar ch)
{
    return ch == QLatin1Char('\t') || ch == QLatin1Char('\n') || ch == QLatin1Char('\r')
        || ch == QLatin1Char('\b');
}

void insertCharacter(QTextCursor &cursor, const FormatState &state, QChar ch)
{
    cursor.setCharFormat(state.toCharFormat());
    cursor.insertText(QString(ch));
}

void stripOtherEscapes(QString *text)
{
    static const QRegularExpression osc(QStringLiteral("\x1B\\][^\x07\x1B]*(?:\x07|\x1B\\\\)"));
    static const QRegularExpression charset(QStringLiteral("\x1B[()#%][0-9A-Za-z]"));
    static const QRegularExpression otherEsc(QStringLiteral("\x1B[=>NOPVWXZ^_]?"));
    static const QRegularExpression c0(QStringLiteral("[\x00-\x07\x0B\x0C\x0E-\x1F]"));

    text->remove(osc);
    text->remove(charset);
    text->remove(otherEsc);
    text->remove(c0);
}

}

QString stripAnsi(const QString &input)
{
    static const QRegularExpression csi(QStringLiteral("\x1B\\[[0-9;?]*[ -/]*[@-~]"));
    QString text = input;
    text.remove(csi);
    stripOtherEscapes(&text);
    return text;
}

void appendToCursor(QTextCursor &cursor, const QString &text, const TerminalTheme &theme)
{
    TerminalTheme resolvedTheme = theme;
    if (!resolvedTheme.defaultForeground.isValid()) {
        resolvedTheme.defaultForeground = QColor(0xD8, 0xFF, 0xE4);
    }
    if (!resolvedTheme.defaultBackground.isValid()) {
        resolvedTheme.defaultBackground = QColor(0x06, 0x12, 0x1F);
    }

    FormatState state(resolvedTheme);

    for (int i = 0; i < text.size();) {
        const QChar ch = text.at(i);
        if (ch == QLatin1Char('\x1B')) {
            int end = i;
            if (parseEscapeSequence(text, i, &end, &state, resolvedTheme)) {
                i = end;
                continue;
            }
            ++i;
            continue;
        }

        if (ch.unicode() < 0x20 && !isRetainedControlCharacter(ch)) {
            ++i;
            continue;
        }

        if (ch == QLatin1Char('\r')) {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('\n')) {
                ++i;
                continue;
            }
            cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            ++i;
            continue;
        }
        if (ch == QLatin1Char('\b')) {
            cursor.deletePreviousChar();
            ++i;
            continue;
        }

        insertCharacter(cursor, state, ch);
        ++i;
    }
}

QTextCharFormat formatAt(const QTextDocument *document, int position)
{
    if (document == nullptr || position < 0 || position >= document->characterCount() - 1) {
        return {};
    }
    QTextCursor cursor(const_cast<QTextDocument *>(document));
    cursor.setPosition(position + 1);
    return cursor.charFormat();
}

}
