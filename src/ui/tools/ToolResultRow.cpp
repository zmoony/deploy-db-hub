#include "ui/tools/ToolResultRow.h"

#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
namespace Tools {

ToolResultRow::ToolResultRow(const QString &label, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("toolResultRow"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    if (!label.isEmpty()) {
        auto *labelWidget = new QLabel(label, this);
        labelWidget->setObjectName(QStringLiteral("toolResultRowLabel"));
        labelWidget->setFixedWidth(120);
        layout->addWidget(labelWidget);
    }

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("toolResultRowEdit"));
    m_edit->setReadOnly(true);
    PageLayout::configureFormInput(m_edit);
    layout->addWidget(m_edit, 1);

    auto *copyButton = new QPushButton(QStringLiteral("复制"), this);
    copyButton->setObjectName(QStringLiteral("formActionButton"));
    copyButton->setFixedHeight(28);
    connect(copyButton, &QPushButton::clicked, this, [this]() {
        if (QClipboard *clipboard = QApplication::clipboard()) {
            clipboard->setText(m_edit->text());
        }
        emit copied();
    });
    layout->addWidget(copyButton);
}

void ToolResultRow::setText(const QString &text)
{
    m_edit->setText(text);
}

QString ToolResultRow::text() const
{
    return m_edit->text();
}

void ToolResultRow::clear()
{
    m_edit->clear();
}

} // namespace Tools
} // namespace Ui
