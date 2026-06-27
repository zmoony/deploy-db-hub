#include "ui/tools/ToolActionBar.h"

#include <QHBoxLayout>
#include <QPushButton>

namespace Ui {
namespace Tools {

ToolActionBar::ToolActionBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("toolActionBar"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_primaryButton = new QPushButton(QStringLiteral("执行"), this);
    m_primaryButton->setObjectName(QStringLiteral("toolPrimaryButton"));
    m_primaryButton->setFixedHeight(32);
    connect(m_primaryButton, &QPushButton::clicked, this, &ToolActionBar::primaryClicked);
    layout->addWidget(m_primaryButton);

    m_clearButton = new QPushButton(QStringLiteral("清空"), this);
    m_clearButton->setObjectName(QStringLiteral("toolSecondaryButton"));
    m_clearButton->setFixedHeight(32);
    connect(m_clearButton, &QPushButton::clicked, this, &ToolActionBar::clearClicked);
    layout->addWidget(m_clearButton);

    m_copyButton = new QPushButton(QStringLiteral("复制结果"), this);
    m_copyButton->setObjectName(QStringLiteral("toolSecondaryButton"));
    m_copyButton->setFixedHeight(32);
    m_copyButton->setEnabled(false);
    connect(m_copyButton, &QPushButton::clicked, this, &ToolActionBar::copyClicked);
    layout->addWidget(m_copyButton);

    layout->addStretch();
}

QPushButton *ToolActionBar::primaryButton() const { return m_primaryButton; }
QPushButton *ToolActionBar::clearButton() const { return m_clearButton; }
QPushButton *ToolActionBar::copyButton() const { return m_copyButton; }

void ToolActionBar::setPrimaryText(const QString &text)
{
    m_primaryButton->setText(text);
}

void ToolActionBar::setCopyEnabled(bool enabled)
{
    m_copyButton->setEnabled(enabled);
}

} // namespace Tools
} // namespace Ui
