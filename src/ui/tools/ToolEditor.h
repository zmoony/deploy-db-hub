#pragma once

#include <QPlainTextEdit>
#include <QWidget>

namespace Ui {
namespace Tools {

class ToolEditor : public QWidget {
    Q_OBJECT
public:
    explicit ToolEditor(QWidget *parent = nullptr);

    void setPlaceholderText(const QString &text);
    void setText(const QString &text);
    QString text() const;
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;
    void clear();

    QPlainTextEdit *editor() const;

signals:
    void textChanged();

private:
    QPlainTextEdit *m_editor = nullptr;
};

} // namespace Tools
} // namespace Ui
