#include "ui/SqlCodeEditor.h"

#include <QAbstractItemView>
#include <QColor>
#include <QCompleter>
#include <QFocusEvent>
#include <QFrame>
#include <QFont>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollBar>
#include <QSet>
#include <QStringListModel>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextFormat>

namespace {

QStringList defaultSqlKeywords()
{
    return {
        QStringLiteral("SELECT"),   QStringLiteral("FROM"),     QStringLiteral("WHERE"),
        QStringLiteral("INSERT"),   QStringLiteral("INTO"),     QStringLiteral("VALUES"),
        QStringLiteral("UPDATE"),   QStringLiteral("SET"),      QStringLiteral("DELETE"),
        QStringLiteral("JOIN"),     QStringLiteral("LEFT"),     QStringLiteral("RIGHT"),
        QStringLiteral("INNER"),    QStringLiteral("OUTER"),    QStringLiteral("FULL"),
        QStringLiteral("CROSS"),    QStringLiteral("ON"),       QStringLiteral("AS"),
        QStringLiteral("AND"),      QStringLiteral("OR"),       QStringLiteral("NOT"),
        QStringLiteral("IN"),       QStringLiteral("EXISTS"),   QStringLiteral("BETWEEN"),
        QStringLiteral("LIKE"),     QStringLiteral("IS"),       QStringLiteral("NULL"),
        QStringLiteral("ORDER"),    QStringLiteral("BY"),       QStringLiteral("GROUP"),
        QStringLiteral("HAVING"),   QStringLiteral("LIMIT"),    QStringLiteral("OFFSET"),
        QStringLiteral("UNION"),    QStringLiteral("ALL"),      QStringLiteral("DISTINCT"),
        QStringLiteral("CREATE"),   QStringLiteral("ALTER"),    QStringLiteral("DROP"),
        QStringLiteral("TABLE"),    QStringLiteral("VIEW"),     QStringLiteral("INDEX"),
        QStringLiteral("SCHEMA"),   QStringLiteral("DATABASE"), QStringLiteral("PRIMARY"),
        QStringLiteral("KEY"),      QStringLiteral("FOREIGN"),  QStringLiteral("REFERENCES"),
        QStringLiteral("CONSTRAINT"), QStringLiteral("DEFAULT"), QStringLiteral("CHECK"),
        QStringLiteral("UNIQUE"),   QStringLiteral("CASE"),     QStringLiteral("WHEN"),
        QStringLiteral("THEN"),     QStringLiteral("ELSE"),     QStringLiteral("END"),
        QStringLiteral("COUNT"),    QStringLiteral("SUM"),      QStringLiteral("AVG"),
        QStringLiteral("MIN"),      QStringLiteral("MAX"),      QStringLiteral("CAST"),
        QStringLiteral("COALESCE"), QStringLiteral("WITH"),     QStringLiteral("RECURSIVE"),
        QStringLiteral("EXPLAIN"),  QStringLiteral("ANALYZE"),  QStringLiteral("VACUUM"),
        QStringLiteral("TRUNCATE"), QStringLiteral("RETURNING"), QStringLiteral("ASC"),
        QStringLiteral("DESC"),     QStringLiteral("TRUE"),     QStringLiteral("FALSE"),
    };
}

class SqlSyntaxHighlighter final : public QSyntaxHighlighter
{
public:
    explicit SqlSyntaxHighlighter(QTextDocument *document)
        : QSyntaxHighlighter(document)
    {
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(QColor(QStringLiteral("#2563EB")));
        keywordFormat.setFontWeight(QFont::DemiBold);

        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(QStringLiteral("#059669")));

        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(QStringLiteral("#D97706")));

        QTextCharFormat commentFormat;
        commentFormat.setForeground(QColor(QStringLiteral("#94A3B8")));
        commentFormat.setFontItalic(true);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(QColor(QStringLiteral("#7C3AED")));
        functionFormat.setFontWeight(QFont::DemiBold);

        const QString keywordPattern =
            QStringLiteral("\\b(?:%1)\\b")
                .arg(defaultSqlKeywords().join(QStringLiteral("|")));

        m_rules.append({QRegularExpression(keywordPattern, QRegularExpression::CaseInsensitiveOption),
                        keywordFormat});
        m_rules.append({QRegularExpression(QStringLiteral("'(?:''|[^'])*'")), stringFormat});
        m_rules.append({QRegularExpression(QStringLiteral("\"(?:\"\"|[^\"])*\"")), stringFormat});
        m_rules.append({QRegularExpression(QStringLiteral("\\b-?(?:0|[1-9]\\d*)(?:\\.\\d+)?\\b")), numberFormat});
        m_rules.append({QRegularExpression(QStringLiteral("--[^\n]*")), commentFormat});
        m_rules.append({QRegularExpression(QStringLiteral("/\\*.*\\*/"), QRegularExpression::DotMatchesEverythingOption),
                        commentFormat});
        m_rules.append({QRegularExpression(QStringLiteral("\\b[A-Z_][A-Z0-9_]*\\s*(?=\\()"),
                                           QRegularExpression::CaseInsensitiveOption),
                        functionFormat});
    }

protected:
    void highlightBlock(const QString &text) override
    {
        for (const Rule &rule : m_rules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                const QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;
};

} // namespace

class LineNumberArea final : public QWidget
{
public:
    explicit LineNumberArea(SqlCodeEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return {m_editor->lineNumberAreaWidth(), 0};
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    SqlCodeEditor *m_editor = nullptr;
};

SqlCodeEditor::SqlCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    setObjectName(QStringLiteral("sqlCodeEditor"));

    QPalette pal = palette();
    pal.setColor(QPalette::Highlight, QColor(QStringLiteral("#2563EB")));
    pal.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
    setPalette(pal);

    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &SqlCodeEditor::blockCountChanged, this, &SqlCodeEditor::updateLineNumberAreaWidth);
    connect(this, &SqlCodeEditor::updateRequest, this, &SqlCodeEditor::updateLineNumberArea);
    connect(this, &SqlCodeEditor::cursorPositionChanged, this, &SqlCodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    new SqlSyntaxHighlighter(document());

    m_completer = new QCompleter(this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setWrapAround(false);
    m_completer->setMaxVisibleItems(12);
    m_completer->setWidget(this);
    if (QAbstractItemView *popup = m_completer->popup()) {
        popup->setObjectName(QStringLiteral("sqlCompleterPopup"));
        popup->setAttribute(Qt::WA_StyledBackground, true);
        popup->setFrameShape(QFrame::NoFrame);
        popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    connect(m_completer,
            QOverload<const QString &>::of(&QCompleter::activated),
            this,
            &SqlCodeEditor::insertCompletion);

    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    rebuildCompleterWords();
}

void SqlCodeEditor::setSchemaName(const QString &schema)
{
    m_schemaName = schema.trimmed();
    rebuildCompleterWords();
}

void SqlCodeEditor::setTableNames(const QStringList &tables)
{
    m_tableNames = tables;
    rebuildCompleterWords();
}

void SqlCodeEditor::rebuildCompleterWords()
{
    QSet<QString> words;
    for (const QString &keyword : defaultSqlKeywords()) {
        words.insert(keyword);
    }
    if (!m_schemaName.isEmpty()) {
        words.insert(m_schemaName);
    }
    for (const QString &table : m_tableNames) {
        const QString trimmed = table.trimmed();
        if (!trimmed.isEmpty()) {
            words.insert(trimmed);
        }
    }

    QStringList sorted = words.values();
    sorted.sort(Qt::CaseInsensitive);
    m_completer->setModel(new QStringListModel(sorted, m_completer));
}

int SqlCodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    const int space = 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void SqlCodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void SqlCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void SqlCodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void SqlCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(QStringLiteral("#F8FAFC")));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingRect(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(QStringLiteral("#94A3B8")));
            painter.drawText(0,
                             top,
                             m_lineNumberArea->width() - 4,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void SqlCodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor(QStringLiteral("#F8FAFC")));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

QString SqlCodeEditor::wordUnderCursor() const
{
    QTextCursor cursor = textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}

void SqlCodeEditor::insertCompletion(const QString &completion)
{
    if (m_completer == nullptr || m_completer->widget() != this) {
        return;
    }

    QTextCursor cursor = textCursor();
    const int extra = completion.length() - m_completer->completionPrefix().length();
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, m_completer->completionPrefix().length());
    cursor.insertText(completion);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, extra);
    setTextCursor(cursor);
}

void SqlCodeEditor::showCompleterPopup()
{
    if (m_completer == nullptr) {
        return;
    }

    const QString prefix = wordUnderCursor();
    if (prefix.isEmpty()) {
        m_completer->popup()->hide();
        return;
    }

    m_completer->setCompletionPrefix(prefix);
    if (m_completer->completionCount() <= 0) {
        m_completer->popup()->hide();
        return;
    }

    QAbstractItemView *popup = m_completer->popup();
    const int popupWidth =
        qMax(240, popup->sizeHintForColumn(0) + popup->verticalScrollBar()->sizeHint().width() + 8);
    const int rowHeight = qMax(28, popup->sizeHintForRow(0));
    const int visibleRows = qMin(m_completer->maxVisibleItems(), m_completer->completionCount());
    const int popupHeight = rowHeight * visibleRows + 8;

    const QRect cr = cursorRect();
    QPoint globalPos = viewport()->mapToGlobal(QPoint(cr.left(), cr.bottom() + 2));

    if (QScreen *screen = QGuiApplication::screenAt(globalPos)) {
        const QRect avail = screen->availableGeometry();
        if (globalPos.x() + popupWidth > avail.right()) {
            globalPos.setX(qMax(avail.left(), avail.right() - popupWidth + 1));
        }
        if (globalPos.y() + popupHeight > avail.bottom()) {
            globalPos = viewport()->mapToGlobal(QPoint(cr.left(), cr.top() - 2));
            globalPos.setY(globalPos.y() - popupHeight);
        }
        globalPos.setX(qBound(avail.left(), globalPos.x(), qMax(avail.left(), avail.right() - popupWidth + 1)));
        globalPos.setY(qBound(avail.top(), globalPos.y(), qMax(avail.top(), avail.bottom() - popupHeight + 1)));
    }

    popup->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    popup->move(globalPos);
    popup->resize(popupWidth, popupHeight);
    popup->show();
    popup->raise();
}

QString SqlCodeEditor::sqlToExecute() const
{
    const QTextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        return cursor.selectedText().replace(QChar(0x2029), QLatin1Char('\n')).trimmed();
    }
    return toPlainText().trimmed();
}

bool SqlCodeEditor::hasExecutableSelection() const
{
    return textCursor().hasSelection() && !sqlToExecute().isEmpty();
}

void SqlCodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (m_completer != nullptr && m_completer->popup()->isVisible()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            event->ignore();
            return;
        default:
            break;
        }
    }

    const bool isCompleterShortcut =
        (event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Space;
    if (!isCompleterShortcut) {
        QPlainTextEdit::keyPressEvent(event);
    } else {
        event->accept();
    }

    if (m_completer == nullptr) {
        return;
    }

    const bool ctrlOrShift = event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (ctrlOrShift && !isCompleterShortcut) {
        return;
    }

    if (isCompleterShortcut || (!event->text().isEmpty() && event->text().at(0).isLetterOrNumber())) {
        showCompleterPopup();
    }
}

void SqlCodeEditor::focusInEvent(QFocusEvent *event)
{
    if (m_completer != nullptr) {
        m_completer->setWidget(this);
    }
    QPlainTextEdit::focusInEvent(event);
}
