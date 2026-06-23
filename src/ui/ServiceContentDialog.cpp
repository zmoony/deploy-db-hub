#include "ui/ServiceContentDialog.h"

#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ServiceContentDialog::ServiceContentDialog(const QString &title, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    m_view = new QPlainTextEdit(this);
    m_view->setObjectName(QStringLiteral("deployLog"));
    m_view->setReadOnly(true);
    layout->addWidget(m_view, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("关闭"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    PageLayout::applyModalDialog(this);
    resize(860, 560);
}

void ServiceContentDialog::setContent(const QString &content)
{
    m_view->setPlainText(content);
}
