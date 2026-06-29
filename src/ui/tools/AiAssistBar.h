#pragma once

#include <QWidget>

class QPushButton;

namespace Ui {
namespace Tools {

class AiAssistBar : public QWidget {
    Q_OBJECT
public:
    explicit AiAssistBar(QWidget *parent = nullptr);

    QPushButton *assistButton() const;
    QPushButton *stopButton() const;

    void setRunning(bool running);

signals:
    void assistClicked();
    void stopClicked();

private:
    QPushButton *m_assistButton = nullptr;
    QPushButton *m_stopButton = nullptr;
};

} // namespace Tools
} // namespace Ui
