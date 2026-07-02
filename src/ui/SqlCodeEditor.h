#pragma once

#include <QPlainTextEdit>
#include <QStringList>

class QCompleter;
class QPaintEvent;
class LineNumberArea;

class SqlCodeEditor final : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit SqlCodeEditor(QWidget *parent = nullptr);

    void setSchemaName(const QString &schema);
    void setTableNames(const QStringList &tables);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    QString sqlToExecute() const;
    bool hasExecutableSelection() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);

private:
    friend class LineNumberArea;

    void insertCompletion(const QString &completion);
    QString wordUnderCursor() const;
    void rebuildCompleterWords();
    void showCompleterPopup();

    LineNumberArea *m_lineNumberArea = nullptr;
    QCompleter *m_completer = nullptr;
    QString m_schemaName;
    QStringList m_tableNames;
};
