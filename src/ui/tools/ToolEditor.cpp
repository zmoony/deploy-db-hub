#include "ui/tools/ToolEditor.h"

#include <QVBoxLayout>

namespace Ui {
namespace Tools {

ToolEditor::ToolEditor(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("toolEditor"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("toolEditorText"));
    m_editor->setMinimumHeight(220);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(m_editor);

    connect(m_editor, &QPlainTextEdit::textChanged, this, &ToolEditor::textChanged);
}

void ToolEditor::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

void ToolEditor::setText(const QString &text)
{
    m_editor->setPlainText(text);
}

QString ToolEditor::text() const
{
    return m_editor->toPlainText();
}

void ToolEditor::setReadOnly(bool readOnly)
{
    m_editor->setReadOnly(readOnly);
}

bool ToolEditor::isReadOnly() const
{
    return m_editor->isReadOnly();
}

void ToolEditor::clear()
{
    m_editor->clear();
}

QPlainTextEdit *ToolEditor::editor() const
{
    return m_editor;
}

} // namespace Tools
} // namespace Ui
