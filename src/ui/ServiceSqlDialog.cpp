#include "ui/ServiceSqlDialog.h"

#include "ui/PageLayout.h"

#include <QJsonObject>

#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

QStringList orderedHeaders(const QVector<QJsonObject> &rows, const QStringList &preferredHeaders)
{
    if (!preferredHeaders.isEmpty()) {
        return preferredHeaders;
    }
    if (rows.isEmpty()) {
        return {};
    }
    return rows.first().keys();
}

}

ServiceSqlDialog::ServiceSqlDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("SQL 查询"));
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    m_sqlLabel = PageLayout::makeSectionLabel(QStringLiteral("SQL"), this);
    layout->addWidget(m_sqlLabel);
    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("deployLog"));
    m_editor->setMinimumHeight(120);
    layout->addWidget(m_editor);

    m_resultLabel = PageLayout::makeSectionLabel(QStringLiteral("结果"), this);
    layout->addWidget(m_resultLabel);
    m_table = new QTableWidget(this);
    m_table->setMinimumHeight(220);
    layout->addWidget(m_table, 1);

    m_message = new QPlainTextEdit(this);
    m_message->setReadOnly(true);
    m_message->setMaximumHeight(80);
    m_message->setVisible(false);
    layout->addWidget(m_message);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    buttons->button(QDialogButtonBox::Close)->setText(QStringLiteral("关闭"));
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    PageLayout::applyModalDialog(this);
    resize(900, 620);
}

void ServiceSqlDialog::setSql(const QString &sql)
{
    m_editor->setPlainText(sql);
}

QString ServiceSqlDialog::sql() const
{
    return m_editor->toPlainText();
}

void ServiceSqlDialog::setResult(const ServiceResult &result)
{
    m_sqlLabel->setVisible(true);
    m_editor->setVisible(true);
    setWindowTitle(QStringLiteral("SQL 查询"));
    setTableResult(result, {});
}

void ServiceSqlDialog::setTableResult(const ServiceResult &result, const QStringList &preferredHeaders)
{
    const bool readOnlyTable = !preferredHeaders.isEmpty();
    m_sqlLabel->setVisible(!readOnlyTable);
    m_editor->setVisible(!readOnlyTable);
    if (readOnlyTable) {
        setWindowTitle(QStringLiteral("消费明细"));
    }

    if (!result.ok) {
        m_message->setPlainText(result.message);
        m_message->setVisible(true);
        m_table->setRowCount(0);
        m_table->setColumnCount(0);
        m_table->setVisible(false);
        return;
    }

    m_message->setVisible(false);

    if (result.rows.isEmpty()) {
        m_message->setPlainText(result.message.isEmpty() ? QStringLiteral("暂无数据") : result.message);
        m_message->setVisible(true);
        m_table->setRowCount(0);
        m_table->setColumnCount(0);
        m_table->setVisible(false);
        return;
    }

    const QStringList headers = orderedHeaders(result.rows, preferredHeaders);
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    PageLayout::configureListingTable(m_table);
    m_table->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); ++row) {
        const QJsonObject item = result.rows.at(row);
        for (int col = 0; col < headers.size(); ++col) {
            m_table->setItem(row, col, new QTableWidgetItem(item.value(headers.at(col)).toString()));
        }
    }
    PageLayout::refreshListingTableColumns(m_table);
    m_table->setVisible(true);
}
