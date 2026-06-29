#pragma once

#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

class CaseToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit CaseToolPage(QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;
};

} // namespace Tools
} // namespace Ui
