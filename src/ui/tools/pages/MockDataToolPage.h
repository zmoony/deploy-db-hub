#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class MockDataToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit MockDataToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
