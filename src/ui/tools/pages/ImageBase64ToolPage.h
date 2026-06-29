#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class ImageBase64ToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit ImageBase64ToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
