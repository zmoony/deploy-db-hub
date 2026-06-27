#include "ui/tools/pages/DiffToolPage.h"

#include "tools/CommonTools.h"
#include "ui/CommonToolsWidget.h"
#include "ui/PageLayout.h"

#include "ui/tools/ToolUiHelpers.h"
#include <QColor>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

QPlainTextEdit *makeEditor(const QString &placeholder, QWidget *parent)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setPlaceholderText(placeholder);
    editor->setMinimumHeight(220);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    return editor;
}

void applyHighlights(QPlainTextEdit *editor, const QVector<int> &lines)
{
    QList<QTextEdit::ExtraSelection> selections;
    QTextCharFormat fmt;
    fmt.setBackground(QColor(0xFF, 0xD7, 0xD7));
    fmt.setProperty(QTextFormat::FullWidthSelection, true);
    QTextDocument *doc = editor->document();
    for (int line : lines) {
        const QTextBlock block = doc->findBlockByNumber(line);
        if (!block.isValid()) {
            continue;
        }
        QTextEdit::ExtraSelection selection;
        selection.cursor = QTextCursor(block);
        selection.format = fmt;
        selections.append(selection);
    }
    editor->setExtraSelections(selections);
}

} // namespace

namespace Ui {
namespace Tools {

DiffToolPage::DiffToolPage(QWidget *parent)
    : ToolPage(parent)
{
    auto *commonTools = qobject_cast<CommonToolsWidget *>(parent);

    setObjectName(QStringLiteral("diffToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *openLeft = Helpers::makeToolButton(QStringLiteral("选择源文件"), toolbar);
    auto *openRight = Helpers::makeToolButton(QStringLiteral("选择对比文件"), toolbar);
    auto *swap = Helpers::makeToolButton(QStringLiteral("交换两侧"), toolbar);
    auto *copyLeft = Helpers::makeToolButton(QStringLiteral("复制源"), toolbar);
    auto *copyRight = Helpers::makeToolButton(QStringLiteral("复制对比"), toolbar);
    auto *aiAssistButton = Helpers::makeToolButton(QStringLiteral("AI 辅助"), toolbar);
    auto *aiStopButton = Helpers::makeToolButton(QStringLiteral("停止"), toolbar);
    aiStopButton->setEnabled(false);
    toolbarLayout->addWidget(openLeft);
    toolbarLayout->addWidget(openRight);
    toolbarLayout->addWidget(swap);
    toolbarLayout->addWidget(copyLeft);
    toolbarLayout->addWidget(copyRight);
    toolbarLayout->addWidget(aiAssistButton);
    toolbarLayout->addWidget(aiStopButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(this);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *left = makeEditor(QStringLiteral("源文本，可粘贴或选择文件"), editors);
    auto *right = makeEditor(QStringLiteral("对比文本，可粘贴或选择文件"), editors);
    editorsLayout->addWidget(left, 1);
    editorsLayout->addWidget(right, 1);
    layout->addWidget(editors, 1);

    auto *aiOutput = makeEditor(QStringLiteral("AI 差异摘要"), this);
    aiOutput->setReadOnly(true);
    aiOutput->setMaximumHeight(180);
    layout->addWidget(aiOutput);

    auto *status = new QLabel(this);
    status->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(status);

    auto recompute = [left, right, status]() {
        const CommonTools::LineDiff diff =
            CommonTools::diffLineIndices(left->toPlainText(), right->toPlainText());
        applyHighlights(left, diff.leftChangedLines);
        applyHighlights(right, diff.rightChangedLines);
        status->setText(QStringLiteral("差异：源 %1 行，对比 %2 行；相同 %3 行")
            .arg(diff.leftChangedLines.size())
            .arg(diff.rightChangedLines.size())
            .arg(diff.sameCount));
    };

    connect(left, &QPlainTextEdit::textChanged, this, recompute);
    connect(right, &QPlainTextEdit::textChanged, this, recompute);
    connect(left->verticalScrollBar(), &QScrollBar::valueChanged,
            right->verticalScrollBar(), &QScrollBar::setValue);
    connect(right->verticalScrollBar(), &QScrollBar::valueChanged,
            left->verticalScrollBar(), &QScrollBar::setValue);

    connect(openLeft, &QPushButton::clicked, this, [this, left]() {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择源文件"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            left->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });
    connect(openRight, &QPushButton::clicked, this, [this, right]() {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择对比文件"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            right->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });
    connect(swap, &QPushButton::clicked, this, [left, right]() {
        const QString leftText = left->toPlainText();
        left->setPlainText(right->toPlainText());
        right->setPlainText(leftText);
    });
    connect(copyLeft, &QPushButton::clicked, this, [left]() {
        Helpers::copyToClipboard(left->toPlainText());
    });
    connect(copyRight, &QPushButton::clicked, this, [right]() {
        Helpers::copyToClipboard(right->toPlainText());
    });

    if (commonTools != nullptr) {
        commonTools->wireAiAssist(this,
                                  aiAssistButton,
                                  aiStopButton,
                                  aiOutput,
                                  status,
                                  [left, right]() {
            return QStringLiteral("Source:\n%1\n\nTarget:\n%2").arg(left->toPlainText(), right->toPlainText());
        },
                                  QStringLiteral("Summarize text differences in concise Chinese. Focus on what changed, added, or removed."));
    }
}

QString DiffToolPage::title() const
{
    return QStringLiteral("文本 Diff");
}

QString DiffToolPage::subtitle() const
{
    return QStringLiteral("比较两段文本差异");
}

} // namespace Tools
} // namespace Ui
