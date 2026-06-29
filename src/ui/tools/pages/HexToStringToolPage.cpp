#include "ui/tools/pages/HexToStringToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

HexToStringToolPage::HexToStringToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("hexToStringToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *primaryButton = Helpers::makeToolButton(QStringLiteral("转换"), toolbar);
    toolbarLayout->addWidget(primaryButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(this);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = new ToolEditor(editors);
    input->setPlaceholderText(QStringLiteral("48656c6c6f205174"));
    auto *output = new ToolEditor(editors);
    output->setPlaceholderText(QStringLiteral("输出"));
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(primaryButton, &QPushButton::clicked, this, [input, output, message]() {
        QString error;
        const QString result = CommonTools::hexToString(input->text(), &error);
        if (!error.isEmpty()) {
            output->clear();
            message->setText(error);
            return;
        }
        output->setText(result);
        message->setText(QStringLiteral("已完成"));
    });
}

QString HexToStringToolPage::title() const
{
    return QStringLiteral("Hex 转字符串");
}

QString HexToStringToolPage::subtitle() const
{
    return QStringLiteral("将十六进制字节转为 UTF-8 字符串。");
}

} // namespace Tools
} // namespace Ui
