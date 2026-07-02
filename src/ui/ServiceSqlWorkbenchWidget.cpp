#include "ui/ServiceSqlWorkbenchWidget.h"

#include "ui/PageLayout.h"
#include "ui/SqlCodeEditor.h"
#include "ui/SqlResultHeaderView.h"

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
#include <QKeySequence>
#include <QLineEdit>
#include <QEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMenu>
#include <QPushButton>
#include <QShortcut>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextStream>
#include <QStringConverter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

QStringList orderedHeaders(const QVector<QJsonObject> &rows)
{
    if (rows.isEmpty()) {
        return {};
    }
    return rows.first().keys();
}

QPushButton *makeDeployPrimaryButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("deployStartButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedHeight(32);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

QPushButton *makeToolSecondaryButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolSecondaryButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedHeight(32);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

QPushButton *makeToolBarButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolBarButton"));
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

void addHeaderButton(QWidget *headerActions, QPushButton *button)
{
    if (headerActions == nullptr || button == nullptr) {
        return;
    }
    if (auto *layout = qobject_cast<QHBoxLayout *>(headerActions->layout())) {
        layout->addWidget(button);
    }
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
    root->setSpacing(PageLayout::Space12);

    QVBoxLayout *tableBody = nullptr;
    auto *tableCard = PageLayout::makeDeploySectionCard(this, QStringLiteral("表列表"), &tableBody);
    tableCard->setMinimumWidth(200);
    tableCard->setMaximumWidth(260);
    tableCard->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_tableTree = new QTreeWidget(tableCard);
    m_tableTree->setObjectName(QStringLiteral("sqlWorkbenchTree"));
    m_tableTree->setHeaderHidden(true);
    m_tableTree->setRootIsDecorated(true);
    m_tableTree->setIndentation(12);
    m_tableTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tableBody->addWidget(m_tableTree, 1);
    connect(m_tableTree, &QTreeWidget::itemClicked, this, &ServiceSqlWorkbenchWidget::onTableTreeClicked);
    connect(m_tableTree, &QTreeWidget::itemDoubleClicked, this, &ServiceSqlWorkbenchWidget::onTableTreeDoubleClicked);
    connect(m_tableTree, &QTreeWidget::customContextMenuRequested, this, &ServiceSqlWorkbenchWidget::onTableContextMenu);
    root->addWidget(tableCard, 0);

    auto *rightColumn = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(PageLayout::Space10);

    QWidget *sqlHeaderActions = nullptr;
    QVBoxLayout *sqlBody = nullptr;
    auto *sqlCard = PageLayout::makeDeploySectionCard(rightColumn,
                                                      QStringLiteral("SQL 查询"),
                                                      &sqlBody,
                                                      &sqlHeaderActions);
    sqlCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_executeButton = makeDeployPrimaryButton(QStringLiteral("执行"), sqlHeaderActions);
    auto *formatButton = makeToolBarButton(QStringLiteral("格式化"), sqlHeaderActions);
    auto *clearButton = makeToolSecondaryButton(QStringLiteral("清空"), sqlHeaderActions);
    addHeaderButton(sqlHeaderActions, m_executeButton);
    addHeaderButton(sqlHeaderActions, formatButton);
    addHeaderButton(sqlHeaderActions, clearButton);

    m_connectionLabel = new QLabel(QStringLiteral("点击连接数据库"), sqlCard);
    m_connectionLabel->setObjectName(QStringLiteral("mutedText"));
    sqlBody->addWidget(m_connectionLabel);

    m_editor = new SqlCodeEditor(sqlCard);
    m_editor->setPlaceholderText(QStringLiteral("请输入 SQL；选择模式后将在当前模式下执行"));
    m_editor->setMinimumHeight(120);
    sqlBody->addWidget(m_editor, 1);
    rightLayout->addWidget(sqlCard, 2);

    QWidget *resultHeaderActions = nullptr;
    QVBoxLayout *resultBody = nullptr;
    auto *resultCard = PageLayout::makeDeploySectionCard(rightColumn,
                                                         QStringLiteral("查询结果"),
                                                         &resultBody,
                                                         &resultHeaderActions);
    resultCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_prevPageButton = makeToolBarButton(QStringLiteral("上一页"), resultHeaderActions);
    m_nextPageButton = makeToolBarButton(QStringLiteral("下一页"), resultHeaderActions);
    m_pageInfoLabel = new QLabel(QStringLiteral("第 0/0 页"), resultHeaderActions);
    m_pageInfoLabel->setObjectName(QStringLiteral("mutedText"));
    m_pageSizeCombo = new QComboBox(resultHeaderActions);
    m_pageSizeCombo->setObjectName(QStringLiteral("sqlPageSizeCombo"));
    m_pageSizeCombo->addItem(QStringLiteral("20 条/页"), 20);
    m_pageSizeCombo->addItem(QStringLiteral("50 条/页"), 50);
    m_pageSizeCombo->addItem(QStringLiteral("100 条/页"), 100);
    m_pageSizeCombo->addItem(QStringLiteral("200 条/页"), 200);
    PageLayout::configureFormInput(m_pageSizeCombo);
    m_pageSizeCombo->setFixedWidth(100);
    auto *copyRowButton = makeToolBarButton(QStringLiteral("复制行(JSON)"), resultHeaderActions);
    auto *copyColumnButton = makeToolBarButton(QStringLiteral("复制列"), resultHeaderActions);
    auto *exportButton = makeToolSecondaryButton(QStringLiteral("导出"), resultHeaderActions);
    auto *exportMenu = new QMenu(exportButton);
    exportMenu->addAction(QStringLiteral("CSV"), this, [this]() { exportRows(QStringLiteral("csv")); });
    exportMenu->addAction(QStringLiteral("JSON"), this, [this]() { exportRows(QStringLiteral("json")); });
    exportMenu->addAction(QStringLiteral("Excel"), this, [this]() { exportRows(QStringLiteral("excel")); });
    exportButton->setMenu(exportMenu);

    if (auto *headerLayout = qobject_cast<QHBoxLayout *>(resultHeaderActions->layout())) {
        headerLayout->addWidget(m_prevPageButton);
        headerLayout->addWidget(m_nextPageButton);
        headerLayout->addWidget(m_pageInfoLabel);
        headerLayout->addWidget(m_pageSizeCombo);
        headerLayout->addWidget(copyRowButton);
        headerLayout->addWidget(copyColumnButton);
        headerLayout->addWidget(exportButton);
    }

    m_resultMetaLabel = new QLabel(QStringLiteral("暂无结果"), resultCard);
    m_resultMetaLabel->setObjectName(QStringLiteral("mutedText"));
    resultBody->addWidget(m_resultMetaLabel);

    m_resultStack = new QStackedWidget(resultCard);
    m_resultEmpty = new QLabel(QStringLiteral("暂无数据"), m_resultStack);
    m_resultEmpty->setAlignment(Qt::AlignCenter);
    m_resultEmpty->setObjectName(QStringLiteral("mutedText"));
    m_resultTable = new QTableWidget(m_resultStack);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setAlternatingRowColors(true);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSortingEnabled(false);
    m_resultHeader = new SqlResultHeaderView(Qt::Horizontal, m_resultTable);
    m_resultTable->setHorizontalHeader(m_resultHeader);
    PageLayout::configureDataTable(m_resultTable);
    m_resultTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_resultTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_resultTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_resultTable->verticalHeader()->setDefaultSectionSize(32);
    m_resultTable->viewport()->installEventFilter(this);
    connect(m_resultHeader, &SqlResultHeaderView::sortClicked, this, &ServiceSqlWorkbenchWidget::onResultHeaderSort);
    connect(m_resultHeader, &SqlResultHeaderView::filterClicked, this, &ServiceSqlWorkbenchWidget::onResultHeaderFilter);

    m_columnFilterPopup = new SqlColumnFilterPopup(this);
    connect(m_columnFilterPopup, &SqlColumnFilterPopup::filterApplied, this, &ServiceSqlWorkbenchWidget::onColumnFilterApplied);
    connect(m_columnFilterPopup, &SqlColumnFilterPopup::filterCleared, this, &ServiceSqlWorkbenchWidget::onColumnFilterCleared);
    m_resultStack->addWidget(m_resultEmpty);
    m_resultStack->addWidget(m_resultTable);
    resultBody->addWidget(m_resultStack, 1);
    rightLayout->addWidget(resultCard, 3);

    root->addWidget(rightColumn, 1);

    connect(m_executeButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onExecute);
    connect(formatButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onFormat);
    connect(clearButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onClear);
    connect(copyRowButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onCopyRow);
    connect(copyColumnButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onCopyColumn);
    connect(m_prevPageButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onPrevPage);
    connect(m_nextPageButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onNextPage);
    connect(m_pageSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ServiceSqlWorkbenchWidget::onPageSizeChanged);
    connect(m_editor, &SqlCodeEditor::cursorPositionChanged, this, [this]() {
        updateExecuteButtonLabel();
    });

    auto *executeShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), m_editor);
    connect(executeShortcut, &QShortcut::activated, this, &ServiceSqlWorkbenchWidget::onExecute);
    m_executeButton->setToolTip(QStringLiteral("有选中文本时执行选中 SQL，否则执行全部（Ctrl+Enter）"));

    updateEditorHint();
    updateExecuteButtonLabel();
    updateResultMeta();
}

void ServiceSqlWorkbenchWidget::setConnectionSummary(const QString &summary)
{
    m_connectionLabel->setText(summary.isEmpty() ? QStringLiteral("点击连接数据库") : summary);
}

void ServiceSqlWorkbenchWidget::setCurrentSchema(const QString &schema)
{
    m_currentSchema = schema.trimmed();
    if (m_editor != nullptr) {
        m_editor->setSchemaName(m_currentSchema);
    }
    updateEditorHint();
}

void ServiceSqlWorkbenchWidget::updateEditorHint()
{
    if (m_currentSchema.isEmpty()) {
        m_editor->setPlaceholderText(
            QStringLiteral("请输入 SQL；选择模式后将在当前模式下执行\n关键字/表名输入时自动提示，Ctrl+Space 手动补全"));
        return;
    }
    m_editor->setPlaceholderText(
        QStringLiteral("当前模式：%1。未写 schema 的表名将在此模式下解析。\n关键字/表名输入时自动提示，Ctrl+Space 手动补全")
            .arg(m_currentSchema));
}

void ServiceSqlWorkbenchWidget::setTables(const QStringList &tables)
{
    if (m_editor != nullptr) {
        m_editor->setTableNames(tables);
    }
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
    if (executing) {
        m_executeButton->setText(QStringLiteral("执行中…"));
        return;
    }
    updateExecuteButtonLabel();
}

void ServiceSqlWorkbenchWidget::updateExecuteButtonLabel()
{
    if (m_executeButton == nullptr || m_editor == nullptr) {
        return;
    }
    if (!m_executeButton->isEnabled()) {
        return;
    }
    m_executeButton->setText(m_editor->hasExecutableSelection() ? QStringLiteral("执行选中")
                                                                : QStringLiteral("执行"));
}

QString ServiceSqlWorkbenchWidget::currentSql() const
{
    return m_editor != nullptr ? m_editor->sqlToExecute() : QString();
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
        m_sourceRowCount = 0;
        m_allRows.clear();
        m_viewRows.clear();
        m_headers.clear();
        m_columnFilters.clear();
        showResultMessage(result.message);
        updateResultMeta();
        return;
    }
    if (result.rows.isEmpty()) {
        m_totalRows = 0;
        m_sourceRowCount = 0;
        m_allRows.clear();
        m_viewRows.clear();
        m_headers.clear();
        m_columnFilters.clear();
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
    m_sourceRowCount = rows.size();
    m_sortColumn = -1;
    m_sortAscending = true;
    m_pageIndex = 0;
    m_columnFilters.clear();
    applySortAndFilter();
}

QVector<QPair<QString, int>> ServiceSqlWorkbenchWidget::columnValueCounts(int column) const
{
    QVector<QPair<QString, int>> counts;
    if (column < 0 || column >= m_headers.size()) {
        return counts;
    }

    QHash<QString, int> counter;
    const QString header = m_headers.at(column);
    for (const QJsonObject &row : m_allRows) {
        const QString value = row.value(header).toString();
        counter[value] = counter.value(value, 0) + 1;
    }

    counts.reserve(counter.size());
    for (auto it = counter.constBegin(); it != counter.constEnd(); ++it) {
        counts.append({it.key(), it.value()});
    }
    std::stable_sort(counts.begin(), counts.end(), [](const QPair<QString, int> &left, const QPair<QString, int> &right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return QString::compare(left.first, right.first, Qt::CaseInsensitive) < 0;
    });
    return counts;
}

void ServiceSqlWorkbenchWidget::applySortAndFilter()
{
    m_viewRows = m_allRows;

    for (auto it = m_columnFilters.constBegin(); it != m_columnFilters.constEnd(); ++it) {
        const int col = it.key();
        if (col < 0 || col >= m_headers.size()) {
            continue;
        }
        const SqlColumnFilterState &filter = it.value();
        if (!filter.isActive()) {
            continue;
        }
        const QString header = m_headers.at(col);
        QVector<QJsonObject> filtered;
        filtered.reserve(m_viewRows.size());
        for (const QJsonObject &row : m_viewRows) {
            const QString value = row.value(header).toString();
            if (!filter.searchText.isEmpty() && !value.contains(filter.searchText, Qt::CaseInsensitive)) {
                continue;
            }
            if (filter.excludedValues.contains(value)) {
                continue;
            }
            filtered.append(row);
        }
        m_viewRows = filtered;
    }

    if (m_sortColumn >= 0 && m_sortColumn < m_headers.size()) {
        const QString sortKey = m_headers.at(m_sortColumn);
        std::stable_sort(m_viewRows.begin(), m_viewRows.end(), [&](const QJsonObject &left, const QJsonObject &right) {
            const QString leftValue = left.value(sortKey).toString();
            const QString rightValue = right.value(sortKey).toString();
            bool leftOk = false;
            bool rightOk = false;
            const double leftNumber = leftValue.toDouble(&leftOk);
            const double rightNumber = rightValue.toDouble(&rightOk);
            int compared = 0;
            if (leftOk && rightOk) {
                compared = leftNumber < rightNumber ? -1 : (leftNumber > rightNumber ? 1 : 0);
            } else {
                compared = QString::compare(leftValue, rightValue, Qt::CaseInsensitive);
            }
            return m_sortAscending ? compared < 0 : compared > 0;
        });
    }

    m_totalRows = m_viewRows.size();
    if (m_totalRows > 0 && m_pageIndex * m_pageSize >= m_totalRows) {
        m_pageIndex = 0;
    }
    updateHeaderIndicators();
    renderResultPage();
    updateResultMeta();
}

void ServiceSqlWorkbenchWidget::applyResultColumnLayout()
{
    if (m_resultTable == nullptr || m_resultHeader == nullptr || m_headers.isEmpty()) {
        return;
    }

    const int colCount = m_headers.size();
    const int viewportWidth = qMax(0, m_resultTable->viewport()->width());
    const int minColumnWidth = qMax(96, m_resultHeader->actionIconsWidth() + 48);
    const bool shouldStretch = static_cast<qint64>(colCount) * minColumnWidth <= viewportWidth;
    m_resultColumnsStretch = shouldStretch;

    if (shouldStretch) {
        for (int col = 0; col < colCount; ++col) {
            m_resultHeader->setSectionResizeMode(col, QHeaderView::Stretch);
        }
        return;
    }

    for (int col = 0; col < colCount; ++col) {
        m_resultHeader->setSectionResizeMode(col, QHeaderView::Interactive);
        if (m_resultHeader->sectionSize(col) < minColumnWidth) {
            m_resultHeader->resizeSection(col, minColumnWidth);
        }
    }
}

bool ServiceSqlWorkbenchWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_resultTable->viewport() && event->type() == QEvent::Resize && !m_headers.isEmpty()) {
        const int colCount = m_headers.size();
        const int viewportWidth = qMax(0, m_resultTable->viewport()->width());
        const int minColumnWidth = qMax(96, m_resultHeader->actionIconsWidth() + 48);
        const bool shouldStretch = static_cast<qint64>(colCount) * minColumnWidth <= viewportWidth;
        if (shouldStretch != m_resultColumnsStretch) {
            applyResultColumnLayout();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ServiceSqlWorkbenchWidget::updateHeaderIndicators()
{
    if (m_resultHeader == nullptr || m_headers.isEmpty()) {
        return;
    }

    m_resultTable->setHorizontalHeaderLabels(m_headers);
    m_resultHeader->setHeaderLabels(m_headers);
    for (int col = 0; col < m_headers.size(); ++col) {
        const bool active = m_columnFilters.contains(col) && m_columnFilters.value(col).isActive();
        m_resultHeader->setFilterActive(col, active);
    }
    m_resultHeader->setSortIndicator(m_sortColumn, m_sortAscending);
}

void ServiceSqlWorkbenchWidget::onResultHeaderSort(int section)
{
    if (section < 0 || section >= m_headers.size()) {
        return;
    }
    if (m_sortColumn == section) {
        m_sortAscending = !m_sortAscending;
    } else {
        m_sortColumn = section;
        m_sortAscending = true;
    }
    m_pageIndex = 0;
    applySortAndFilter();
}

void ServiceSqlWorkbenchWidget::onResultHeaderFilter(int section, const QPoint &globalPos)
{
    showColumnFilterPopup(section, globalPos);
}

void ServiceSqlWorkbenchWidget::showColumnFilterPopup(int column, const QPoint &globalPos)
{
    if (column < 0 || column >= m_headers.size() || m_columnFilterPopup == nullptr) {
        return;
    }

    const SqlColumnFilterState state = m_columnFilters.value(column);
    m_columnFilterPopup->openForColumn(column,
                                       m_headers.at(column),
                                       columnValueCounts(column),
                                       state,
                                       globalPos);
}

void ServiceSqlWorkbenchWidget::onColumnFilterApplied(int column, const SqlColumnFilterState &state)
{
    if (state.isActive()) {
        m_columnFilters.insert(column, state);
    } else {
        m_columnFilters.remove(column);
    }
    m_pageIndex = 0;
    applySortAndFilter();
}

void ServiceSqlWorkbenchWidget::onColumnFilterCleared(int column)
{
    m_columnFilters.remove(column);
    m_pageIndex = 0;
    applySortAndFilter();
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
    m_resultHeader->setHeaderLabels(m_headers);
    m_resultTable->setRowCount(end - start);
    for (int row = start; row < end; ++row) {
        const QJsonObject item = m_viewRows.at(row);
        const int displayRow = row - start;
        for (int col = 0; col < m_headers.size(); ++col) {
            m_resultTable->setItem(displayRow,
                                 col,
                                 new QTableWidgetItem(item.value(m_headers.at(col)).toString()));
        }
    }
    applyResultColumnLayout();
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
    if (m_sourceRowCount > 0) {
        parts << QStringLiteral("共 %1 条").arg(m_sourceRowCount);
        if (m_totalRows != m_sourceRowCount) {
            parts << QStringLiteral("筛选后 %1 条").arg(m_totalRows);
        }
    } else if (m_totalRows > 0) {
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
}

void ServiceSqlWorkbenchWidget::showResultMessage(const QString &message)
{
    m_viewRows.clear();
    m_columnFilters.clear();
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
    if (absoluteRow < 0 || absoluteRow >= m_viewRows.size()) {
        return;
    }
    const QJsonDocument doc(m_viewRows.at(absoluteRow));
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
    for (int row = 0; row < m_viewRows.size(); ++row) {
        cells.append(m_viewRows.at(row).value(m_headers.at(col)).toString());
    }
    QApplication::clipboard()->setText(cells.join(QLatin1Char('\n')));
}

void ServiceSqlWorkbenchWidget::exportRows(const QString &format) const
{
    if (m_viewRows.isEmpty() || m_headers.isEmpty()) {
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
        for (const QJsonObject &row : m_viewRows) {
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
        for (const QJsonObject &row : m_viewRows) {
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
        for (const QJsonObject &row : m_viewRows) {
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
        m_pageSize = 20;
    }
    m_pageIndex = 0;
    renderResultPage();
}

void ServiceSqlWorkbenchWidget::onTableTreeClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    emit tableSelected(item->text(0));
}

void ServiceSqlWorkbenchWidget::onTableTreeDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    emit tableQueryRequested(item->text(0));
}

void ServiceSqlWorkbenchWidget::onTableContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_tableTree->itemAt(pos);
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    const QString tableName = item->text(0);
    QMenu menu(this);
    menu.addAction(QStringLiteral("查询数据"), this, [this, tableName]() {
        emit tableQueryRequested(tableName);
    });
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
