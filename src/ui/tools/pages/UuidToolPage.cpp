#include "ui/tools/pages/UuidToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

UuidToolPage::UuidToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("uuidToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *options = new QWidget(this);
    auto *optionsLayout = new QHBoxLayout(options);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(PageLayout::Space8);
    optionsLayout->addWidget(new QLabel(QStringLiteral("数量"), options));
    auto *count = new QSpinBox(options);
    count->setRange(1, 1000);
    count->setValue(5);
    PageLayout::configureFormInput(count);
    auto *uppercase = new QCheckBox(QStringLiteral("大写"), options);
    auto *noDash = new QCheckBox(QStringLiteral("去横线"), options);
    auto *generate = Helpers::makeToolButton(QStringLiteral("生成"), options);
    auto *copy = Helpers::makeToolButton(QStringLiteral("复制全部"), options);
    optionsLayout->addWidget(count);
    optionsLayout->addWidget(uppercase);
    optionsLayout->addWidget(noDash);
    optionsLayout->addWidget(generate);
    optionsLayout->addWidget(copy);
    optionsLayout->addStretch();
    layout->addWidget(options);

    auto *output = new ToolEditor(this);
    output->setPlaceholderText(QStringLiteral("输出"));
    output->setReadOnly(true);
    layout->addWidget(output, 1);

    connect(generate, &QPushButton::clicked, this, [count, uppercase, noDash, output]() {
        const QStringList ids = CommonTools::generateUuids(
            count->value(), uppercase->isChecked(), noDash->isChecked());
        output->setText(ids.join(QLatin1Char('\n')));
    });
    connect(copy, &QPushButton::clicked, this, [output]() {
        Helpers::copyToClipboard(output->text());
    });
}

QString UuidToolPage::title() const
{
    return QStringLiteral("UUID 生成");
}

QString UuidToolPage::subtitle() const
{
    return QStringLiteral("生成随机 UUID");
}

} // namespace Tools
} // namespace Ui
