#pragma once

#include "adapters/services/RedisServiceClient.h"

#include <QDialog>
#include <QStringList>

class QPlainTextEdit;
class QTableWidget;
class QLabel;

class ServiceSqlDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceSqlDialog(QWidget *parent = nullptr);

    void setSql(const QString &sql);
    QString sql() const;
    void setResult(const ServiceResult &result);
    void setTableResult(const ServiceResult &result, const QStringList &preferredHeaders = {});

private:
    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_sqlLabel = nullptr;
    QLabel *m_resultLabel = nullptr;
    QTableWidget *m_table = nullptr;
    QPlainTextEdit *m_message = nullptr;
};
