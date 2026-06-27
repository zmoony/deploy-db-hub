#pragma once

#include <QWidget>

class QPushButton;

namespace Ui {
namespace Tools {

class ToolActionBar : public QWidget {
    Q_OBJECT
public:
    explicit ToolActionBar(QWidget *parent = nullptr);

    QPushButton *primaryButton() const;
    QPushButton *clearButton() const;
    QPushButton *copyButton() const;

    void setPrimaryText(const QString &text);
    void setCopyEnabled(bool enabled);

signals:
    void primaryClicked();
    void clearClicked();
    void copyClicked();

private:
    QPushButton *m_primaryButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_copyButton = nullptr;
};

} // namespace Tools
} // namespace Ui
