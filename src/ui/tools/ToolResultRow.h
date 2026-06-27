#pragma once

#include <QWidget>

class QLineEdit;

namespace Ui {
namespace Tools {

class ToolResultRow : public QWidget {
    Q_OBJECT
public:
    explicit ToolResultRow(const QString &label, QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const;
    void clear();

signals:
    void copied();

private:
    QLineEdit *m_edit = nullptr;
};

} // namespace Tools
} // namespace Ui
