#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class UuidToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit UuidToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
