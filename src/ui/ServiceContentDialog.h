#pragma once

#include <QDialog>

class QPlainTextEdit;

class ServiceContentDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceContentDialog(const QString &title, QWidget *parent = nullptr);

    void setContent(const QString &content);

private:
    QPlainTextEdit *m_view = nullptr;
};
