#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class NumberBaseToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit NumberBaseToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
