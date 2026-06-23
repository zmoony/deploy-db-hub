#include "ui/ServiceSqlDialog.h"

#include "ui/PageLayout.h"

#include <QJsonObject>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ServiceSqlDialog::ServiceSqlDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("SQL 查询"));
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    layout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("SQL"), this));
    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("deployLog"));
    m_editor->setMinimumHeight(120);
    layout->addWidget(m_editor);

    layout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("结果"), this));
    m_table = new QTableWidget(this);
    PageLayout::configureDataTable(m_table);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setMinimumHeight(180);
    layout->addWidget(m_table, 1);

    m_message = new QPlainTextEdit(this);
    m_message->setReadOnly(true);
    m_message->setMaximumHeight(80);
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
    m_message->setPlainText(result.ok ? result.message : result.message);
    m_table->clear();
    if (!result.ok || result.rows.isEmpty()) {
        m_table->setRowCount(0);
        m_table->setColumnCount(0);
        return;
    }

    QStringList headers = result.rows.first().keys();
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); ++row) {
        const QJsonObject item = result.rows.at(row);
        for (int col = 0; col < headers.size(); ++col) {
            m_table->setItem(row, col, new QTableWidgetItem(item.value(headers.at(col)).toString()));
        }
    }
    m_table->resizeColumnsToContents();
}
