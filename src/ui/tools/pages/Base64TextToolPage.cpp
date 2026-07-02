#include "ui/tools/pages/Base64TextToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

Base64TextToolPage::Base64TextToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("base64TextToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *encode = Helpers::makeToolButton(QStringLiteral("文本 → Base64"), toolbar);
    auto *decode = Helpers::makeToolButton(QStringLiteral("Base64 → 文本"), toolbar);
    auto *copy = Helpers::makeToolButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(encode);
    toolbarLayout->addWidget(decode);
    toolbarLayout->addWidget(copy);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(this);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = new ToolEditor(editors);
    input->setPlaceholderText(QStringLiteral("原文 / Base64"));
    input->editor()->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *output = new ToolEditor(editors);
    output->setPlaceholderText(QStringLiteral("输出"));
    output->setReadOnly(true);
    output->editor()->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(message);

    connect(encode, &QPushButton::clicked, this, [input, output, message]() {
        output->setText(CommonTools::textToBase64(input->text()));
        message->setText(QStringLiteral("已编码"));
    });
    connect(decode, &QPushButton::clicked, this, [input, output, message]() {
        QString error;
        const QString text = CommonTools::base64ToText(input->text(), &error);
        if (!error.isEmpty()) {
            message->setText(error);
            return;
        }
        output->setText(text);
        message->setText(QStringLiteral("已解码"));
    });
    connect(copy, &QPushButton::clicked, this, [output]() {
        Helpers::copyToClipboard(output->text());
    });
}

QString Base64TextToolPage::title() const
{
    return QStringLiteral("Base64 编解码");
}

QString Base64TextToolPage::subtitle() const
{
    return QStringLiteral("Base64 文本编码和解码");
}

} // namespace Tools
} // namespace Ui
