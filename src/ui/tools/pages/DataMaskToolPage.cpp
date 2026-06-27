#include "ui/tools/pages/DataMaskToolPage.h"

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

DataMaskToolPage::DataMaskToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("dataMaskToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *primaryButton = Helpers::makeToolButton(QStringLiteral("脱敏"), toolbar);
    toolbarLayout->addWidget(primaryButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(this);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = new ToolEditor(editors);
    input->setPlaceholderText(QStringLiteral("phone=13812345678 email=a@example.com"));
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
        const QString result = CommonTools::maskSensitiveText(input->text());
        output->setText(result);
        message->setText(QStringLiteral("已完成"));
    });
}

QString DataMaskToolPage::title() const
{
    return QStringLiteral("数据采样脱敏");
}

QString DataMaskToolPage::subtitle() const
{
    return QStringLiteral("从样本数据中抽样并按规则脱敏导出。");
}

} // namespace Tools
} // namespace Ui
