#pragma once

#include <QWidget>

namespace Ui {
namespace Tools {

class ToolPage : public QWidget {
    Q_OBJECT
public:
    explicit ToolPage(QWidget *parent = nullptr);
    virtual ~ToolPage() = default;

    virtual QString title() const = 0;
    virtual QString subtitle() const;

signals:
    void statusMessage(const QString &text, int timeoutMs = 3000);
};

} // namespace Tools
} // namespace Ui
