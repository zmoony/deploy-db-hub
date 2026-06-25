#include "ui/ServiceSqlWorkbenchWidget.h"

#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTableWidget>
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

QTableWidget *makeResultTable(QWidget *parent)
{
    auto *table = new QTableWidget(parent);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    PageLayout::configureListingTable(table);
    table->verticalHeader()->setDefaultSectionSize(36);
    return table;
}

QPushButton *makeToolbarAction(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(32);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

void fillResultTable(QTableWidget *table, const ServiceResult &result)
{
    const QStringList headers = orderedHeaders(result.rows);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); ++row) {
        const QJsonObject item = result.rows.at(row);
        for (int col = 0; col < headers.size(); ++col) {
            table->setItem(row, col, new QTableWidgetItem(item.value(headers.at(col)).toString()));
        }
    }
    PageLayout::refreshListingTableColumns(table);
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

}

ServiceSqlWorkbenchWidget::ServiceSqlWorkbenchWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(PageLayout::Space16);

    auto *sidebar = new QFrame(splitter);
    sidebar->setObjectName(QStringLiteral("sqlWorkbenchSidebar"));
    sidebar->setMinimumWidth(180);
    sidebar->setMaximumWidth(280);
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(PageLayout::Space16,
                                      PageLayout::Space16,
                                      PageLayout::Space16,
                                      PageLayout::Space16);
    sidebarLayout->setSpacing(PageLayout::Space16);

    auto *sidebarTitle = PageLayout::makeSectionLabel(QStringLiteral("Tables"), sidebar);
    sidebarLayout->addWidget(sidebarTitle);

    m_tableTree = new QTreeWidget(sidebar);
    m_tableTree->setHeaderHidden(true);
    m_tableTree->setRootIsDecorated(true);
    m_tableTree->setIndentation(16);
    sidebarLayout->addWidget(m_tableTree, 1);
    connect(m_tableTree, &QTreeWidget::itemActivated, this, &ServiceSqlWorkbenchWidget::onTableTreeActivated);

    auto *mainPanel = new QWidget(splitter);
    auto *mainLayout = new QVBoxLayout(mainPanel);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(PageLayout::Space16);

    auto *topToolbar = new QWidget(mainPanel);
    auto *topToolbarLayout = new QHBoxLayout(topToolbar);
    topToolbarLayout->setContentsMargins(0, 0, 0, 0);
    topToolbarLayout->setSpacing(PageLayout::Space16);

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
    m_editor->setPlaceholderText(QStringLiteral("请输入SQL"));
    m_editor->setMinimumHeight(160);
    mainLayout->addWidget(m_editor, 2);

    auto *resultToolbar = new QWidget(mainPanel);
    auto *resultToolbarLayout = new QHBoxLayout(resultToolbar);
    resultToolbarLayout->setContentsMargins(0, 0, 0, 0);
    resultToolbarLayout->setSpacing(PageLayout::Space16);

    auto *allRowsButton = new QPushButton(QStringLiteral("所有行"), resultToolbar);
    allRowsButton->setEnabled(false);
    allRowsButton->setMinimumHeight(32);
    auto *copyRowButton = new QPushButton(QStringLiteral("复制行"), resultToolbar);
    copyRowButton->setMinimumHeight(32);
    auto *copyColumnButton = new QPushButton(QStringLiteral("复制列"), resultToolbar);
    copyColumnButton->setMinimumHeight(32);
    resultToolbarLayout->addWidget(allRowsButton);
    resultToolbarLayout->addWidget(copyRowButton);
    resultToolbarLayout->addWidget(copyColumnButton);
    resultToolbarLayout->addStretch();
    mainLayout->addWidget(resultToolbar);

    m_resultTabs = new QTabWidget(mainPanel);
    m_resultTabs->setDocumentMode(true);
    m_resultStack = new QStackedWidget(m_resultTabs);
    m_resultEmpty = new QLabel(QStringLiteral("暂无数据"), m_resultStack);
    m_resultEmpty->setAlignment(Qt::AlignCenter);
    m_resultEmpty->setObjectName(QStringLiteral("mutedText"));
    m_resultStack->addWidget(m_resultEmpty);
    m_resultTabs->addTab(m_resultStack, QStringLiteral("结果0"));
    mainLayout->addWidget(m_resultTabs, 3);

    splitter->addWidget(sidebar);
    splitter->addWidget(mainPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 760});
    root->addWidget(splitter);

    connect(m_executeButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onExecute);
    connect(formatButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onFormat);
    connect(clearButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onClear);
    connect(copyRowButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onCopyRow);
    connect(copyColumnButton, &QPushButton::clicked, this, &ServiceSqlWorkbenchWidget::onCopyColumn);
}

void ServiceSqlWorkbenchWidget::setConnectionSummary(const QString &summary)
{
    m_connectionLabel->setText(summary.isEmpty() ? QStringLiteral("点击连接数据库") : summary);
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

void ServiceSqlWorkbenchWidget::showResult(const ServiceResult &result)
{
    if (!result.ok) {
        showResultMessage(result.message);
        return;
    }
    showResultTable(result);
}

void ServiceSqlWorkbenchWidget::showResultTable(const ServiceResult &result)
{
    while (m_resultTabs->count() > 1) {
        QWidget *widget = m_resultTabs->widget(1);
        m_resultTabs->removeTab(1);
        widget->deleteLater();
    }

    if (result.rows.isEmpty()) {
        showResultMessage(result.message.isEmpty() ? QStringLiteral("暂无数据") : result.message);
        return;
    }

    auto *table = makeResultTable(m_resultStack);
    fillResultTable(table, result);

    if (m_resultStack->count() == 1) {
        m_resultStack->addWidget(table);
    } else {
        QWidget *old = m_resultStack->widget(1);
        m_resultStack->removeWidget(old);
        old->deleteLater();
        m_resultStack->addWidget(table);
    }
    m_resultStack->setCurrentWidget(table);
    m_resultTabs->setTabText(0, QStringLiteral("结果0"));
    m_resultTabs->setCurrentIndex(0);
}

void ServiceSqlWorkbenchWidget::showResultMessage(const QString &message)
{
    while (m_resultTabs->count() > 1) {
        QWidget *widget = m_resultTabs->widget(1);
        m_resultTabs->removeTab(1);
        widget->deleteLater();
    }
    if (m_resultStack->count() > 1) {
        QWidget *old = m_resultStack->widget(1);
        m_resultStack->removeWidget(old);
        old->deleteLater();
    }
    m_resultEmpty->setText(message.isEmpty() ? QStringLiteral("暂无数据") : message);
    m_resultStack->setCurrentWidget(m_resultEmpty);
    m_resultTabs->setTabText(0, QStringLiteral("结果0"));
    m_resultTabs->setCurrentIndex(0);
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
    QWidget *current = m_resultTabs->currentWidget();
    auto *table = qobject_cast<QTableWidget *>(current);
    if (table == nullptr && m_resultStack != nullptr && m_resultStack->currentWidget() != m_resultEmpty) {
        table = qobject_cast<QTableWidget *>(m_resultStack->currentWidget());
    }
    if (table == nullptr) {
        return;
    }
    const int row = table->currentRow();
    if (row < 0) {
        return;
    }
    QStringList cells;
    for (int col = 0; col < table->columnCount(); ++col) {
        const QTableWidgetItem *item = table->item(row, col);
        cells.append(item != nullptr ? item->text() : QString());
    }
    QApplication::clipboard()->setText(cells.join(QLatin1Char('\t')));
}

void ServiceSqlWorkbenchWidget::onCopyColumn()
{
    QWidget *current = m_resultTabs->currentWidget();
    auto *table = qobject_cast<QTableWidget *>(current);
    if (table == nullptr && m_resultStack != nullptr && m_resultStack->currentWidget() != m_resultEmpty) {
        table = qobject_cast<QTableWidget *>(m_resultStack->currentWidget());
    }
    if (table == nullptr) {
        return;
    }
    const int col = table->currentColumn();
    if (col < 0) {
        return;
    }
    QStringList cells;
    for (int row = 0; row < table->rowCount(); ++row) {
        const QTableWidgetItem *item = table->item(row, col);
        cells.append(item != nullptr ? item->text() : QString());
    }
    QApplication::clipboard()->setText(cells.join(QLatin1Char('\n')));
}

void ServiceSqlWorkbenchWidget::onTableTreeActivated(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (item == nullptr || item->parent() == nullptr) {
        return;
    }
    emit tableSelected(item->text(0));
}
