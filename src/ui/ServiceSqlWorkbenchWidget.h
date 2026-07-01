#pragma once

#include "adapters/services/RedisServiceClient.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPlainTextEdit;
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
    void onTableTreeActivated(QTreeWidgetItem *item, int column);
    void onTableContextMenu(const QPoint &pos);

private:
    void updateEditorHint();
    void showResultTable(const ServiceResult &result, qint64 elapsedMs);
    void showResultMessage(const QString &message);
    void setPagedRows(const QVector<QJsonObject> &rows, const QStringList &headers);
    void renderResultPage();
    void updateResultMeta();
    QTableWidget *currentResultTable() const;
    QJsonObject rowAsJson(int row) const;
    void exportRows(const QString &format) const;

    QTreeWidget *m_tableTree = nullptr;
    QLabel *m_connectionLabel = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_resultMetaLabel = nullptr;
    QPushButton *m_executeButton = nullptr;
    QPushButton *m_prevPageButton = nullptr;
    QPushButton *m_nextPageButton = nullptr;
    QComboBox *m_pageSizeCombo = nullptr;
    QLabel *m_pageInfoLabel = nullptr;
    QTableWidget *m_resultTable = nullptr;
    QStackedWidget *m_resultStack = nullptr;
    QLabel *m_resultEmpty = nullptr;

    QString m_currentSchema;
    QVector<QJsonObject> m_allRows;
    QStringList m_headers;
    int m_pageIndex = 0;
    int m_pageSize = 50;
    int m_totalRows = 0;
    qint64 m_lastElapsedMs = -1;
};
