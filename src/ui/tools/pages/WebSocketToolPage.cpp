#include "ui/tools/pages/WebSocketToolPage.h"

#include "ui/WebSocketToolWidget.h"

#include <QVBoxLayout>

namespace Ui {
namespace Tools {

WebSocketToolPage::WebSocketToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("webSocketToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *widget = new WebSocketToolWidget(this);
    layout->addWidget(widget, 1);
}

QString WebSocketToolPage::title() const
{
    return QStringLiteral("WebSocket");
}

QString WebSocketToolPage::subtitle() const
{
    return QStringLiteral("WebSocket 测试");
}

} // namespace Tools
} // namespace Ui
