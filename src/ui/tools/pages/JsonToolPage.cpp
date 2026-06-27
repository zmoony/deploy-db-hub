#include "ui/tools/pages/JsonToolPage.h"

#include "tools/CommonTools.h"
#include "ui/CommonToolsWidget.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

namespace {

const QColor kJsonKeyColor(0x1F, 0x29, 0x33);
const QColor kJsonStringColor(0x2E, 0x7D, 0x32);
const QColor kJsonNumberColor(0xC2, 0x18, 0x5B);
const QColor kJsonBoolColor(0x6A, 0x1B, 0x9A);
const QColor kJsonNullColor(0xF5, 0x7C, 0x00);

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

QString jsonScalarText(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QJsonValue::Double: {
        const double number = value.toDouble();
        const qint64 rounded = qRound64(number);
        if (qAbs(number) < 1e15 && qFuzzyCompare(number + 1.0, static_cast<double>(rounded) + 1.0)) {
            return QString::number(rounded);
        }
        return QString::number(number, 'g', 15);
    }
    case QJsonValue::String:
        return value.toString();
    case QJsonValue::Null:
        return QStringLiteral("null");
    default:
        return QString();
    }
}

QColor jsonValueColor(const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Double:
        return kJsonNumberColor;
    case QJsonValue::String:
        return kJsonStringColor;
    case QJsonValue::Bool:
        return kJsonBoolColor;
    case QJsonValue::Null:
        return kJsonNullColor;
    default:
        return kJsonKeyColor;
    }
}

void populateJsonTree(QTreeWidgetItem *parent, const QString &keyText, const QJsonValue &value)
{
    auto *item = new QTreeWidgetItem(parent);
    item->setForeground(0, QBrush(kJsonKeyColor));

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        item->setText(0, keyText);
        item->setText(1, QStringLiteral("{ %1 }").arg(object.size()));
        item->setForeground(1, QBrush(kJsonKeyColor));
        for (auto it = object.begin(); it != object.end(); ++it) {
            populateJsonTree(item, it.key(), it.value());
        }
    } else if (value.isArray()) {
        const QJsonArray array = value.toArray();
        item->setText(0, keyText);
        item->setText(1, QStringLiteral("[ %1 ]").arg(array.size()));
        item->setForeground(1, QBrush(kJsonKeyColor));
        for (int i = 0; i < array.size(); ++i) {
            populateJsonTree(item, QStringLiteral("[%1]").arg(i), array.at(i));
        }
    } else {
        item->setText(0, keyText);
        item->setText(1, jsonScalarText(value));
        item->setForeground(1, QBrush(jsonValueColor(value)));
    }
}

} // namespace

namespace Ui {
namespace Tools {

JsonToolPage::JsonToolPage(QWidget *parent)
    : ToolPage(parent)
{
    auto *commonTools = qobject_cast<CommonToolsWidget *>(parent);

    setObjectName(QStringLiteral("jsonToolPage"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *formatButton = makeActionButton(QStringLiteral("格式化"), toolbar);
    auto *expandButton = makeActionButton(QStringLiteral("全部展开"), toolbar);
    auto *collapseButton = makeActionButton(QStringLiteral("全部折叠"), toolbar);
    auto *copyButton = makeActionButton(QStringLiteral("复制格式化结果"), toolbar);
    auto *clearButton = makeActionButton(QStringLiteral("清空"), toolbar);
    auto *aiAssistButton = makeActionButton(QStringLiteral("AI 辅助"), toolbar);
    auto *aiStopButton = makeActionButton(QStringLiteral("停止"), toolbar);
    aiStopButton->setEnabled(false);
    auto *search = new QLineEdit(toolbar);
    search->setPlaceholderText(QStringLiteral("搜索键或值"));
    search->setClearButtonEnabled(true);
    PageLayout::configureFormInput(search);
    toolbarLayout->addWidget(formatButton);
    toolbarLayout->addWidget(expandButton);
    toolbarLayout->addWidget(collapseButton);
    toolbarLayout->addWidget(copyButton);
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addWidget(aiAssistButton);
    toolbarLayout->addWidget(aiStopButton);
    toolbarLayout->addWidget(search, 1);
    layout->addWidget(toolbar);

    auto *body = new QWidget(this);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space12);

    auto *input = makeEditor(QStringLiteral("{\"name\":\"deploy-hub\",\"port\":8080,\"enabled\":true,\"extra\":null}"), body);
    auto *rightPanel = new QStackedWidget(body);
    auto *tree = new QTreeWidget(rightPanel);
    tree->setObjectName(QStringLiteral("jsonTree"));
    tree->setColumnCount(2);
    tree->setHeaderLabels({QStringLiteral("键 / 索引"), QStringLiteral("值")});
    tree->setUniformRowHeights(true);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setStretchLastSection(true);
    auto *aiOutput = makeEditor(QStringLiteral("AI 输出"), rightPanel);
    aiOutput->setReadOnly(true);
    rightPanel->addWidget(tree);
    rightPanel->addWidget(aiOutput);
    bodyLayout->addWidget(input, 1);
    bodyLayout->addWidget(rightPanel, 1);
    layout->addWidget(body, 1);

    auto *message = new QLabel(this);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    auto rebuildTree = [tree, input, message]() {
        tree->clear();
        const QString text = input->toPlainText().trimmed();
        if (text.isEmpty()) {
            message->setText(QString());
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            message->setText(QStringLiteral("JSON 解析失败：%1").arg(parseError.errorString()));
            return;
        }
        auto *root = tree->invisibleRootItem();
        if (document.isObject()) {
            populateJsonTree(root, QStringLiteral("root"), document.object());
        } else if (document.isArray()) {
            populateJsonTree(root, QStringLiteral("root"), document.array());
        } else {
            populateJsonTree(root, QStringLiteral("root"), QJsonValue());
        }
        tree->expandAll();
        message->setText(QStringLiteral("已解析"));
    };

    connect(formatButton, &QPushButton::clicked, this, [input, rebuildTree]() {
        QString error;
        const QString formatted = CommonTools::formatJson(input->toPlainText(), &error);
        if (error.isEmpty() && !formatted.isEmpty()) {
            input->setPlainText(formatted);
        }
        rebuildTree();
    });
    connect(expandButton, &QPushButton::clicked, tree, &QTreeWidget::expandAll);
    connect(collapseButton, &QPushButton::clicked, tree, &QTreeWidget::collapseAll);
    connect(copyButton, &QPushButton::clicked, this, [input, message]() {
        QString error;
        const QString formatted = CommonTools::formatJson(input->toPlainText(), &error);
        if (!error.isEmpty()) {
            message->setText(QStringLiteral("无法复制：%1").arg(error));
            return;
        }
        copyTextToClipboard(formatted);
        message->setText(QStringLiteral("已复制格式化结果"));
    });
    connect(clearButton, &QPushButton::clicked, this, [input, tree, search, message]() {
        input->clear();
        tree->clear();
        search->clear();
        message->setText(QStringLiteral("已清空"));
    });

    auto copySelectedValue = [tree, message]() {
        QTreeWidgetItem *item = tree->currentItem();
        if (item == nullptr) {
            return;
        }
        const QString value = item->text(1);
        copyTextToClipboard(value.isEmpty() ? item->text(0) : value);
        message->setText(QStringLiteral("已复制选中内容"));
    };

    connect(tree, &QTreeWidget::customContextMenuRequested, tree,
            [tree, message](const QPoint &pos) {
        QTreeWidgetItem *item = tree->itemAt(pos);
        if (item == nullptr) {
            return;
        }
        QMenu menu(tree);
        QAction *copyValue = menu.addAction(QStringLiteral("复制值"));
        QAction *copyKey = menu.addAction(QStringLiteral("复制键"));
        QAction *copyPair = menu.addAction(QStringLiteral("复制键: 值"));
        QAction *chosen = menu.exec(tree->viewport()->mapToGlobal(pos));
        if (chosen == nullptr) {
            return;
        }
        if (chosen == copyValue) {
            copyTextToClipboard(item->text(1));
        } else if (chosen == copyKey) {
            copyTextToClipboard(item->text(0));
        } else if (chosen == copyPair) {
            copyTextToClipboard(QStringLiteral("%1: %2").arg(item->text(0), item->text(1)));
        }
        message->setText(QStringLiteral("已复制选中内容"));
    });

    auto *copyShortcut = new QShortcut(QKeySequence::Copy, tree);
    copyShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(copyShortcut, &QShortcut::activated, tree, copySelectedValue);

    connect(search, &QLineEdit::textChanged, tree, [tree](const QString &keyword) {
        const QString needle = keyword.trimmed();
        QTreeWidgetItemIterator it(tree);
        while (*it) {
            QTreeWidgetItem *item = *it;
            const bool match = needle.isEmpty()
                || item->text(0).contains(needle, Qt::CaseInsensitive)
                || item->text(1).contains(needle, Qt::CaseInsensitive);
            item->setHidden(!needle.isEmpty() && !match);
            if (match && !needle.isEmpty()) {
                for (QTreeWidgetItem *parent = item->parent(); parent != nullptr; parent = parent->parent()) {
                    parent->setHidden(false);
                    parent->setExpanded(true);
                }
            }
            ++it;
        }
    });

    rebuildTree();

    if (commonTools != nullptr) {
        commonTools->wireAiAssist(this,
                                  aiAssistButton,
                                  aiStopButton,
                                  aiOutput,
                                  message,
                                  [input]() { return input->toPlainText(); },
                                  QStringLiteral("You fix and format invalid JSON. Reply with valid JSON only, no markdown or explanation."),
                                  [rightPanel]() { rightPanel->setCurrentIndex(1); });
    }
}

QString JsonToolPage::title() const
{
    return QStringLiteral("JSON 格式化");
}

QString JsonToolPage::subtitle() const
{
    return QStringLiteral("美化和验证 JSON");
}

} // namespace Tools
} // namespace Ui
