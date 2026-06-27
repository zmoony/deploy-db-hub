#include "ui/tools/pages/CaseToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolResultRow.h"

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

CaseToolPage::CaseToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("caseToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *input = new QLineEdit(this);
    input->setPlaceholderText(QStringLiteral("如 deploy hub service 或 deployHubService"));
    PageLayout::configureFormInput(input);
    layout->addWidget(input);

    auto *convert = makeActionButton(QStringLiteral("转换"), this);
    layout->addWidget(convert, 0, Qt::AlignLeft);

    auto *camel = new ToolResultRow(QStringLiteral("camelCase"), this);
    auto *pascal = new ToolResultRow(QStringLiteral("PascalCase"), this);
    auto *snake = new ToolResultRow(QStringLiteral("snake_case"), this);
    auto *kebab = new ToolResultRow(QStringLiteral("kebab-case"), this);
    auto *constantCase = new ToolResultRow(QStringLiteral("CONSTANT_CASE"), this);
    auto *title = new ToolResultRow(QStringLiteral("Title Case"), this);

    auto *rowsLayout = new QVBoxLayout;
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(PageLayout::Space8);
    rowsLayout->addWidget(camel);
    rowsLayout->addWidget(pascal);
    rowsLayout->addWidget(snake);
    rowsLayout->addWidget(kebab);
    rowsLayout->addWidget(constantCase);
    rowsLayout->addWidget(title);
    layout->addLayout(rowsLayout);
    layout->addStretch();

    auto doConvert = [input, camel, pascal, snake, kebab, constantCase, title]() {
        const CommonTools::CaseForms forms = CommonTools::convertCase(input->text());
        camel->setText(forms.camel);
        pascal->setText(forms.pascal);
        snake->setText(forms.snake);
        kebab->setText(forms.kebab);
        constantCase->setText(forms.constantCase);
        title->setText(forms.title);
    };
    connect(convert, &QPushButton::clicked, this, doConvert);
    connect(input, &QLineEdit::returnPressed, this, doConvert);
}

QString CaseToolPage::title() const
{
    return QStringLiteral("大小写转换");
}

QString CaseToolPage::subtitle() const
{
    return QStringLiteral("转换文本大小写");
}

} // namespace Tools
} // namespace Ui
