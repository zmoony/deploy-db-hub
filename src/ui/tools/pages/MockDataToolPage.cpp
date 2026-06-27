#include "ui/tools/pages/MockDataToolPage.h"

#include "tools/CommonTools.h"
#include "ui/CommonToolsWidget.h"
#include "ui/PageLayout.h"
#include "ui/tools/ToolEditor.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>


namespace Ui {
namespace Tools {

MockDataToolPage::MockDataToolPage(QWidget *parent)
    : ToolPage(parent)
{
    auto *commonTools = qobject_cast<CommonToolsWidget *>(parent);

    setObjectName(QStringLiteral("mockDataToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *primaryButton = Helpers::makeToolButton(QStringLiteral("生成 Mock"), toolbar);
    auto *aiAssistButton = Helpers::makeToolButton(QStringLiteral("AI 辅助"), toolbar);
    auto *aiStopButton = Helpers::makeToolButton(QStringLiteral("停止"), toolbar);
    aiStopButton->setEnabled(false);
    toolbarLayout->addWidget(primaryButton);
    toolbarLayout->addWidget(aiAssistButton);
    toolbarLayout->addWidget(aiStopButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(this);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = new ToolEditor(editors);
    input->setPlaceholderText(QStringLiteral("{\"name\":\"demo\",\"count\":1,\"enabled\":true}"));
    auto *output = new ToolEditor(editors);
    output->setPlaceholderText(QStringLiteral("输出"));
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(primaryButton, &QPushButton::clicked, this, [input, output, message]() {
        QString error;
        const QString result = CommonTools::mockFromJsonExample(input->text(), &error);
        if (!error.isEmpty()) {
            output->clear();
            message->setText(error);
            return;
        }
        output->setText(result);
        message->setText(QStringLiteral("已完成"));
    });

    if (commonTools != nullptr) {
        commonTools->wireAiAssist(this,
                                  aiAssistButton,
                                  aiStopButton,
                                  output->editor(),
                                  message,
                                  [input]() { return input->text(); },
                                  QStringLiteral("You generate mock JSON data. Reply with a JSON array only, no markdown or explanation."));
    }
}

QString MockDataToolPage::title() const
{
    return QStringLiteral("契约 Mock 数据");
}

QString MockDataToolPage::subtitle() const
{
    return QStringLiteral("粘贴 Swagger/Thrift 定义或 JSON 示例，生成多组 mock 数据。");
}

} // namespace Tools
} // namespace Ui
