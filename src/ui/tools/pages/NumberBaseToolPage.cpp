#include "ui/tools/pages/NumberBaseToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolResultRow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
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

} // namespace

namespace Ui {
namespace Tools {

NumberBaseToolPage::NumberBaseToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("numberBaseToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *inputRow = new QWidget(this);
    auto *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(PageLayout::Space8);
    auto *input = new QLineEdit(inputRow);
    input->setPlaceholderText(QStringLiteral("输入数字"));
    PageLayout::configureFormInput(input);
    auto *fromBase = new QComboBox(inputRow);
    fromBase->addItem(QStringLiteral("十进制"), 10);
    fromBase->addItem(QStringLiteral("二进制"), 2);
    fromBase->addItem(QStringLiteral("八进制"), 8);
    fromBase->addItem(QStringLiteral("十六进制"), 16);
    PageLayout::configureFormInput(fromBase);
    auto *convert = makeActionButton(QStringLiteral("转换"), inputRow);
    inputLayout->addWidget(input, 1);
    inputLayout->addWidget(fromBase);
    inputLayout->addWidget(convert);
    layout->addWidget(inputRow);

    auto *binary = new ToolResultRow(QStringLiteral("二进制"), this);
    auto *octal = new ToolResultRow(QStringLiteral("八进制"), this);
    auto *decimal = new ToolResultRow(QStringLiteral("十进制"), this);
    auto *hex = new ToolResultRow(QStringLiteral("十六进制"), this);

    auto *rowsLayout = new QVBoxLayout;
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(PageLayout::Space8);
    rowsLayout->addWidget(binary);
    rowsLayout->addWidget(octal);
    rowsLayout->addWidget(decimal);
    rowsLayout->addWidget(hex);
    layout->addLayout(rowsLayout);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);
    layout->addStretch();

    connect(convert, &QPushButton::clicked, this,
            [input, fromBase, binary, octal, decimal, hex, message]() {
        QString error;
        const int base = fromBase->currentData().toInt();
        const CommonTools::NumberBases bases =
            CommonTools::convertNumberBase(input->text(), base, &error);
        if (!bases.valid) {
            message->setText(error);
            return;
        }
        binary->setText(bases.binary);
        octal->setText(bases.octal);
        decimal->setText(bases.decimal);
        hex->setText(bases.hex);
        message->setText(QStringLiteral("已转换"));
    });
}

QString NumberBaseToolPage::title() const
{
    return QStringLiteral("进制转换");
}

QString NumberBaseToolPage::subtitle() const
{
    return QStringLiteral("在不同进制之间转换数字");
}

} // namespace Tools
} // namespace Ui
