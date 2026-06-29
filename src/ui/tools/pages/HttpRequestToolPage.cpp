#include "ui/tools/pages/HttpRequestToolPage.h"

#include "ui/HttpRequestWidget.h"

#include <QVBoxLayout>

namespace Ui {
namespace Tools {

HttpRequestToolPage::HttpRequestToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("httpRequestToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *widget = new HttpRequestWidget(this);
    layout->addWidget(widget, 1);
}

QString HttpRequestToolPage::title() const
{
    return QStringLiteral("HTTP 请求");
}

QString HttpRequestToolPage::subtitle() const
{
    return QStringLiteral("发送 HTTP 请求");
}

} // namespace Tools
} // namespace Ui
