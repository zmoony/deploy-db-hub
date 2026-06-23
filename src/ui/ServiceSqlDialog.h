#pragma once

#include "adapters/services/RedisServiceClient.h"

#include <QDialog>

class QPlainTextEdit;
class QTableWidget;

class ServiceSqlDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceSqlDialog(QWidget *parent = nullptr);

    void setSql(const QString &sql);
    QString sql() const;
    void setResult(const ServiceResult &result);

private:
    QPlainTextEdit *m_editor = nullptr;
    QTableWidget *m_table = nullptr;
    QPlainTextEdit *m_message = nullptr;
};
