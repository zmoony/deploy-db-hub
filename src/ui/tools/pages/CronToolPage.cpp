#include "ui/tools/pages/CronToolPage.h"

#include "tools/CommonTools.h"
#include "ui/CommonToolsWidget.h"
#include "ui/PageLayout.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
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

} // namespace

namespace Ui {
namespace Tools {

CronToolPage::CronToolPage(QWidget *parent)
    : ToolPage(parent)
{
    auto *commonTools = qobject_cast<CommonToolsWidget *>(parent);

    setObjectName(QStringLiteral("cronToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *exprRow = new QWidget(this);
    auto *exprLayout = new QHBoxLayout(exprRow);
    exprLayout->setContentsMargins(0, 0, 0, 0);
    exprLayout->setSpacing(PageLayout::Space8);
    auto *expr = new QLineEdit(exprRow);
    expr->setText(QStringLiteral("0 0 12 * * ?"));
    PageLayout::configureFormInput(expr);
    auto *parseButton = makeActionButton(QStringLiteral("解析 / 预览"), exprRow);
    auto *aiAssistButton = makeActionButton(QStringLiteral("AI 辅助"), exprRow);
    auto *aiStopButton = makeActionButton(QStringLiteral("停止"), exprRow);
    aiStopButton->setEnabled(false);
    exprLayout->addWidget(expr, 1);
    exprLayout->addWidget(parseButton);
    exprLayout->addWidget(aiAssistButton);
    exprLayout->addWidget(aiStopButton);
    layout->addWidget(exprRow);

    auto *builder = new QWidget(this);
    auto *builderLayout = new QHBoxLayout(builder);
    builderLayout->setContentsMargins(0, 0, 0, 0);
    builderLayout->setSpacing(PageLayout::Space6);
    const QStringList fieldNames = {QStringLiteral("秒"), QStringLiteral("分"), QStringLiteral("时"),
                                    QStringLiteral("日"), QStringLiteral("月"), QStringLiteral("周"), QStringLiteral("年")};
    const QStringList fieldDefaults = {QStringLiteral("0"), QStringLiteral("0"), QStringLiteral("12"),
                                       QStringLiteral("*"), QStringLiteral("*"), QStringLiteral("?"), QString()};
    QVector<QLineEdit *> fieldEdits;
    for (int i = 0; i < fieldNames.size(); ++i) {
        auto *cell = new QWidget(builder);
        auto *cellLayout = new QVBoxLayout(cell);
        cellLayout->setContentsMargins(0, 0, 0, 0);
        cellLayout->setSpacing(2);
        auto *caption = new QLabel(fieldNames.at(i), cell);
        caption->setAlignment(Qt::AlignCenter);
        auto *edit = new QLineEdit(cell);
        edit->setText(fieldDefaults.at(i));
        edit->setAlignment(Qt::AlignCenter);
        PageLayout::configureFormInput(edit);
        edit->setMinimumWidth(56);
        fieldEdits.append(edit);
        cellLayout->addWidget(caption);
        cellLayout->addWidget(edit);
        builderLayout->addWidget(cell, 1);
    }
    auto *composeButton = makeActionButton(QStringLiteral("由字段生成"), builder);
    builderLayout->addWidget(composeButton);
    layout->addWidget(builder);

    auto *presets = new QWidget(this);
    auto *presetsLayout = new QHBoxLayout(presets);
    presetsLayout->setContentsMargins(0, 0, 0, 0);
    presetsLayout->setSpacing(PageLayout::Space6);
    const QVector<QPair<QString, QString>> presetList = {
        {QStringLiteral("每分钟"), QStringLiteral("0 * * * * ?")},
        {QStringLiteral("每小时"), QStringLiteral("0 0 * * * ?")},
        {QStringLiteral("每天 0 点"), QStringLiteral("0 0 0 * * ?")},
        {QStringLiteral("每周一 9 点"), QStringLiteral("0 0 9 ? * 1")},
        {QStringLiteral("每月 1 号"), QStringLiteral("0 0 0 1 * ?")}
    };
    for (const auto &preset : presetList) {
        auto *button = makeActionButton(preset.first, presets);
        const QString value = preset.second;
        connect(button, &QPushButton::clicked, expr, [expr, value]() {
            expr->setText(value);
        });
        presetsLayout->addWidget(button);
    }
    presetsLayout->addStretch();
    layout->addWidget(presets);

    auto *description = new QLabel(this);
    description->setObjectName(QStringLiteral("sectionLabel"));
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *runs = new QListWidget(this);
    layout->addWidget(runs, 1);

    auto *aiOutput = makeEditor(QStringLiteral("AI 解释"), this);
    aiOutput->setReadOnly(true);
    aiOutput->setMaximumHeight(160);
    layout->addWidget(aiOutput);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(composeButton, &QPushButton::clicked, this, [expr, fieldEdits]() {
        QStringList parts;
        for (QLineEdit *edit : fieldEdits) {
            const QString value = edit->text().trimmed();
            if (value.isEmpty()) {
                break;
            }
            parts.append(value);
        }
        if (parts.size() >= 5) {
            expr->setText(parts.join(QLatin1Char(' ')));
        }
    });

    connect(parseButton, &QPushButton::clicked, this, [expr, description, runs, message]() {
        runs->clear();
        QString error;
        const CommonTools::CronSchedule schedule =
            CommonTools::analyzeCron(expr->text(), QDateTime::currentDateTime(), 10, &error);
        if (!schedule.valid) {
            description->clear();
            message->setText(QStringLiteral("解析失败：%1").arg(error));
            return;
        }
        description->setText(schedule.description);
        for (const QDateTime &run : schedule.nextRuns) {
            runs->addItem(run.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss dddd")));
        }
        if (schedule.nextRuns.isEmpty()) {
            message->setText(QStringLiteral("未来一段时间内没有匹配的执行时间"));
        } else {
            message->setText(QStringLiteral("已列出未来 %1 次执行时间").arg(schedule.nextRuns.size()));
        }
    });

    if (commonTools != nullptr) {
        commonTools->wireAiAssist(this,
                                  aiAssistButton,
                                  aiStopButton,
                                  aiOutput,
                                  message,
                                  [expr]() { return expr->text(); },
                                  QStringLiteral("Explain the cron expression in concise Chinese, including field meanings and typical schedule."));
    }
}

QString CronToolPage::title() const
{
    return QStringLiteral("Cron 解析");
}

QString CronToolPage::subtitle() const
{
    return QStringLiteral("解析 Cron 表达式");
}

} // namespace Tools
} // namespace Ui
