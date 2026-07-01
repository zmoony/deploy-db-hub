#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;

class ServiceRedisSearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServiceRedisSearchDialog(QWidget *parent = nullptr);

    QString pattern() const;
    QString typeFilter() const;

    static QString typeFilterAll();

private:
    QLineEdit *m_patternEdit = nullptr;
    QComboBox *m_typeCombo = nullptr;
};
