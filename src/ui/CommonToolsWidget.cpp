#include "ui/CommonToolsWidget.h"

#include "ui/AiChatWidget.h"
#include "adapters/ai/OpenAiChatClient.h"
#include "ui/AiAssistHelper.h"
#include "ui/tools/pages/AiConfigToolPage.h"
#include "ui/AiStreamBuffer.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "tools/CommonTools.h"
#include "ui/HttpRequestWidget.h"
#include "ui/PageLayout.h"
#include "ui/WebSocketToolWidget.h"

#include <QAction>
#include <QApplication>
#include <QBrush>
#include <QBuffer>
#include <QCheckBox>
#include <QClipboard>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSharedPointer>
#include <QSpinBox>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QStringList>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUrl>
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

QLineEdit *addResultRow(QFormLayout *form, const QString &label, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(PageLayout::Space8);
    auto *edit = new QLineEdit(row);
    edit->setReadOnly(true);
    PageLayout::configureFormInput(edit);
    auto *copy = new QPushButton(QStringLiteral("复制"), row);
    copy->setObjectName(QStringLiteral("formActionButton"));
    copy->setFixedHeight(PageLayout::DialogFieldHeight);
    QObject::connect(copy, &QPushButton::clicked, edit, [edit]() {
        copyTextToClipboard(edit->text());
    });
    rowLayout->addWidget(edit, 1);
    rowLayout->addWidget(copy);
    form->addRow(label, row);
    return edit;
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

}

CommonToolsWidget::CommonToolsWidget(AiSettingsStore *aiSettings,
                                     CredentialStore *credentials,
                                     QWidget *parent)
    : QWidget(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(new Ui::Tools::AiConfigToolPage(m_aiSettings, m_credentials, this));
    m_stack->addWidget(new AiChatWidget(m_aiSettings, m_credentials, this));
    m_stack->addWidget(buildJsonViewerPage());
    m_stack->addWidget(buildDiffPage());
    m_stack->addWidget(buildImageBase64Page());
    m_stack->addWidget(buildRegexPage());
    m_stack->addWidget(buildCronPage());
    m_stack->addWidget(buildTimestampPage());
    m_stack->addWidget(buildHtmlEntityPage());
    m_stack->addWidget(buildHttpStatusPage());
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("Hex 转字符串"),
        QStringLiteral("将十六进制字节转为 UTF-8 字符串。"),
        QStringLiteral("转换"),
        QString(),
        QStringLiteral("48656c6c6f205174")));
    m_stack->addWidget(buildWebSocketPage());
    m_stack->addWidget(buildHttpRequestPage());
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("契约 Mock 数据"),
        QStringLiteral("粘贴 Swagger/Thrift 定义或 JSON 示例，生成多组 mock 数据。"),
        QStringLiteral("生成 Mock"),
        QString(),
        QStringLiteral("{\"name\":\"demo\",\"count\":1,\"enabled\":true}"),
        true,
        QStringLiteral("You generate mock JSON data. Reply with a JSON array only, no markdown or explanation.")));
    m_stack->addWidget(buildTextToolPage(
        QStringLiteral("数据采样脱敏"),
        QStringLiteral("从样本数据中抽样并按规则脱敏导出。"),
        QStringLiteral("脱敏"),
        QString(),
        QStringLiteral("phone=13812345678 email=a@example.com")));
    m_stack->addWidget(buildUuidPage());
    m_stack->addWidget(buildHashPage());
    m_stack->addWidget(buildUrlCodecPage());
    m_stack->addWidget(buildBase64TextPage());
    m_stack->addWidget(buildJwtPage());
    m_stack->addWidget(buildNumberBasePage());
    m_stack->addWidget(buildCasePage());

    layout->addWidget(m_stack, 1);
}

QStringList CommonToolsWidget::toolLabels() const
{
    return {
        QStringLiteral("AI 配置"),
        QStringLiteral("AI 聊天"),
        QStringLiteral("JSON 格式化"),
        QStringLiteral("文本比较"),
        QStringLiteral("图片/Base64"),
        QStringLiteral("正则表达式"),
        QStringLiteral("Cron 表达式"),
        QStringLiteral("时间戳"),
        QStringLiteral("HTML 特殊字符"),
        QStringLiteral("HTTP 状态码"),
        QStringLiteral("Hex 转字符串"),
        QStringLiteral("WebSocket 测试"),
        QStringLiteral("HTTP 请求调试"),
        QStringLiteral("契约 Mock 数据"),
        QStringLiteral("数据采样脱敏"),
        QStringLiteral("UUID 生成"),
        QStringLiteral("哈希计算"),
        QStringLiteral("URL 编解码"),
        QStringLiteral("Base64 文本"),
        QStringLiteral("JWT 解析"),
        QStringLiteral("进制转换"),
        QStringLiteral("命名转换")
    };
}

QWidget *CommonToolsWidget::takeToolPage(int index)
{
    if (m_stack == nullptr || index < 0 || index >= m_stack->count()) {
        return nullptr;
    }
    QWidget *page = m_stack->widget(index);
    m_stack->removeWidget(page);
    page->setParent(nullptr);
    return page;
}

int CommonToolsWidget::toolPageCount() const
{
    return m_stack != nullptr ? m_stack->count() : 0;
}

QWidget *CommonToolsWidget::buildTextToolPage(const QString &title,
                                              const QString &subtitle,
                                              const QString &primaryAction,
                                              const QString &secondaryAction,
                                              const QString &placeholder,
                                              bool enableAiAssist,
                                              const QString &aiSystemPrompt)
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);

    auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *primaryButton = makeActionButton(primaryAction, toolbar);
    toolbarLayout->addWidget(primaryButton);
    QPushButton *secondaryButton = nullptr;
    if (!secondaryAction.isEmpty()) {
        secondaryButton = makeActionButton(secondaryAction, toolbar);
        toolbarLayout->addWidget(secondaryButton);
    }
    QPushButton *aiAssistButton = nullptr;
    QPushButton *aiStopButton = nullptr;
    if (enableAiAssist) {
        aiAssistButton = makeActionButton(QStringLiteral("AI 辅助"), toolbar);
        aiStopButton = makeActionButton(QStringLiteral("停止"), toolbar);
        aiStopButton->setEnabled(false);
        toolbarLayout->addWidget(aiAssistButton);
        toolbarLayout->addWidget(aiStopButton);
    }
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(placeholder, editors);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(primaryButton, &QPushButton::clicked, this, [this, title, input, output, message]() {
        QString error;
        QString result;
        if (title == QStringLiteral("JSON 格式化")) {
            result = CommonTools::formatJson(input->toPlainText(), &error);
        } else if (title == QStringLiteral("文件差异")) {
            const QStringList parts = input->toPlainText().split(QStringLiteral("\n---\n"));
            if (parts.size() != 2) {
                error = QStringLiteral("请用单独一行 --- 分隔两段文本");
            } else {
                result = CommonTools::compareLines(parts.at(0), parts.at(1));
            }
        } else if (title == QStringLiteral("图片/Base64")) {
            result = CommonTools::textToBase64(input->toPlainText());
        } else if (title == QStringLiteral("正则表达式")) {
            const QStringList parts = input->toPlainText().split(QStringLiteral("\n---\n"));
            if (parts.size() != 2) {
                error = QStringLiteral("请用单独一行 --- 分隔正则和样本文本");
            } else {
                result = CommonTools::matchRegularExpression(parts.at(0), parts.at(1), &error);
            }
        } else if (title == QStringLiteral("Cron 表达式")) {
            result = CommonTools::describeCron(input->toPlainText(), &error);
        } else if (title == QStringLiteral("时间戳")) {
            result = CommonTools::timestampToLocalText(input->toPlainText(), &error);
        } else if (title == QStringLiteral("Hex 转字符串")) {
            result = CommonTools::hexToString(input->toPlainText(), &error);
        } else if (title == QStringLiteral("契约 Mock 数据")) {
            result = CommonTools::mockFromJsonExample(input->toPlainText(), &error);
        } else if (title == QStringLiteral("数据采样脱敏")) {
            result = CommonTools::maskSensitiveText(input->toPlainText());
        }
        setOutput(output, message, result, error);
    });

    if (secondaryButton != nullptr) {
        connect(secondaryButton, &QPushButton::clicked, this, [this, input, output, message]() {
            QString error;
            const QString result = CommonTools::base64ToText(input->toPlainText(), &error);
            setOutput(output, message, result, error);
        });
    }

    if (enableAiAssist && aiAssistButton != nullptr && aiStopButton != nullptr) {
        wireAiAssist(page,
                     aiAssistButton,
                     aiStopButton,
                     output,
                     message,
                     [input]() { return input->toPlainText(); },
                     aiSystemPrompt);
    }

    return page;
}

QWidget *CommonToolsWidget::buildJsonViewerPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *toolbar = new QWidget(page);
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

    auto *body = new QWidget(page);
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

    auto *message = new QLabel(page);
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

    wireAiAssist(page,
                 aiAssistButton,
                 aiStopButton,
                 aiOutput,
                 message,
                 [input]() { return input->toPlainText(); },
                 QStringLiteral("You fix and format invalid JSON. Reply with valid JSON only, no markdown or explanation."),
                 [rightPanel]() { rightPanel->setCurrentIndex(1); });

    return page;
}

QWidget *CommonToolsWidget::buildRegexPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *pattern = new QLineEdit(page);
    pattern->setPlaceholderText(QStringLiteral("(deploy)-(?<id>\\d+)"));
    PageLayout::configureFormInput(pattern);
    layout->addWidget(pattern);

    auto *options = new QWidget(page);
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

    auto *input = makeEditor(QStringLiteral("deploy-42 deploy-7"), page);
    auto *result = new QTreeWidget(page);
    result->setColumnCount(3);
    result->setHeaderLabels({QStringLiteral("匹配 / 分组"), QStringLiteral("内容"), QStringLiteral("位置")});
    result->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    result->header()->setStretchLastSection(true);
    layout->addWidget(input, 1);
    layout->addWidget(result, 1);

    auto *message = new QLabel(page);
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

    return page;
}

QWidget *CommonToolsWidget::buildTimestampPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *nowRow = new QWidget(page);
    auto *nowLayout = new QHBoxLayout(nowRow);
    nowLayout->setContentsMargins(0, 0, 0, 0);
    nowLayout->setSpacing(PageLayout::Space8);
    auto *nowLabel = new QLabel(nowRow);
    nowLabel->setObjectName(QStringLiteral("sectionLabel"));
    auto *copyNow = makeActionButton(QStringLiteral("复制当前毫秒"), nowRow);
    nowLayout->addWidget(nowLabel, 1);
    nowLayout->addWidget(copyNow);
    layout->addWidget(nowRow);

    auto *nowTimer = new QTimer(page);
    nowTimer->setInterval(1000);
    auto updateNow = [nowLabel]() {
        const qint64 ms = CommonTools::currentTimestampMs();
        nowLabel->setText(QStringLiteral("当前：%1 ms / %2 s / %3")
            .arg(ms)
            .arg(ms / 1000)
            .arg(CommonTools::timestampToDateText(ms, true, QString())));
    };
    connect(nowTimer, &QTimer::timeout, page, updateNow);
    nowTimer->start();
    updateNow();
    connect(copyNow, &QPushButton::clicked, page, []() {
        copyTextToClipboard(QString::number(CommonTools::currentTimestampMs()));
    });

    auto *form = new QFormLayout;
    PageLayout::applyInlineForm(form);

    auto *unit = new QComboBox(page);
    unit->addItems({QStringLiteral("毫秒"), QStringLiteral("秒")});
    PageLayout::configureFormInput(unit);
    form->addRow(QStringLiteral("时间戳单位"), unit);

    auto *format = new QLineEdit(page);
    format->setText(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    PageLayout::configureFormInput(format);
    form->addRow(QStringLiteral("时间格式"), format);

    auto *tsInput = new QLineEdit(page);
    tsInput->setText(QString::number(CommonTools::currentTimestampMs()));
    PageLayout::configureFormInput(tsInput);
    auto *toDate = makeActionButton(QStringLiteral("时间戳 → 时间"), page);
    auto *tsRow = new QWidget(page);
    auto *tsRowLayout = new QHBoxLayout(tsRow);
    tsRowLayout->setContentsMargins(0, 0, 0, 0);
    tsRowLayout->setSpacing(PageLayout::Space8);
    tsRowLayout->addWidget(tsInput, 1);
    tsRowLayout->addWidget(toDate);
    form->addRow(QStringLiteral("时间戳"), tsRow);

    auto *dateInput = new QLineEdit(page);
    dateInput->setText(CommonTools::timestampToDateText(CommonTools::currentTimestampMs(), true, QString()));
    PageLayout::configureFormInput(dateInput);
    auto *toTs = makeActionButton(QStringLiteral("时间 → 时间戳"), page);
    auto *dateRow = new QWidget(page);
    auto *dateRowLayout = new QHBoxLayout(dateRow);
    dateRowLayout->setContentsMargins(0, 0, 0, 0);
    dateRowLayout->setSpacing(PageLayout::Space8);
    dateRowLayout->addWidget(dateInput, 1);
    dateRowLayout->addWidget(toTs);
    form->addRow(QStringLiteral("时间"), dateRow);

    layout->addLayout(form);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    message->setWordWrap(true);
    layout->addWidget(message);
    layout->addStretch();

    connect(toDate, &QPushButton::clicked, page, [tsInput, unit, format, dateInput, message]() {
        bool ok = false;
        const qint64 raw = tsInput->text().trimmed().toLongLong(&ok);
        if (!ok) {
            message->setText(QStringLiteral("时间戳必须为整数"));
            return;
        }
        const bool ms = unit->currentIndex() == 0;
        const QString text = CommonTools::timestampToDateText(raw, ms, format->text());
        dateInput->setText(text);
        message->setText(QStringLiteral("已转换为时间：%1").arg(text));
    });

    connect(toTs, &QPushButton::clicked, page, [dateInput, unit, format, tsInput, message]() {
        QString error;
        const qint64 ms = CommonTools::dateTextToTimestampMs(dateInput->text(), format->text(), &error);
        if (!error.isEmpty()) {
            message->setText(error);
            return;
        }
        const bool useMs = unit->currentIndex() == 0;
        const qint64 value = useMs ? ms : ms / 1000;
        tsInput->setText(QString::number(value));
        message->setText(QStringLiteral("已转换为时间戳：%1 %2").arg(value).arg(useMs ? QStringLiteral("ms") : QStringLiteral("s")));
    });

    return page;
}

QWidget *CommonToolsWidget::buildHtmlEntityPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *table = new QTableWidget(page);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QStringLiteral("实体"), QStringLiteral("字符"), QStringLiteral("说明")});
    PageLayout::configureDataTable(table);
    const auto rows = CommonTools::htmlEntityRows();
    table->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(rows.at(row).code));
        table->setItem(row, 1, new QTableWidgetItem(rows.at(row).label));
        table->setItem(row, 2, new QTableWidgetItem(rows.at(row).note));
    }
    layout->addWidget(table, 1);

    auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *encodeButton = makeActionButton(QStringLiteral("编码"), toolbar);
    auto *decodeButton = makeActionButton(QStringLiteral("解码"), toolbar);
    auto *copyButton = makeActionButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(encodeButton);
    toolbarLayout->addWidget(decodeButton);
    toolbarLayout->addWidget(copyButton);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(QStringLiteral("<a href=\"x\">链接 & 文本</a>"), editors);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    connect(table, &QTableWidget::cellDoubleClicked, table, [table](int row, int) {
        if (QTableWidgetItem *item = table->item(row, 0)) {
            copyTextToClipboard(item->text());
        }
    });
    connect(encodeButton, &QPushButton::clicked, page, [input, output]() {
        output->setPlainText(CommonTools::htmlEncode(input->toPlainText()));
    });
    connect(decodeButton, &QPushButton::clicked, page, [input, output]() {
        output->setPlainText(CommonTools::htmlDecode(input->toPlainText()));
    });
    connect(copyButton, &QPushButton::clicked, page, [output]() {
        copyTextToClipboard(output->toPlainText());
    });

    return page;
}

QWidget *CommonToolsWidget::buildHttpStatusPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *search = new QLineEdit(page);
    search->setPlaceholderText(QStringLiteral("如 404 或 Not Found"));
    search->setClearButtonEnabled(true);
    PageLayout::configureFormInput(search);
    layout->addWidget(search);

    auto *table = new QTableWidget(page);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({QStringLiteral("编码"), QStringLiteral("含义"), QStringLiteral("说明")});
    PageLayout::configureDataTable(table);
    const auto rows = CommonTools::httpStatusRows();
    table->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        table->setItem(row, 0, new QTableWidgetItem(rows.at(row).code));
        table->setItem(row, 1, new QTableWidgetItem(rows.at(row).label));
        table->setItem(row, 2, new QTableWidgetItem(rows.at(row).note));
    }
    layout->addWidget(table, 1);

    connect(search, &QLineEdit::textChanged, table, [table](const QString &keyword) {
        const QString needle = keyword.trimmed();
        for (int row = 0; row < table->rowCount(); ++row) {
            bool match = needle.isEmpty();
            for (int col = 0; col < table->columnCount() && !match; ++col) {
                if (QTableWidgetItem *item = table->item(row, col)) {
                    match = item->text().contains(needle, Qt::CaseInsensitive);
                }
            }
            table->setRowHidden(row, !match);
        }
    });
    connect(table, &QTableWidget::cellDoubleClicked, table, [table](int row, int) {
        const QTableWidgetItem *code = table->item(row, 0);
        const QTableWidgetItem *label = table->item(row, 1);
        if (code != nullptr && label != nullptr) {
            copyTextToClipboard(QStringLiteral("%1 %2").arg(code->text(), label->text()));
        }
    });

    return page;
}

QWidget *CommonToolsWidget::buildDiffPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *openLeft = makeActionButton(QStringLiteral("选择源文件"), toolbar);
    auto *openRight = makeActionButton(QStringLiteral("选择对比文件"), toolbar);
    auto *swap = makeActionButton(QStringLiteral("交换两侧"), toolbar);
    auto *copyLeft = makeActionButton(QStringLiteral("复制源"), toolbar);
    auto *copyRight = makeActionButton(QStringLiteral("复制对比"), toolbar);
    auto *aiAssistButton = makeActionButton(QStringLiteral("AI 辅助"), toolbar);
    auto *aiStopButton = makeActionButton(QStringLiteral("停止"), toolbar);
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

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *left = makeEditor(QStringLiteral("源文本，可粘贴或选择文件"), editors);
    auto *right = makeEditor(QStringLiteral("对比文本，可粘贴或选择文件"), editors);
    editorsLayout->addWidget(left, 1);
    editorsLayout->addWidget(right, 1);
    layout->addWidget(editors, 1);

    auto *aiOutput = makeEditor(QStringLiteral("AI 差异摘要"), page);
    aiOutput->setReadOnly(true);
    aiOutput->setMaximumHeight(180);
    layout->addWidget(aiOutput);

    auto *status = new QLabel(page);
    status->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(status);

    auto applyHighlights = [](QPlainTextEdit *editor, const QVector<int> &lines) {
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
    };

    auto recompute = [left, right, status, applyHighlights]() {
        const CommonTools::LineDiff diff =
            CommonTools::diffLineIndices(left->toPlainText(), right->toPlainText());
        applyHighlights(left, diff.leftChangedLines);
        applyHighlights(right, diff.rightChangedLines);
        status->setText(QStringLiteral("差异：源 %1 行，对比 %2 行；相同 %3 行")
            .arg(diff.leftChangedLines.size())
            .arg(diff.rightChangedLines.size())
            .arg(diff.sameCount));
    };

    connect(left, &QPlainTextEdit::textChanged, page, recompute);
    connect(right, &QPlainTextEdit::textChanged, page, recompute);
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
    connect(swap, &QPushButton::clicked, page, [left, right]() {
        const QString leftText = left->toPlainText();
        left->setPlainText(right->toPlainText());
        right->setPlainText(leftText);
    });
    connect(copyLeft, &QPushButton::clicked, page, [left]() {
        copyTextToClipboard(left->toPlainText());
    });
    connect(copyRight, &QPushButton::clicked, page, [right]() {
        copyTextToClipboard(right->toPlainText());
    });

    wireAiAssist(page,
                 aiAssistButton,
                 aiStopButton,
                 aiOutput,
                 status,
                 [left, right]() {
        return QStringLiteral("Source:\n%1\n\nTarget:\n%2").arg(left->toPlainText(), right->toPlainText());
    },
                 QStringLiteral("Summarize text differences in concise Chinese. Focus on what changed, added, or removed."));

    return page;
}

QWidget *CommonToolsWidget::buildCronPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *exprRow = new QWidget(page);
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

    auto *builder = new QWidget(page);
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

    auto *presets = new QWidget(page);
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

    auto *description = new QLabel(page);
    description->setObjectName(QStringLiteral("sectionLabel"));
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *runs = new QListWidget(page);
    layout->addWidget(runs, 1);

    auto *aiOutput = makeEditor(QStringLiteral("AI 解释"), page);
    aiOutput->setReadOnly(true);
    aiOutput->setMaximumHeight(160);
    layout->addWidget(aiOutput);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(composeButton, &QPushButton::clicked, page, [expr, fieldEdits]() {
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

    connect(parseButton, &QPushButton::clicked, page, [expr, description, runs, message]() {
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

    wireAiAssist(page,
                 aiAssistButton,
                 aiStopButton,
                 aiOutput,
                 message,
                 [expr]() { return expr->text(); },
                 QStringLiteral("Explain the cron expression in concise Chinese, including field meanings and typical schedule."));

    return page;
}

QWidget *CommonToolsWidget::buildImageBase64Page()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
struct ImageState {
        QByteArray bytes;
        QString mime;
    };
    auto state = QSharedPointer<ImageState>::create();

    auto *urlRow = new QWidget(page);
    auto *urlLayout = new QHBoxLayout(urlRow);
    urlLayout->setContentsMargins(0, 0, 0, 0);
    urlLayout->setSpacing(PageLayout::Space8);
    auto *pickButton = makeActionButton(QStringLiteral("选择本地图片"), urlRow);
    auto *urlEdit = new QLineEdit(urlRow);
    urlEdit->setPlaceholderText(QStringLiteral("https://example.com/image.png"));
    PageLayout::configureFormInput(urlEdit);
    auto *urlButton = makeActionButton(QStringLiteral("从 URL 加载"), urlRow);
    urlLayout->addWidget(pickButton);
    urlLayout->addWidget(urlEdit, 1);
    urlLayout->addWidget(urlButton);
    layout->addWidget(urlRow);

    auto *actions = new QWidget(page);
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(PageLayout::Space8);
    auto *toBase64 = makeActionButton(QStringLiteral("图片 → Base64"), actions);
    auto *toImage = makeActionButton(QStringLiteral("Base64 → 图片"), actions);
    auto *copyButton = makeActionButton(QStringLiteral("复制 Base64"), actions);
    auto *clearButton = makeActionButton(QStringLiteral("清空"), actions);
    actionsLayout->addWidget(toBase64);
    actionsLayout->addWidget(toImage);
    actionsLayout->addWidget(copyButton);
    actionsLayout->addWidget(clearButton);
    actionsLayout->addStretch();
    layout->addWidget(actions);

    auto *body = new QWidget(page);
    auto *bodyLayout = new QHBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space12);

    auto *base64Edit = new QPlainTextEdit(body);
    base64Edit->setPlaceholderText(QStringLiteral("Base64 文本（可含 data:image/...;base64, 前缀）"));
    base64Edit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    base64Edit->setMinimumHeight(260);

    auto *previewArea = new QScrollArea(body);
    previewArea->setWidgetResizable(true);
    auto *preview = new QLabel(previewArea);
    preview->setAlignment(Qt::AlignCenter);
    preview->setText(QStringLiteral("图片预览"));
    previewArea->setWidget(preview);

    bodyLayout->addWidget(base64Edit, 1);
    bodyLayout->addWidget(previewArea, 1);
    layout->addWidget(body, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    auto mimeForPath = [](const QString &path) -> QString {
        const QString lower = path.toLower();
        if (lower.endsWith(QStringLiteral(".jpg")) || lower.endsWith(QStringLiteral(".jpeg"))) {
            return QStringLiteral("image/jpeg");
        }
        if (lower.endsWith(QStringLiteral(".gif"))) {
            return QStringLiteral("image/gif");
        }
        if (lower.endsWith(QStringLiteral(".bmp"))) {
            return QStringLiteral("image/bmp");
        }
        if (lower.endsWith(QStringLiteral(".webp"))) {
            return QStringLiteral("image/webp");
        }
        return QStringLiteral("image/png");
    };

    auto showImage = [preview, message](const QByteArray &bytes) -> bool {
        QImage image;
        if (!image.loadFromData(bytes)) {
            message->setText(QStringLiteral("无法识别图片数据"));
            return false;
        }
        QPixmap pixmap = QPixmap::fromImage(image);
        if (pixmap.width() > 520 || pixmap.height() > 360) {
            pixmap = pixmap.scaled(520, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        preview->setPixmap(pixmap);
        return true;
    };

    connect(pickButton, &QPushButton::clicked, this,
            [this, state, showImage, mimeForPath, message](){
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择图片"), QString(),
            QStringLiteral("图片 (*.png *.jpg *.jpeg *.gif *.bmp *.webp)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            message->setText(QStringLiteral("无法读取文件"));
            return;
        }
        const QByteArray bytes = file.readAll();
        if (showImage(bytes)) {
            state->bytes = bytes;
            state->mime = mimeForPath(path);
            message->setText(QStringLiteral("已加载本地图片（%1 字节）").arg(bytes.size()));
        }
    });

    connect(urlButton, &QPushButton::clicked, this,
            [this, state, showImage, urlEdit, message](){
        const QUrl url(urlEdit->text().trimmed());
        if (!url.isValid() || url.scheme().isEmpty()) {
            message->setText(QStringLiteral("URL 无效"));
            return;
        }
        auto *manager = new QNetworkAccessManager(this);
        QNetworkRequest request(url);
        QNetworkReply *reply = manager->get(request);
        message->setText(QStringLiteral("正在下载图片..."));
        QObject::connect(reply, &QNetworkReply::finished, this,
                         [reply, manager, state, showImage, message]() {
            if (reply->error() != QNetworkReply::NoError) {
                message->setText(QStringLiteral("下载失败：%1").arg(reply->errorString()));
            } else {
                const QByteArray bytes = reply->readAll();
                const QString contentType =
                    reply->header(QNetworkRequest::ContentTypeHeader).toString();
                if (showImage(bytes)) {
                    state->bytes = bytes;
                    state->mime = contentType.isEmpty() ? QStringLiteral("image/png")
                                                        : contentType.section(QLatin1Char(';'), 0, 0);
                    message->setText(QStringLiteral("已加载网络图片（%1 字节）").arg(bytes.size()));
                }
            }
            reply->deleteLater();
            manager->deleteLater();
        });
    });

    connect(toBase64, &QPushButton::clicked, page, [state, base64Edit, message]() {
        if (state->bytes.isEmpty()) {
            message->setText(QStringLiteral("请先加载图片"));
            return;
        }
        const QString mime = state->mime.isEmpty() ? QStringLiteral("image/png") : state->mime;
        const QString encoded = QStringLiteral("data:%1;base64,%2")
            .arg(mime, QString::fromLatin1(state->bytes.toBase64()));
        base64Edit->setPlainText(encoded);
        message->setText(QStringLiteral("已转换为 Base64"));
    });

    connect(toImage, &QPushButton::clicked, page, [state, base64Edit, showImage, message]() {
        QString text = base64Edit->toPlainText().trimmed();
        const int comma = text.indexOf(QLatin1Char(','));
        if (text.startsWith(QStringLiteral("data:")) && comma >= 0) {
            text = text.mid(comma + 1);
        }
        text.remove(QLatin1Char('\n'));
        text.remove(QLatin1Char('\r'));
        text.remove(QLatin1Char(' '));
        const QByteArray bytes = QByteArray::fromBase64(text.toLatin1());
        if (bytes.isEmpty()) {
            message->setText(QStringLiteral("Base64 内容为空或无效"));
            return;
        }
        if (showImage(bytes)) {
            state->bytes = bytes;
            message->setText(QStringLiteral("已还原为图片"));
        }
    });

    connect(copyButton, &QPushButton::clicked, page, [base64Edit]() {
        copyTextToClipboard(base64Edit->toPlainText());
    });
    connect(clearButton, &QPushButton::clicked, page, [state, base64Edit, preview, message]() {
        state->bytes.clear();
        state->mime.clear();
        base64Edit->clear();
        preview->setText(QStringLiteral("图片预览"));
        preview->setPixmap(QPixmap());
        message->clear();
    });

    return page;
}

QWidget *CommonToolsWidget::buildUuidPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *options = new QWidget(page);
    auto *optionsLayout = new QHBoxLayout(options);
    optionsLayout->setContentsMargins(0, 0, 0, 0);
    optionsLayout->setSpacing(PageLayout::Space8);
    optionsLayout->addWidget(new QLabel(QStringLiteral("数量"), options));
    auto *count = new QSpinBox(options);
    count->setRange(1, 1000);
    count->setValue(5);
    PageLayout::configureFormInput(count);
    auto *uppercase = new QCheckBox(QStringLiteral("大写"), options);
    auto *noDash = new QCheckBox(QStringLiteral("去横线"), options);
    auto *generate = makeActionButton(QStringLiteral("生成"), options);
    auto *copy = makeActionButton(QStringLiteral("复制全部"), options);
    optionsLayout->addWidget(count);
    optionsLayout->addWidget(uppercase);
    optionsLayout->addWidget(noDash);
    optionsLayout->addWidget(generate);
    optionsLayout->addWidget(copy);
    optionsLayout->addStretch();
    layout->addWidget(options);

    auto *output = makeEditor(QStringLiteral("输出"), page);
    output->setReadOnly(true);
    layout->addWidget(output, 1);

    connect(generate, &QPushButton::clicked, page, [count, uppercase, noDash, output]() {
        const QStringList ids = CommonTools::generateUuids(
            count->value(), uppercase->isChecked(), noDash->isChecked());
        output->setPlainText(ids.join(QLatin1Char('\n')));
    });
    connect(copy, &QPushButton::clicked, page, [output]() {
        copyTextToClipboard(output->toPlainText());
    });

    return page;
}

QWidget *CommonToolsWidget::buildHashPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *input = makeEditor(QStringLiteral("待计算文本"), page);
    layout->addWidget(input, 1);

    auto *compute = makeActionButton(QStringLiteral("计算哈希"), page);
    layout->addWidget(compute, 0, Qt::AlignLeft);

    auto *form = new QFormLayout;
    PageLayout::applyInlineForm(form);
    QLineEdit *md5 = addResultRow(form, QStringLiteral("MD5"), page);
    QLineEdit *sha1 = addResultRow(form, QStringLiteral("SHA1"), page);
    QLineEdit *sha256 = addResultRow(form, QStringLiteral("SHA256"), page);
    QLineEdit *sha512 = addResultRow(form, QStringLiteral("SHA512"), page);
    layout->addLayout(form);
    layout->addStretch();

    connect(compute, &QPushButton::clicked, page, [input, md5, sha1, sha256, sha512]() {
        const CommonTools::HashResult result =
            CommonTools::computeHashes(input->toPlainText().toUtf8());
        md5->setText(result.md5);
        sha1->setText(result.sha1);
        sha256->setText(result.sha256);
        sha512->setText(result.sha512);
    });

    return page;
}

QWidget *CommonToolsWidget::buildUrlCodecPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *encode = makeActionButton(QStringLiteral("编码"), toolbar);
    auto *decode = makeActionButton(QStringLiteral("解码"), toolbar);
    auto *copy = makeActionButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(encode);
    toolbarLayout->addWidget(decode);
    toolbarLayout->addWidget(copy);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(QStringLiteral("原文 / 编码后文本"), editors);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    connect(encode, &QPushButton::clicked, page, [input, output]() {
        output->setPlainText(CommonTools::urlEncode(input->toPlainText()));
    });
    connect(decode, &QPushButton::clicked, page, [input, output]() {
        output->setPlainText(CommonTools::urlDecode(input->toPlainText()));
    });
    connect(copy, &QPushButton::clicked, page, [output]() {
        copyTextToClipboard(output->toPlainText());
    });

    return page;
}

QWidget *CommonToolsWidget::buildBase64TextPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *encode = makeActionButton(QStringLiteral("文本 → Base64"), toolbar);
    auto *decode = makeActionButton(QStringLiteral("Base64 → 文本"), toolbar);
    auto *copy = makeActionButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(encode);
    toolbarLayout->addWidget(decode);
    toolbarLayout->addWidget(copy);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *editors = new QWidget(page);
    auto *editorsLayout = new QHBoxLayout(editors);
    editorsLayout->setContentsMargins(0, 0, 0, 0);
    editorsLayout->setSpacing(PageLayout::Space12);
    auto *input = makeEditor(QStringLiteral("原文 / Base64"), editors);
    input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *output = makeEditor(QStringLiteral("输出"), editors);
    output->setReadOnly(true);
    output->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editorsLayout->addWidget(input, 1);
    editorsLayout->addWidget(output, 1);
    layout->addWidget(editors, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(encode, &QPushButton::clicked, page, [input, output, message]() {
        output->setPlainText(CommonTools::textToBase64(input->toPlainText()));
        message->setText(QStringLiteral("已编码"));
    });
    connect(decode, &QPushButton::clicked, page, [input, output, message]() {
        QString error;
        const QString text = CommonTools::base64ToText(input->toPlainText(), &error);
        if (!error.isEmpty()) {
            message->setText(error);
            return;
        }
        output->setPlainText(text);
        message->setText(QStringLiteral("已解码"));
    });
    connect(copy, &QPushButton::clicked, page, [output]() {
        copyTextToClipboard(output->toPlainText());
    });

    return page;
}

QWidget *CommonToolsWidget::buildJwtPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *toolbar = new QWidget(page);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(PageLayout::Space8);
    auto *parse = makeActionButton(QStringLiteral("解析"), toolbar);
    auto *copy = makeActionButton(QStringLiteral("复制结果"), toolbar);
    toolbarLayout->addWidget(parse);
    toolbarLayout->addWidget(copy);
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    auto *input = makeEditor(QStringLiteral("粘贴 JWT (xxxxx.yyyyy.zzzzz)"), page);
    input->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto *output = makeEditor(QStringLiteral("解析结果"), page);
    output->setReadOnly(true);
    layout->addWidget(input, 1);
    layout->addWidget(output, 1);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);

    connect(parse, &QPushButton::clicked, page, [input, output, message]() {
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
    connect(copy, &QPushButton::clicked, page, [output]() {
        copyTextToClipboard(output->toPlainText());
    });

    return page;
}

QWidget *CommonToolsWidget::buildNumberBasePage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *inputRow = new QWidget(page);
    auto *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(PageLayout::Space8);
    auto *input = new QLineEdit(inputRow);
    input->setPlaceholderText(QStringLiteral("输入数字"));
    PageLayout::configureFormInput(input);
    auto *fromBase = new QComboBox(inputRow);
    fromBase->addItem(QStringLiteral("十进制"), 10);
    fromBase->addItem(QStringLiteral("二进制"), 2);
    fromBase->addItem(QStringLiteral("八进制"), 8);
    fromBase->addItem(QStringLiteral("十六进制"), 16);
    PageLayout::configureFormInput(fromBase);
    auto *convert = makeActionButton(QStringLiteral("转换"), inputRow);
    inputLayout->addWidget(input, 1);
    inputLayout->addWidget(fromBase);
    inputLayout->addWidget(convert);
    layout->addWidget(inputRow);

    auto *form = new QFormLayout;
    PageLayout::applyInlineForm(form);
    QLineEdit *binary = addResultRow(form, QStringLiteral("二进制"), page);
    QLineEdit *octal = addResultRow(form, QStringLiteral("八进制"), page);
    QLineEdit *decimal = addResultRow(form, QStringLiteral("十进制"), page);
    QLineEdit *hex = addResultRow(form, QStringLiteral("十六进制"), page);
    layout->addLayout(form);

    auto *message = new QLabel(page);
    message->setObjectName(QStringLiteral("toolMessage"));
    layout->addWidget(message);
    layout->addStretch();

    connect(convert, &QPushButton::clicked, page,
            [input, fromBase, binary, octal, decimal, hex, message]() {
        QString error;
        const int base = fromBase->currentData().toInt();
        const CommonTools::NumberBases bases =
            CommonTools::convertNumberBase(input->text(), base, &error);
        if (!bases.valid) {
            message->setText(error);
            return;
        }
        binary->setText(bases.binary);
        octal->setText(bases.octal);
        decimal->setText(bases.decimal);
        hex->setText(bases.hex);
        message->setText(QStringLiteral("已转换"));
    });

    return page;
}

QWidget *CommonToolsWidget::buildCasePage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
auto *input = new QLineEdit(page);
    input->setPlaceholderText(QStringLiteral("如 deploy hub service 或 deployHubService"));
    PageLayout::configureFormInput(input);
    layout->addWidget(input);

    auto *convert = makeActionButton(QStringLiteral("转换"), page);
    layout->addWidget(convert, 0, Qt::AlignLeft);

    auto *form = new QFormLayout;
    PageLayout::applyInlineForm(form);
    QLineEdit *camel = addResultRow(form, QStringLiteral("camelCase"), page);
    QLineEdit *pascal = addResultRow(form, QStringLiteral("PascalCase"), page);
    QLineEdit *snake = addResultRow(form, QStringLiteral("snake_case"), page);
    QLineEdit *kebab = addResultRow(form, QStringLiteral("kebab-case"), page);
    QLineEdit *constantCase = addResultRow(form, QStringLiteral("CONSTANT_CASE"), page);
    QLineEdit *title = addResultRow(form, QStringLiteral("Title Case"), page);
    layout->addLayout(form);
    layout->addStretch();

    auto doConvert = [input, camel, pascal, snake, kebab, constantCase, title]() {
        const CommonTools::CaseForms forms = CommonTools::convertCase(input->text());
        camel->setText(forms.camel);
        pascal->setText(forms.pascal);
        snake->setText(forms.snake);
        kebab->setText(forms.kebab);
        constantCase->setText(forms.constantCase);
        title->setText(forms.title);
    };
    connect(convert, &QPushButton::clicked, page, doConvert);
    connect(input, &QLineEdit::returnPressed, page, doConvert);

    return page;
}

QWidget *CommonToolsWidget::buildHttpRequestPage()
{
    return new HttpRequestWidget(this);
}

QWidget *CommonToolsWidget::buildWebSocketPage()
{
    return new WebSocketToolWidget(this);
}

void CommonToolsWidget::setOutput(QPlainTextEdit *output, QLabel *message, const QString &text, const QString &error)
{
    if (!error.isEmpty()) {
        output->clear();
        message->setText(error);
        return;
    }
    output->setPlainText(text);
    message->setText(QStringLiteral("已完成"));
}

bool CommonToolsWidget::resolveAiCredentials(AiSettings *settings, QString *apiKey, QString *error) const
{
    return AiAssistHelper::resolveCredentials(m_aiSettings, m_credentials, settings, apiKey, error);
}

void CommonToolsWidget::wireAiAssist(QWidget *page,
                                     QPushButton *assistButton,
                                     QPushButton *stopButton,
                                     QPlainTextEdit *output,
                                     QLabel *message,
                                     const std::function<QString()> &userContentProvider,
                                     const QString &systemPrompt,
                                     const std::function<void()> &onStart)
{
    auto *client = new OpenAiChatClient(page);
    auto *buffer = new AiStreamBuffer(output, page);

    auto setBusy = [assistButton, stopButton](bool busy) {
        assistButton->setEnabled(!busy);
        stopButton->setEnabled(busy);
    };

    connect(client, &OpenAiChatClient::deltaReceived, page, [buffer](const QString &chunk) {
        buffer->append(chunk);
    });
    connect(client, &OpenAiChatClient::finished, page, [message, buffer, setBusy]() {
        buffer->flush();
        message->setText(QStringLiteral("AI 已完成"));
        setBusy(false);
    });
    connect(client, &OpenAiChatClient::failed, page, [message, buffer, setBusy](const QString &failedMessage) {
        buffer->flush();
        message->setText(QStringLiteral("AI 失败：%1").arg(failedMessage));
        setBusy(false);
    });

    connect(assistButton, &QPushButton::clicked, page, [this, client, output, message, buffer, userContentProvider, systemPrompt, onStart, setBusy]() {
        const QString userContent = userContentProvider().trimmed();
        if (userContent.isEmpty()) {
            message->setText(QStringLiteral("请先输入内容"));
            return;
        }

        AiSettings settings;
        QString apiKey;
        QString error;
        if (!resolveAiCredentials(&settings, &apiKey, &error)) {
            message->setText(error);
            return;
        }

        if (onStart) {
            onStart();
        }

        client->abort();
        buffer->reset();
        output->clear();
        setBusy(true);
        message->setText(QStringLiteral("AI 生成中..."));

        const QJsonArray messages{
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("system")},
                {QStringLiteral("content"), systemPrompt}
            },
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("user")},
                {QStringLiteral("content"), userContent}
            }
        };
        client->streamChat(settings.apiBaseUrl, apiKey, settings.model, messages);
    });

    connect(stopButton, &QPushButton::clicked, page, [client, message, buffer, setBusy]() {
        client->abort();
        buffer->flush();
        message->setText(QStringLiteral("已停止"));
        setBusy(false);
    });
}
