#include "ui/tools/pages/JwtToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QPushButton *makeActionButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolBarButton"));
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

QPlainTextEdit *makeEditor(const QString &placeholder, QWidget *parent)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setPlaceholderText(placeholder);
    editor->setMinimumHeight(220);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    return editor;
}

void copyTextToClipboard(const QString &text)
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }
}

} // namespace

namespace Ui {
namespace Tools {

JwtToolPage::JwtToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("jwtToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *parse = makeActionButton(QStringLiteral("解析"), toolbar);
    auto *copy = makeActionButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(parse);
    toolbarLayout->addWidget(copy);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *input = makeEditor(QStringLiteral("粘贴 JWT (xxxxx.yyyyy.zzzzz)"), this);
    input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *output = makeEditor(QStringLiteral("解析结果"), this);
    output->setReadOnly(true);
    layout->addWidget(input, 1);
    layout->addWidget(output, 1);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(parse, &QPushButton::clicked, this, [input, output, message]() {
        QString error;
        const QString result = CommonTools::decodeJwt(input->toPlainText(), &error);
        if (!error.isEmpty()) {
            output->clear();
            message->setText(error);
            return;
        }
        output->setPlainText(result);
        message->setText(QStringLiteral("已解析"));
    });
    connect(copy, &QPushButton::clicked, this, [output]() {
        copyTextToClipboard(output->toPlainText());
    });
}

QString JwtToolPage::title() const
{
    return QStringLiteral("JWT 解码");
}

QString JwtToolPage::subtitle() const
{
    return QStringLiteral("解码 JWT Token");
}

} // namespace Tools
} // namespace Ui
