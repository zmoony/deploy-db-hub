#include "ui/tools/pages/HtmlEntityToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace {

QPlainTextEdit *makeEditor(const QString &placeholder, QWidget *parent)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setPlaceholderText(placeholder);
    editor->setMinimumHeight(220);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    return editor;
}

} // namespace

namespace Ui {
namespace Tools {

HtmlEntityToolPage::HtmlEntityToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("htmlEntityToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *table = new QTableWidget(this);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QStringLiteral("实体"), QStringLiteral("字符"), QStringLiteral("说明")});
    PageLayout::configureDataTable(table);
    const auto rows = CommonTools::htmlEntityRows();
    table->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(rows.at(row).code));
        table->setItem(row, 1, new QTableWidgetItem(rows.at(row).label));
        table->setItem(row, 2, new QTableWidgetItem(rows.at(row).note));
    }
    layout->addWidget(table, 1);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *encodeButton = Helpers::makeToolButton(QStringLiteral("编码"), toolbar);
    auto *decodeButton = Helpers::makeToolButton(QStringLiteral("解码"), toolbar);
    auto *copyButton = Helpers::makeToolButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(encodeButton);
    toolbarLayout->addWidget(decodeButton);
    toolbarLayout->addWidget(copyButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(this);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(QStringLiteral("<a href=\"x\">链接 & 文本</a>"), editors);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    connect(table, &QTableWidget::cellDoubleClicked, table, [table](int row, int) {
        if (QTableWidgetItem *item = table->item(row, 0)) {
            Helpers::copyToClipboard(item->text());
        }
    });
    connect(encodeButton, &QPushButton::clicked, this, [input, output]() {
        output->setPlainText(CommonTools::htmlEncode(input->toPlainText()));
    });
    connect(decodeButton, &QPushButton::clicked, this, [input, output]() {
        output->setPlainText(CommonTools::htmlDecode(input->toPlainText()));
    });
    connect(copyButton, &QPushButton::clicked, this, [output]() {
        Helpers::copyToClipboard(output->toPlainText());
    });
}

QString HtmlEntityToolPage::title() const
{
    return QStringLiteral("HTML 实体");
}

QString HtmlEntityToolPage::subtitle() const
{
    return QStringLiteral("HTML 实体编码和解码");
}

} // namespace Tools
} // namespace Ui
