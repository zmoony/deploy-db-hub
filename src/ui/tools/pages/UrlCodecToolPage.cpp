#include "ui/tools/pages/UrlCodecToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *makeActionButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolBarButton"));
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

void copyTextToClipboard(const QString &text)
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }
}

} // namespace

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
    auto *encode = makeActionButton(QStringLiteral("编码"), toolbar);
    auto *decode = makeActionButton(QStringLiteral("解码"), toolbar);
    auto *copy = makeActionButton(QStringLiteral("复制结果"), toolbar);
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
        copyTextToClipboard(output->text());
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
