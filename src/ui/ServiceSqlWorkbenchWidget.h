#pragma once

#include "adapters/services/RedisServiceClient.h"

#include <QWidget>

class QFrame;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

class ServiceSqlWorkbenchWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceSqlWorkbenchWidget(QWidget *parent = nullptr);

    void setConnectionSummary(const QString &summary);
    void setTables(const QStringList &tables);
    void setExecuting(bool executing);
    void showResult(const ServiceResult &result);
    QString currentSql() const;
    void setSqlText(const QString &sql);

signals:
    void executeRequested(const QString &sql);
    void tableSelected(const QString &tableName);
    void refreshTablesRequested();

private slots:
    void onExecute();
    void onFormat();
    void onClear();
    void onCopyRow();
    void onCopyColumn();
    void onTableTreeActivated(QTreeWidgetItem *item, int column);

private:
    void showResultTable(const ServiceResult &result);
    void showResultMessage(const QString &message);

    QTreeWidget *m_tableTree = nullptr;
    QLabel *m_connectionLabel = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QTabWidget *m_resultTabs = nullptr;
    QStackedWidget *m_resultStack = nullptr;
    QLabel *m_resultEmpty = nullptr;
    QPushButton *m_executeButton = nullptr;
};
