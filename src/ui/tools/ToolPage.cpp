#include "ui/tools/ToolPage.h"

namespace Ui {
namespace Tools {

ToolPage::ToolPage(QWidget *parent)
    : QWidget(parent)
{
}

QString ToolPage::subtitle() const
{
    return QString();
}

} // namespace Tools
} // namespace Ui
