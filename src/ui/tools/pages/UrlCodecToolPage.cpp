#include "ui/tools/pages/UrlCodecToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

UrlCodecToolPage::UrlCodecToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("urlCodecToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *encode = Helpers::makeToolButton(QStringLiteral("编码"), toolbar);
    auto *decode = Helpers::makeToolButton(QStringLiteral("解码"), toolbar);
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
    input->setPlaceholderText(QStringLiteral("原文 / 编码后文本"));
    auto *output = new ToolEditor(editors);
    output->setPlaceholderText(QStringLiteral("输出"));
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    connect(encode, &QPushButton::clicked, this, [input, output]() {
        output->setText(CommonTools::urlEncode(input->text()));
    });
    connect(decode, &QPushButton::clicked, this, [input, output]() {
        output->setText(CommonTools::urlDecode(input->text()));
    });
    connect(copy, &QPushButton::clicked, this, [output]() {
        Helpers::copyToClipboard(output->text());
    });
}

QString UrlCodecToolPage::title() const
{
    return QStringLiteral("URL 编解码");
}

QString UrlCodecToolPage::subtitle() const
{
    return QStringLiteral("URL 编码和解码");
}

} // namespace Tools
} // namespace Ui
