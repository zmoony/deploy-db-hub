#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class Base64TextToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit Base64TextToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
