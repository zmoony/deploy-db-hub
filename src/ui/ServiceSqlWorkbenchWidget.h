#pragma once

#include "adapters/services/RedisServiceClient.h"
#include "ui/SqlColumnFilterPopup.h"

#include <QHash>
#include <QWidget>

class SqlCodeEditor;
class SqlResultHeaderView;
class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

class ServiceSqlWorkbenchWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceSqlWorkbenchWidget(QWidget *parent = nullptr);

    void setConnectionSummary(const QString &summary);
    void setCurrentSchema(const QString &schema);
    void setTables(const QStringList &tables);
    void setExecuting(bool executing);
    void showResult(const ServiceResult &result, qint64 elapsedMs = -1);
    QString currentSql() const;
    void setSqlText(const QString &sql);

signals:
    void executeRequested(const QString &sql);
    void tableSelected(const QString &tableName);
    void tableQueryRequested(const QString &tableName);
    void refreshTablesRequested();
    void showTableStructureRequested(const QString &tableName);
    void deleteTableRequested(const QString &tableName);

private slots:
    void onExecute();
    void onFormat();
    void onClear();
    void onCopyRow();
    void onCopyColumn();
    void onPrevPage();
    void onNextPage();
    void onPageSizeChanged(int index);
    void onTableTreeClicked(QTreeWidgetItem *item, int column);
    void onTableTreeDoubleClicked(QTreeWidgetItem *item, int column);
    void onTableContextMenu(const QPoint &pos);
    void onResultHeaderSort(int section);
    void onResultHeaderFilter(int section, const QPoint &globalPos);
    void onColumnFilterApplied(int column, const SqlColumnFilterState &state);
    void onColumnFilterCleared(int column);

private:
    void updateEditorHint();
    void updateExecuteButtonLabel();
    void showResultTable(const ServiceResult &result, qint64 elapsedMs);
    void showResultMessage(const QString &message);
    void setPagedRows(const QVector<QJsonObject> &rows, const QStringList &headers);
    void applySortAndFilter();
    void applyResultColumnLayout();
    void updateHeaderIndicators();
    void renderResultPage();
    void updateResultMeta();
    QVector<QPair<QString, int>> columnValueCounts(int column) const;
    void showColumnFilterPopup(int column, const QPoint &globalPos);
    QTableWidget *currentResultTable() const;
    QJsonObject rowAsJson(int row) const;
    void exportRows(const QString &format) const;

    bool eventFilter(QObject *watched, QEvent *event) override;

    QTreeWidget *m_tableTree = nullptr;
    QLabel *m_connectionLabel = nullptr;
    SqlCodeEditor *m_editor = nullptr;
    QLabel *m_resultMetaLabel = nullptr;
    QPushButton *m_executeButton = nullptr;
    QPushButton *m_prevPageButton = nullptr;
    QPushButton *m_nextPageButton = nullptr;
    QComboBox *m_pageSizeCombo = nullptr;
    QLabel *m_pageInfoLabel = nullptr;
    QTableWidget *m_resultTable = nullptr;
    SqlResultHeaderView *m_resultHeader = nullptr;
    SqlColumnFilterPopup *m_columnFilterPopup = nullptr;
    QStackedWidget *m_resultStack = nullptr;
    QLabel *m_resultEmpty = nullptr;

    QString m_currentSchema;
    QVector<QJsonObject> m_allRows;
    QVector<QJsonObject> m_viewRows;
    QStringList m_headers;
    QHash<int, SqlColumnFilterState> m_columnFilters;
    int m_pageIndex = 0;
    int m_pageSize = 20;
    int m_totalRows = 0;
    int m_sourceRowCount = 0;
    int m_sortColumn = -1;
    bool m_sortAscending = true;
    bool m_resultColumnsStretch = true;
    qint64 m_lastElapsedMs = -1;
};
