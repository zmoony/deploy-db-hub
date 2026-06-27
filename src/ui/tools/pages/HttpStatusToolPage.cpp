#include "ui/tools/pages/HttpStatusToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

HttpStatusToolPage::HttpStatusToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("httpStatusToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *search = new QLineEdit(this);
    search->setPlaceholderText(QStringLiteral("如 404 或 Not Found"));
    search->setClearButtonEnabled(true);
    PageLayout::configureFormInput(search);
    layout->addWidget(search);

    auto *table = new QTableWidget(this);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QStringLiteral("编码"), QStringLiteral("含义"), QStringLiteral("说明")});
    PageLayout::configureDataTable(table);
    const auto rows = CommonTools::httpStatusRows();
    table->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(rows.at(row).code));
        table->setItem(row, 1, new QTableWidgetItem(rows.at(row).label));
        table->setItem(row, 2, new QTableWidgetItem(rows.at(row).note));
    }
    layout->addWidget(table, 1);

    connect(search, &QLineEdit::textChanged, table, [table](const QString &keyword) {
        const QString needle = keyword.trimmed();
        for (int row = 0; row < table->rowCount(); ++row) {
            bool match = needle.isEmpty();
            for (int col = 0; col < table->columnCount() && !match; ++col) {
                if (QTableWidgetItem *item = table->item(row, col)) {
                    match = item->text().contains(needle, Qt::CaseInsensitive);
                }
            }
            table->setRowHidden(row, !match);
        }
    });
    connect(table, &QTableWidget::cellDoubleClicked, table, [table](int row, int) {
        const QTableWidgetItem *code = table->item(row, 0);
        const QTableWidgetItem *label = table->item(row, 1);
        if (code != nullptr && label != nullptr) {
            Helpers::copyToClipboard(QStringLiteral("%1 %2").arg(code->text(), label->text()));
        }
    });
}

QString HttpStatusToolPage::title() const
{
    return QStringLiteral("HTTP 状态码");
}

QString HttpStatusToolPage::subtitle() const
{
    return QStringLiteral("HTTP 状态码速查");
}

} // namespace Tools
} // namespace Ui
