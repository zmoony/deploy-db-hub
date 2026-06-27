#include "ui/tools/pages/RegexToolPage.h"

#include "tools/CommonTools.h"
#include "ui/PageLayout.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
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

} // namespace

namespace Ui {
namespace Tools {

RegexToolPage::RegexToolPage(QWidget *parent)
    : ToolPage(parent)
{
    setObjectName(QStringLiteral("regexToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *pattern = new QLineEdit(this);
    pattern->setPlaceholderText(QStringLiteral("(deploy)-(?<id>\\d+)"));
    PageLayout::configureFormInput(pattern);
    layout->addWidget(pattern);

    auto *options = new QWidget(this);
    auto *optionsLayout = new QHBoxLayout(options);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(PageLayout::Space12);
    auto *ignoreCase = new QCheckBox(QStringLiteral("忽略大小写"), options);
    auto *multiline = new QCheckBox(QStringLiteral("多行 ^$"), options);
    auto *dotAll = new QCheckBox(QStringLiteral(". 匹配换行"), options);
    auto *matchButton = makeActionButton(QStringLiteral("匹配"), options);
    optionsLayout->addWidget(ignoreCase);
    optionsLayout->addWidget(multiline);
    optionsLayout->addWidget(dotAll);
    optionsLayout->addWidget(matchButton);
    optionsLayout->addStretch();
    layout->addWidget(options);

    auto *input = makeEditor(QStringLiteral("deploy-42 deploy-7"), this);
    auto *result = new QTreeWidget(this);
    result->setColumnCount(3);
    result->setHeaderLabels({QStringLiteral("匹配 / 分组"), QStringLiteral("内容"), QStringLiteral("位置")});
    result->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    result->header()->setStretchLastSection(true);
    layout->addWidget(input, 1);
    layout->addWidget(result, 1);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(matchButton, &QPushButton::clicked, this,
            [pattern, input, result, message, ignoreCase, multiline, dotAll]() {
        result->clear();
        CommonTools::RegexOptions opts;
        opts.caseInsensitive = ignoreCase->isChecked();
        opts.multiline = multiline->isChecked();
        opts.dotMatchesAll = dotAll->isChecked();
        QString error;
        const QVector<CommonTools::RegexMatch> matches =
            CommonTools::runRegularExpression(pattern->text(), input->toPlainText(), opts, &error);
        if (!error.isEmpty()) {
            message->setText(QStringLiteral("正则无效：%1").arg(error));
            return;
        }
        if (matches.isEmpty()) {
            message->setText(QStringLiteral("未匹配"));
            return;
        }
        int index = 1;
        for (const CommonTools::RegexMatch &match : matches) {
            auto *matchItem = new QTreeWidgetItem(result);
            matchItem->setText(0, QStringLiteral("匹配 %1").arg(index++));
            matchItem->setText(1, match.text);
            matchItem->setText(2, QStringLiteral("%1, 长度 %2").arg(match.start).arg(match.length));
            for (const CommonTools::RegexGroup &group : match.groups) {
                auto *groupItem = new QTreeWidgetItem(matchItem);
                const QString label = group.name.isEmpty()
                    ? QStringLiteral("组 %1").arg(group.index)
                    : QStringLiteral("组 %1 (%2)").arg(group.index).arg(group.name);
                groupItem->setText(0, label);
                groupItem->setText(1, group.matched ? group.text : QStringLiteral("<未参与匹配>"));
            }
        }
        result->expandAll();
        message->setText(QStringLiteral("匹配 %1 处").arg(matches.size()));
    });
}

QString RegexToolPage::title() const
{
    return QStringLiteral("正则匹配");
}

QString RegexToolPage::subtitle() const
{
    return QStringLiteral("正则表达式匹配和替换");
}

} // namespace Tools
} // namespace Ui
