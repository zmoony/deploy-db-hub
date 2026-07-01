#include "ui/ServiceSqlWorkbenchWidget.h"

#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QStringConverter>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace {

QStringList orderedHeaders(const QVector<QJsonObject> &rows)
{
    if (rows.isEmpty()) {
        return {};
    }
    return rows.first().keys();
}

QPushButton *makeToolbarAction(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    if (!objectName.isEmpty()) {
        button->setObjectName(objectName);
    }
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(28);
    button->setMaximumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

QString formatSqlText(const QString &sql)
{
    static const QStringList keywords = {
        QStringLiteral("SELECT"),  QStringLiteral("FROM"),    QStringLiteral("WHERE"),
        QStringLiteral("INSERT"),  QStringLiteral("INTO"),    QStringLiteral("VALUES"),
        QStringLiteral("UPDATE"),  QStringLiteral("SET"),     QStringLiteral("DELETE"),
        QStringLiteral("JOIN"),    QStringLiteral("LEFT"),    QStringLiteral("RIGHT"),
        QStringLiteral("INNER"),   QStringLiteral("OUTER"),   QStringLiteral("ON"),
        QStringLiteral("ORDER"),   QStringLiteral("BY"),      QStringLiteral("GROUP"),
        QStringLiteral("HAVING"),  QStringLiteral("LIMIT"),   QStringLiteral("OFFSET"),
        QStringLiteral("AND"),     QStringLiteral("OR"),      QStringLiteral("UNION"),
        QStringLiteral("CREATE"),  QStringLiteral("ALTER"),   QStringLiteral("DROP"),
    };

    QString formatted = sql.trimmed();
    for (const QString &keyword : keywords) {
        const QRegularExpression pattern(QStringLiteral("\\b%1\\b").arg(keyword),
                                         QRegularExpression::CaseInsensitiveOption);
        formatted.replace(pattern, keyword);
    }
    for (const QString &keyword : keywords) {
        formatted.replace(QRegularExpression(QStringLiteral("\\s+%1\\s+").arg(keyword)),
                          QStringLiteral("\n%1 ").arg(keyword));
    }
    return formatted.trimmed();
}

QString csvEscape(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    if (escaped.contains(QLatin1Char(',')) || escaped.contains(QLatin1Char('"'))
        || escaped.contains(QLatin1Char('\n'))) {
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return escaped;
}

} // namespace

ServiceSqlWorkbenchWidget::ServiceSqlWorkbenchWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(PageLayout::Space8);

    auto *sidebar = new QFrame(splitter);
    sidebar->setObjectName(QStringLiteral("sqlWorkbenchSidebar"));
    sidebar->setMinimumWidth(160);
    sidebar->setMaximumWidth(240);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(PageLayout::Space12,
                                      PageLayout::Space12,
                                      PageLayout::Space12,
                                      PageLayout::Space12);
    sidebarLayout->setSpacing(PageLayout::Space8);

    auto *sidebarTitle = PageLayout::makeSectionLabel(QStringLiteral("Tables"), sidebar);
    sidebarLayout->addWidget(sidebarTitle);

    m_tableTree = new QTreeWidget(sidebar);
    m_tableTree->setObjectName(QStringLiteral("sqlWorkbenchTree"));
    m_tableTree->setHeaderHidden(true);
    m_tableTree->setRootIsDecorated(true);
    m_tableTree->setIndentation(12);
    m_tableTree->setContextMenuPolicy(Qt::CustomContextMenu);
    sidebarLayout->addWidget(m_tableTree, 1);
    connect(m_tableTree, &QTreeWidget::itemActivated, this, &ServiceSqlWorkbenchWidget::onTableTreeActivated);
    connect(m_tableTree, &QTreeWidget::customContextMenuRequested, this, &ServiceSqlWorkbenchWidget::onTableContextMenu);

    auto *mainPanel = new QWidget(splitter);
    auto *mainLayout = new QVBoxLayout(mainPanel);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(PageLayout::Space8);

    auto *topToolbar = new QWidget(mainPanel);
    auto *topToolbarLayout = new QHBoxLayout(topToolbar);
    topToolbarLayout->setContentsMargins(0, 0, 0, 0);
    topToolbarLayout->setSpacing(PageLayout::Space6);

    m_executeButton = makeToolbarAction(QStringLiteral("执行"), QStringLiteral("primaryButton"), topToolbar);
    auto *formatButton = makeToolbarAction(QStringLiteral("格式化"), QString(), topToolbar);
    auto *clearButton = makeToolbarAction(QStringLiteral("清空"), QStringLiteral("dangerButton"), topToolbar);
    topToolbarLayout->addWidget(m_executeButton);
    topToolbarLayout->addWidget(formatButton);
    topToolbarLayout->addWidget(clearButton);
    topToolbarLayout->addStretch();

    m_connectionLabel = new QLabel(QStringLiteral("点击连接数据库"), topToolbar);
    m_connectionLabel->setObjectName(QStringLiteral("mutedText"));
    topToolbarLayout->addWidget(m_connectionLabel);
    mainLayout->addWidget(topToolbar);

    m_editor = new QPlainTextEdit(mainPanel);
    m_editor->setObjectName(QStringLiteral("sqlWorkbenchEditor"));
    m_editor->setPlaceholderText(QStringLiteral("请输入 SQL；选择模式后将在当前模式下执行"));
    m_editor->setMinimumHeight(100);
    mainLayout->addWidget(m_editor, 2);

    auto *resultToolbar = new QWidget(mainPanel);
    auto *resultToolbarLayout = new QHBoxLayout(resultToolbar);
    resultToolbarLayout->setContentsMargins(0, 0, 0, 0);
    resultToolbarLayout->setSpacing(PageLayout::Space6);

    m_resultMetaLabel = new QLabel(QStringLiteral("暂无结果"), resultToolbar);
    m_resultMetaLabel->setObjectName(QStringLiteral("mutedText"));
    resultToolbarLayout->addWidget(m_resultMetaLabel, 1);

    m_prevPageButton = makeToolbarAction(QStringLiteral("上一页"), QString(), resultToolbar);
    m_nextPageButton = makeToolbarAction(QStringLiteral("下一页"), QString(), resultToolbar);
    m_pageInfoLabel = new QLabel(QStringLiteral("第 0/0 页"), resultToolbar);
    m_pageInfoLabel->setObjectName(QStringLiteral("mutedText"));
    m_pageSizeCombo = new QComboBox(resultToolbar);
    m_pageSizeCombo->setObjectName(QStringLiteral("sqlPageSizeCombo"));
    m_pageSizeCombo->addItem(QStringLiteral("50 条/页"), 50);
    m_pageSizeCombo->addItem(QStringLiteral("100 条/页"), 100);
    m_pageSizeCombo->addItem(QStringLiteral("200 条/页"), 200);
    PageLayout::configureFormInput(m_pageSizeCombo);
    m_pageSizeCombo->setFixedWidth(100);

    auto *copyRowButton = makeToolbarAction(QStringLiteral("复制行(JSON)"), QString(), resultToolbar);
    auto *copyColumnButton = makeToolbarAction(QStringLiteral("复制列"), QString(), resultToolbar);
    auto *exportButton = makeToolbarAction(QStringLiteral("导出"), QStringLiteral("secondaryButton"), resultToolbar);
    auto *exportMenu = new QMenu(exportButton);
    exportMenu->addAction(QStringLiteral("CSV"), this, [this]() { exportRows(QStringLiteral("csv")); });
    exportMenu->addAction(QStringLiteral("JSON"), this, [this]() { exportRows(QStringLiteral("json")); });
    exportMenu->addAction(QStringLiteral("Excel"), this, [this]() { exportRows(QStringLiteral("excel")); });
    exportButton->setMenu(exportMenu);

    resultToolbarLayout->addWidget(m_prevPageButton);
    resultToolbarLayout->addWidget(m_nextPageButton);
    resultToolbarLayout->addWidget(m_pageInfoLabel);
    resultToolbarLayout->addWidget(m_pageSizeCombo);
    resultToolbarLayout->addWidget(copyRowButton);
    resultToolbarLayout->addWidget(copyColumnButton);
    resultToolbarLayout->addWidget(exportButton);
    mainLayout->addWidget(resultToolbar);

    m_resultStack = new QStackedWidget(mainPanel);
    m_resultEmpty = new QLabel(QStringLiteral("暂无数据"), m_resultStack);
    m_resultEmpty->setAlignment(Qt::AlignCenter);
    m_resultEmpty->setObjectName(QStringLiteral("mutedText"));
    m_resultTable = new QTableWidget(m_resultStack);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    PageLayout::configureListingTable(m_resultTable);
    m_resultTable->verticalHeader()->setDefaultSectionSize(32);
    m_resultStack->addWidget(m_resultEmpty);
    m_resultStack->addWidget(m_resultTable);
    mainLayout->addWidget(m_resultStack, 3);

    splitter->addWidget(sidebar);
    splitter->addWidget(mainPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({200, 780});
    root->addWidget(splitter);

    connect(m_executeButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onExecute);
    connect(formatButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onFormat);
    connect(clearButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onClear);
    connect(copyRowButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onCopyRow);
    connect(copyColumnButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onCopyColumn);
    connect(m_prevPageButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onPrevPage);
    connect(m_nextPageButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onNextPage);
    connect(m_pageSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ServiceSqlWorkbenchWidget::onPageSizeChanged);

    updateEditorHint();
    updateResultMeta();
}

void ServiceSqlWorkbenchWidget::setConnectionSummary(const QString &summary)
{
    m_connectionLabel->setText(summary.isEmpty() ? QStringLiteral("点击连接数据库") : summary);
}

void ServiceSqlWorkbenchWidget::setCurrentSchema(const QString &schema)
{
    m_currentSchema = schema.trimmed();
    updateEditorHint();
}

void ServiceSqlWorkbenchWidget::updateEditorHint()
{
    if (m_currentSchema.isEmpty()) {
        m_editor->setPlaceholderText(QStringLiteral("请输入 SQL；选择模式后将在当前模式下执行"));
        return;
    }
    m_editor->setPlaceholderText(
        QStringLiteral("当前模式：%1。未写 schema 的表名将在此模式下解析。\n请输入 SQL…").arg(m_currentSchema));
}

void ServiceSqlWorkbenchWidget::setTables(const QStringList &tables)
{
    m_tableTree->clear();
    auto *root = new QTreeWidgetItem(m_tableTree, {QStringLiteral("Tables")});
    root->setExpanded(true);
    for (const QString &table : tables) {
        new QTreeWidgetItem(root, {table});
    }
}

void ServiceSqlWorkbenchWidget::setExecuting(bool executing)
{
    m_executeButton->setEnabled(!executing);
    m_executeButton->setText(executing ? QStringLiteral("执行中…") : QStringLiteral("执行"));
}

QString ServiceSqlWorkbenchWidget::currentSql() const
{
    return m_editor->toPlainText().trimmed();
}

void ServiceSqlWorkbenchWidget::setSqlText(const QString &sql)
{
    m_editor->setPlainText(sql);
}

void ServiceSqlWorkbenchWidget::showResult(const ServiceResult &result, qint64 elapsedMs)
{
    m_lastElapsedMs = elapsedMs;
    if (!result.ok) {
        m_totalRows = 0;
        m_allRows.clear();
        m_headers.clear();
        showResultMessage(result.message);
        updateResultMeta();
        return;
    }
    if (result.rows.isEmpty()) {
        m_totalRows = 0;
        m_allRows.clear();
        m_headers.clear();
        showResultMessage(result.message.isEmpty() ? QStringLiteral("执行成功，无结果集") : result.message);
        updateResultMeta();
        return;
    }
    showResultTable(result, elapsedMs);
}

void ServiceSqlWorkbenchWidget::setPagedRows(const QVector<QJsonObject> &rows, const QStringList &headers)
{
    m_allRows = rows;
    m_headers = headers;
    m_totalRows = rows.size();
    m_pageIndex = 0;
}

void ServiceSqlWorkbenchWidget::renderResultPage()
{
    if (m_totalRows == 0 || m_headers.isEmpty()) {
        m_resultStack->setCurrentWidget(m_resultEmpty);
        m_prevPageButton->setEnabled(false);
        m_nextPageButton->setEnabled(false);
        m_pageInfoLabel->setText(QStringLiteral("第 0/0 页"));
        return;
    }

    const int totalPages = qMax(1, (m_totalRows + m_pageSize - 1) / m_pageSize);
    m_pageIndex = qBound(0, m_pageIndex, totalPages - 1);
    const int start = m_pageIndex * m_pageSize;
    const int end = qMin(start + m_pageSize, m_totalRows);

    m_resultTable->setColumnCount(m_headers.size());
    m_resultTable->setHorizontalHeaderLabels(m_headers);
    m_resultTable->setRowCount(end - start);
    for (int row = start; row < end; ++row) {
        const QJsonObject item = m_allRows.at(row);
        const int displayRow = row - start;
        for (int col = 0; col < m_headers.size(); ++col) {
            m_resultTable->setItem(displayRow,
                                 col,
                                 new QTableWidgetItem(item.value(m_headers.at(col)).toString()));
        }
    }
    PageLayout::refreshListingTableColumns(m_resultTable);
    m_resultStack->setCurrentWidget(m_resultTable);

    m_prevPageButton->setEnabled(m_pageIndex > 0);
    m_nextPageButton->setEnabled(m_pageIndex + 1 < totalPages);
    m_pageInfoLabel->setText(QStringLiteral("第 %1/%2 页").arg(m_pageIndex + 1).arg(totalPages));
}

void ServiceSqlWorkbenchWidget::updateResultMeta()
{
    QStringList parts;
    if (m_lastElapsedMs >= 0) {
        parts << QStringLiteral("耗时 %1 ms").arg(m_lastElapsedMs);
    }
    if (m_totalRows > 0) {
        parts << QStringLiteral("共 %1 条").arg(m_totalRows);
    } else if (!m_resultEmpty->text().isEmpty() && m_resultEmpty->text() != QStringLiteral("暂无数据")) {
        parts << m_resultEmpty->text();
    } else {
        parts << QStringLiteral("暂无结果");
    }
    m_resultMetaLabel->setText(parts.join(QStringLiteral(" | ")));
}

void ServiceSqlWorkbenchWidget::showResultTable(const ServiceResult &result, qint64 elapsedMs)
{
    Q_UNUSED(elapsedMs);
    const QStringList headers = orderedHeaders(result.rows);
    setPagedRows(result.rows, headers);
    renderResultPage();
    updateResultMeta();
}

void ServiceSqlWorkbenchWidget::showResultMessage(const QString &message)
{
    m_resultEmpty->setText(message.isEmpty() ? QStringLiteral("暂无数据") : message);
    m_resultStack->setCurrentWidget(m_resultEmpty);
    m_prevPageButton->setEnabled(false);
    m_nextPageButton->setEnabled(false);
    m_pageInfoLabel->setText(QStringLiteral("第 0/0 页"));
}

QTableWidget *ServiceSqlWorkbenchWidget::currentResultTable() const
{
    if (m_resultStack != nullptr && m_resultStack->currentWidget() == m_resultTable) {
        return m_resultTable;
    }
    return nullptr;
}

QJsonObject ServiceSqlWorkbenchWidget::rowAsJson(int row) const
{
    QJsonObject object;
    if (row < 0 || row >= m_resultTable->rowCount()) {
        return object;
    }
    for (int col = 0; col < m_headers.size(); ++col) {
        const QTableWidgetItem *item = m_resultTable->item(row, col);
        object.insert(m_headers.at(col), item != nullptr ? item->text() : QString());
    }
    return object;
}

void ServiceSqlWorkbenchWidget::onExecute()
{
    const QString sql = currentSql();
    if (sql.isEmpty()) {
        return;
    }
    emit executeRequested(sql);
}

void ServiceSqlWorkbenchWidget::onFormat()
{
    m_editor->setPlainText(formatSqlText(m_editor->toPlainText()));
}

void ServiceSqlWorkbenchWidget::onClear()
{
    m_editor->clear();
}

void ServiceSqlWorkbenchWidget::onCopyRow()
{
    QTableWidget *table = currentResultTable();
    if (table == nullptr) {
        return;
    }
    const int row = table->currentRow();
    if (row < 0) {
        return;
    }
    const int absoluteRow = m_pageIndex * m_pageSize + row;
    if (absoluteRow < 0 || absoluteRow >= m_allRows.size()) {
        return;
    }
    const QJsonDocument doc(m_allRows.at(absoluteRow));
    QApplication::clipboard()->setText(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void ServiceSqlWorkbenchWidget::onCopyColumn()
{
    QTableWidget *table = currentResultTable();
    if (table == nullptr) {
        return;
    }
    const int col = table->currentColumn();
    if (col < 0 || col >= m_headers.size()) {
        return;
    }
    QStringList cells;
    for (int row = 0; row < m_totalRows; ++row) {
        cells.append(m_allRows.at(row).value(m_headers.at(col)).toString());
    }
    QApplication::clipboard()->setText(cells.join(QLatin1Char('\n')));
}

void ServiceSqlWorkbenchWidget::exportRows(const QString &format) const
{
    if (m_allRows.isEmpty() || m_headers.isEmpty()) {
        QMessageBox::information(const_cast<ServiceSqlWorkbenchWidget *>(this),
                                 QStringLiteral("无法导出"),
                                 QStringLiteral("当前没有可导出的结果。"));
        return;
    }

    QString filter;
    QString defaultSuffix;
    if (format == QStringLiteral("json")) {
        filter = QStringLiteral("JSON (*.json)");
        defaultSuffix = QStringLiteral("json");
    } else if (format == QStringLiteral("excel")) {
        filter = QStringLiteral("Excel (*.xls)");
        defaultSuffix = QStringLiteral("xls");
    } else {
        filter = QStringLiteral("CSV (*.csv)");
        defaultSuffix = QStringLiteral("csv");
    }

    const QString path = QFileDialog::getSaveFileName(const_cast<ServiceSqlWorkbenchWidget *>(this),
                                                      QStringLiteral("导出查询结果"),
                                                      QString(),
                                                      filter);
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        QMessageBox::warning(const_cast<ServiceSqlWorkbenchWidget *>(this),
                             QStringLiteral("导出失败"),
                             file.errorString());
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    if (format == QStringLiteral("json")) {
        QJsonArray array;
        for (const QJsonObject &row : m_allRows) {
            array.append(row);
        }
        stream << QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Indented));
    } else if (format == QStringLiteral("excel")) {
        stream << QStringLiteral("<html><head><meta charset=\"UTF-8\"></head><body><table border=\"1\">");
        stream << QStringLiteral("<tr>");
        for (const QString &header : m_headers) {
            stream << QStringLiteral("<th>") << header.toHtmlEscaped() << QStringLiteral("</th>");
        }
        stream << QStringLiteral("</tr>");
        for (const QJsonObject &row : m_allRows) {
            stream << QStringLiteral("<tr>");
            for (const QString &header : m_headers) {
                stream << QStringLiteral("<td>") << row.value(header).toString().toHtmlEscaped()
                       << QStringLiteral("</td>");
            }
            stream << QStringLiteral("</tr>");
        }
        stream << QStringLiteral("</table></body></html>");
    } else {
        stream << m_headers.join(QLatin1Char(',')) << QLatin1Char('\n');
        for (const QJsonObject &row : m_allRows) {
            QStringList cells;
            for (const QString &header : m_headers) {
                cells.append(csvEscape(row.value(header).toString()));
            }
            stream << cells.join(QLatin1Char(',')) << QLatin1Char('\n');
        }
    }

    file.close();
    QMessageBox::information(const_cast<ServiceSqlWorkbenchWidget *>(this),
                             QStringLiteral("导出完成"),
                             QStringLiteral("已保存到：%1").arg(path));
}

void ServiceSqlWorkbenchWidget::onPrevPage()
{
    if (m_pageIndex > 0) {
        --m_pageIndex;
        renderResultPage();
    }
}

void ServiceSqlWorkbenchWidget::onNextPage()
{
    const int totalPages = qMax(1, (m_totalRows + m_pageSize - 1) / m_pageSize);
    if (m_pageIndex + 1 < totalPages) {
        ++m_pageIndex;
        renderResultPage();
    }
}

void ServiceSqlWorkbenchWidget::onPageSizeChanged(int index)
{
    m_pageSize = m_pageSizeCombo->itemData(index).toInt();
    if (m_pageSize <= 0) {
        m_pageSize = 50;
    }
    m_pageIndex = 0;
    renderResultPage();
}

void ServiceSqlWorkbenchWidget::onTableTreeActivated(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    emit tableSelected(item->text(0));
}

void ServiceSqlWorkbenchWidget::onTableContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tableTree->itemAt(pos);
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    const QString tableName = item->text(0);
    QMenu menu(this);
    menu.addAction(QStringLiteral("查看表结构"), this, [this, tableName]() {
        emit showTableStructureRequested(tableName);
    });
    menu.addAction(QStringLiteral("刷新表列表"), this, [this]() {
        emit refreshTablesRequested();
    });
    menu.addSeparator();
    menu.addAction(QStringLiteral("删除表"), this, [this, tableName]() {
        emit deleteTableRequested(tableName);
    });
    menu.exec(m_tableTree->viewport()->mapToGlobal(pos));
}
