#include "ui/ServiceElasticsearchIndexStructureDialog.h"

#include "adapters/services/ElasticsearchServiceClient.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

ServiceElasticsearchIndexStructureDialog::ServiceElasticsearchIndexStructureDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("索引表结构"));
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *metaCard = new QFrame(this);
    metaCard->setObjectName(QStringLiteral("dialogFormPanel"));
    metaCard->setAttribute(Qt::WA_StyledBackground, true);
    auto *metaLayout = new QVBoxLayout(metaCard);
    metaLayout->setContentsMargins(0, 0, 0, 0);
    metaLayout->setSpacing(0);

    auto *metaHeader = new QWidget(metaCard);
    metaHeader->setObjectName(QStringLiteral("dialogFormPanelHeader"));
    metaHeader->setAttribute(Qt::WA_StyledBackground, true);
    auto *metaHeaderLayout = new QHBoxLayout(metaHeader);
    metaHeaderLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space12, PageLayout::Space16, PageLayout::Space12);
    metaHeaderLayout->setSpacing(0);
    auto *metaTitle = new QLabel(QStringLiteral("索引元信息"), metaHeader);
    metaTitle->setObjectName(QStringLiteral("dialogFormPanelTitle"));
    metaHeaderLayout->addWidget(metaTitle);
    metaHeaderLayout->addStretch();
    metaLayout->addWidget(metaHeader);

    auto *metaBody = new QWidget(metaCard);
    auto *metaBodyLayout = new QHBoxLayout(metaBody);
    metaBodyLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space12, PageLayout::Space16, PageLayout::Space12);
    metaBodyLayout->setSpacing(PageLayout::Space8);
    m_metaLabel = new QLabel(QStringLiteral("基本信息"), metaBody);
    m_metaLabel->setObjectName(QStringLiteral("dialogFormValueLabel"));
    m_metaValue = new QTextEdit(metaBody);
    m_metaValue->setObjectName(QStringLiteral("dialogFormValueText"));
    m_metaValue->setReadOnly(true);
    m_metaValue->setFrameShape(QFrame::NoFrame);
    m_metaValue->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_metaValue->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_metaValue->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_metaValue->setFixedHeight(28);
    m_metaValue->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_metaValue->setFocusPolicy(Qt::ClickFocus);
    metaBodyLayout->addWidget(m_metaLabel);
    metaBodyLayout->addWidget(m_metaValue, 1);
    metaLayout->addWidget(metaBody);

    layout->addWidget(metaCard);

    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs, 1);

    m_mappingView = new QPlainTextEdit(m_tabs);
    m_mappingView->setObjectName(QStringLiteral("deployLog"));
    m_mappingView->setReadOnly(true);
    m_mappingView->setPlaceholderText(QStringLiteral("加载 mapping 中…"));
    m_tabs->addTab(m_mappingView, QStringLiteral("Mapping"));

    m_settingsView = new QPlainTextEdit(m_tabs);
    m_settingsView->setObjectName(QStringLiteral("deployLog"));
    m_settingsView->setReadOnly(true);
    m_settingsView->setPlaceholderText(QStringLiteral("加载 settings 中…"));
    m_tabs->addTab(m_settingsView, QStringLiteral("Settings"));

    m_aliasesTable = new QTableWidget(m_tabs);
    m_aliasesTable->setColumnCount(3);
    m_aliasesTable->setHorizontalHeaderLabels({QStringLiteral("别名"), QStringLiteral("指向索引"), QStringLiteral("是否为写索引")});
    m_aliasesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_aliasesTable->horizontalHeader()->setStretchLastSection(true);
    m_aliasesTable->verticalHeader()->setVisible(false);
    m_aliasesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_aliasesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_aliasesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabs->addTab(m_aliasesTable, QStringLiteral("Aliases"));

    m_templatesTable = new QTableWidget(m_tabs);
    m_templatesTable->setColumnCount(4);
    m_templatesTable->setHorizontalHeaderLabels(
        {QStringLiteral("模板名"), QStringLiteral("匹配模式"), QStringLiteral("优先级"), QStringLiteral("命中模式数")});
    m_templatesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_templatesTable->horizontalHeader()->setStretchLastSection(true);
    m_templatesTable->verticalHeader()->setVisible(false);
    m_templatesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_templatesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_templatesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tabs->addTab(m_templatesTable, QStringLiteral("Templates"));

    auto *analyzePanel = new QWidget(m_tabs);
    auto *analyzeLayout = new QVBoxLayout(analyzePanel);
    analyzeLayout->setContentsMargins(0, 0, 0, 0);
    analyzeLayout->setSpacing(PageLayout::Space8);

    auto *analyzerRow = new QHBoxLayout;
    analyzerRow->setSpacing(PageLayout::Space8);
    analyzerRow->addWidget(new QLabel(QStringLiteral("分词器"), analyzePanel));
    m_analyzerCombo = new QComboBox(analyzePanel);
    m_analyzerCombo->setMinimumWidth(220);
    m_analyzerCombo->setEditable(true);
    analyzerRow->addWidget(m_analyzerCombo, 1);

    m_analyzeButton = new QPushButton(QStringLiteral("分析"), analyzePanel);
    m_analyzeButton->setObjectName(QStringLiteral("primaryButton"));
    analyzerRow->addWidget(m_analyzeButton);
    analyzeLayout->addLayout(analyzerRow);

    m_analyzeTextInput = new QLineEdit(analyzePanel);
    m_analyzeTextInput->setPlaceholderText(QStringLiteral("输入要分词的文本，例如：The quick brown fox 123"));
    m_analyzeTextInput->setClearButtonEnabled(true);
    analyzeLayout->addWidget(m_analyzeTextInput);

    auto *infoTag = new QFrame(analyzePanel);
    infoTag->setObjectName(QStringLiteral("dialogInfoTag"));
    infoTag->setAttribute(Qt::WA_StyledBackground, true);
    infoTag->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    infoTag->setFixedHeight(28);
    auto *infoTagLayout = new QHBoxLayout(infoTag);
    infoTagLayout->setContentsMargins(PageLayout::Space8, 0, PageLayout::Space8, 0);
    infoTagLayout->setSpacing(PageLayout::Space6);
    auto *infoIcon = new QLabel(QStringLiteral("ⓘ"), infoTag);
    infoIcon->setObjectName(QStringLiteral("dialogInfoTagIcon"));
    infoTagLayout->addWidget(infoIcon, 0, Qt::AlignVCenter);
    m_analyzeHintLabel = new QLabel(infoTag);
    m_analyzeHintLabel->setObjectName(QStringLiteral("dialogInfoTagText"));
    m_analyzeHintLabel->setTextFormat(Qt::RichText);
    m_analyzeHintLabel->setWordWrap(false);
    m_analyzeHintLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_analyzeHintLabel->setFixedHeight(20);
    m_analyzeHintLabel->setText(QStringLiteral(
        "<span style=\"color:#0066CC;\">内置分词器</span>（<b>standard</b> / <b>simple</b> / <b>whitespace</b> / <b>keyword</b>）始终可用；"
        "索引自定义分词器与 <b>ik_max_word</b> / <b>ik_smart</b> 需要 ES 安装对应插件并重启。"));
    infoTagLayout->addWidget(m_analyzeHintLabel, 1, Qt::AlignVCenter);
    analyzeLayout->addWidget(infoTag);

    m_analyzeStatusLabel = new QLabel(analyzePanel);
    m_analyzeStatusLabel->setObjectName(QStringLiteral("mutedText"));
    m_analyzeStatusLabel->setWordWrap(false);
    m_analyzeStatusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_analyzeStatusLabel->setFixedHeight(18);
    m_analyzeStatusLabel->hide();
    analyzeLayout->addWidget(m_analyzeStatusLabel);

    m_tokensTable = new QTableWidget(analyzePanel);
    m_tokensTable->setColumnCount(5);
    m_tokensTable->setHorizontalHeaderLabels(
        {QStringLiteral("#"), QStringLiteral("Token"), QStringLiteral("起始偏移"), QStringLiteral("结束偏移"), QStringLiteral("类型")});
    m_tokensTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_tokensTable->horizontalHeader()->setStretchLastSection(true);
    m_tokensTable->verticalHeader()->setVisible(false);
    m_tokensTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tokensTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    analyzeLayout->addWidget(m_tokensTable, 1);

    m_tabs->addTab(analyzePanel, QStringLiteral("分词测试"));

    auto *actions = new QHBoxLayout;
    actions->setSpacing(PageLayout::Space8);
    m_refreshButton = new QPushButton(QStringLiteral("刷新"), this);
    m_refreshButton->setObjectName(QStringLiteral("secondaryButton"));
    m_copyButton = new QPushButton(QStringLiteral("复制当前标签"), this);
    m_copyButton->setObjectName(QStringLiteral("secondaryButton"));
    m_closeButton = new QPushButton(QStringLiteral("关闭"), this);
    actions->addWidget(m_refreshButton);
    actions->addWidget(m_copyButton);
    actions->addStretch();
    actions->addWidget(m_closeButton);
    layout->addLayout(actions);

    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        m_cachedSettings.clear();
        m_analyzers.clear();
        reloadTab(m_tabs->currentIndex());
    });
    connect(m_copyButton, &QPushButton::clicked, this, [this]() {
        QString text;
        switch (m_tabs->currentIndex()) {
        case TabMapping: text = m_mappingView->toPlainText(); break;
        case TabSettings: text = m_settingsView->toPlainText(); break;
        case TabAnalyze: {
            QStringList parts;
            if (m_analyzeStatusLabel != nullptr && m_analyzeStatusLabel->isVisible()) {
                parts << m_analyzeStatusLabel->text();
            }
            parts << QString::number(m_tokensTable->rowCount()) + QStringLiteral(" 个 token");
            text = parts.join(QStringLiteral("\n"));
            break;
        }
        default: {
            QTableWidget *current = m_tabs->currentIndex() == TabAliases ? m_aliasesTable : m_templatesTable;
            QStringList rows;
            rows << current->horizontalHeaderItem(0)->text();
            for (int r = 0; r < current->rowCount(); ++r) {
                QStringList cells;
                for (int c = 0; c < current->columnCount(); ++c) {
                    cells << (current->item(r, c) ? current->item(r, c)->text() : QString());
                }
                rows << cells.join(QChar(9));
            }
            text = rows.join(QChar('\n'));
        }
        }
        QGuiApplication::clipboard()->setText(text);
    });
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        reloadTab(index);
    });
    connect(m_analyzeButton, &QPushButton::clicked, this, &ServiceElasticsearchIndexStructureDialog::onAnalyzeText);
    connect(m_analyzeTextInput, &QLineEdit::returnPressed, this, &ServiceElasticsearchIndexStructureDialog::onAnalyzeText);

    PageLayout::applyModalDialog(this);
    resize(1100, 720);
}

void ServiceElasticsearchIndexStructureDialog::setEndpoint(const ServiceEndpoint &endpoint)
{
    m_endpoint = endpoint;
}

void ServiceElasticsearchIndexStructureDialog::setIndexName(const QString &index)
{
    m_index = index;
    m_metaValue->setPlainText(QStringLiteral("索引：%1    节点：%2:%3")
                                  .arg(index,
                                       m_endpoint.host,
                                       QString::number(m_endpoint.port)));
}

int ServiceElasticsearchIndexStructureDialog::exec()
{
    if (m_index.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法查看"), QStringLiteral("未提供索引名。"));
        return QDialog::Rejected;
    }
    reloadTab(m_tabs->currentIndex());
    return QDialog::exec();
}

void ServiceElasticsearchIndexStructureDialog::reloadTab(int index)
{
    switch (index) {
    case TabMapping: loadMapping(); break;
    case TabSettings: loadSettings(); break;
    case TabAliases: loadAliases(); break;
    case TabTemplates: loadTemplates(); break;
    case TabAnalyze: refreshAnalyzerList(); break;
    default: break;
    }
}

void ServiceElasticsearchIndexStructureDialog::loadMapping()
{
    m_mappingView->setPlainText(QStringLiteral("加载中…"));
    const ServiceResult result = ElasticsearchServiceClient::getIndexMapping(m_endpoint, m_index);
    if (!result.ok) {
        m_mappingView->setPlainText(QStringLiteral("加载 mapping 失败：%1").arg(result.message));
        return;
    }
    m_mappingView->setPlainText(prettyJson(result.message));
}

void ServiceElasticsearchIndexStructureDialog::loadSettings()
{
    m_settingsView->setPlainText(QStringLiteral("加载中…"));
    const ServiceResult result = ElasticsearchServiceClient::getIndexSettings(m_endpoint, m_index);
    if (!result.ok) {
        m_settingsView->setPlainText(QStringLiteral("加载 settings 失败：%1").arg(result.message));
        m_cachedSettings.clear();
        m_analyzers.clear();
        return;
    }
    m_cachedSettings = result.message;
    m_settingsView->setPlainText(prettyJson(result.message));
    QJsonDocument doc = QJsonDocument::fromJson(result.message.toUtf8());
    m_analyzers = ElasticsearchServiceClient::listAnalyzersFromSettings(doc.object());
}

void ServiceElasticsearchIndexStructureDialog::loadAliases()
{
    m_aliasesTable->setRowCount(0);
    const ServiceResult result = ElasticsearchServiceClient::getAliasesForIndex(m_endpoint, m_index);
    if (!result.ok) {
        m_aliasesTable->setRowCount(1);
        auto *item = new QTableWidgetItem(QStringLiteral("加载 aliases 失败：%1").arg(result.message));
        m_aliasesTable->setItem(0, 0, item);
        m_aliasesTable->setSpan(0, 0, 1, 3);
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(result.message.toUtf8());
    if (!doc.isObject()) {
        m_aliasesTable->setRowCount(1);
        m_aliasesTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("响应不是 JSON 对象")));
        m_aliasesTable->setSpan(0, 0, 1, 3);
        return;
    }
    const QJsonObject root = doc.object();
    int row = 0;
    for (auto indexIt = root.begin(); indexIt != root.end(); ++indexIt) {
        const QJsonObject aliases = indexIt.value().toObject().value(QStringLiteral("aliases")).toObject();
        for (auto aliasIt = aliases.begin(); aliasIt != aliases.end(); ++aliasIt) {
            m_aliasesTable->insertRow(row);
            m_aliasesTable->setItem(row, 0, new QTableWidgetItem(aliasIt.key()));
            m_aliasesTable->setItem(row, 1, new QTableWidgetItem(indexIt.key()));
            const bool isWriteIndex = aliasIt.value().toObject().value(QStringLiteral("is_write_index")).toBool(false);
            m_aliasesTable->setItem(row, 2, new QTableWidgetItem(isWriteIndex ? QStringLiteral("是") : QStringLiteral("否")));
            ++row;
        }
    }
    if (row == 0) {
        m_aliasesTable->setRowCount(1);
        auto *empty = new QTableWidgetItem(QStringLiteral("该索引没有别名"));
        empty->setFlags(empty->flags() & ~Qt::ItemIsSelectable);
        m_aliasesTable->setItem(0, 0, empty);
        m_aliasesTable->setSpan(0, 0, 1, 3);
    }
    m_aliasesTable->resizeRowsToContents();
}

void ServiceElasticsearchIndexStructureDialog::loadTemplates()
{
    m_templatesTable->setRowCount(0);
    const ServiceResult result = ElasticsearchServiceClient::listIndexTemplates(m_endpoint);
    if (!result.ok) {
        m_templatesTable->setRowCount(1);
        auto *item = new QTableWidgetItem(QStringLiteral("加载 templates 失败：%1").arg(result.message));
        m_templatesTable->setItem(0, 0, item);
        m_templatesTable->setSpan(0, 0, 1, 4);
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(result.message.toUtf8());
    if (!doc.isObject()) {
        m_templatesTable->setRowCount(1);
        m_templatesTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("响应不是 JSON 对象")));
        m_templatesTable->setSpan(0, 0, 1, 4);
        return;
    }
    const QJsonArray templates = doc.object().value(QStringLiteral("index_templates")).toArray();

    auto globMatch = [](const QString &pattern, const QString &name) {
        QString re = QRegularExpression::escape(pattern);
        re.replace(QRegularExpression(QStringLiteral("\\\\\\*")), QStringLiteral(".*"));
        QRegularExpression rx(QStringLiteral("^%1$").arg(re));
        return rx.match(name).hasMatch();
    };

    int row = 0;
    for (const QJsonValue &v : templates) {
        const QJsonObject wrapper = v.toObject();
        const QJsonObject templateObj = wrapper.value(QStringLiteral("index_template")).toObject();
        const QString name = wrapper.value(QStringLiteral("name")).toString();
        const QJsonArray patterns = templateObj.value(QStringLiteral("index_patterns")).toArray();
        const int priority = templateObj.value(QStringLiteral("priority")).toInt(0);

        QStringList patternStrs;
        int matched = 0;
        for (const QJsonValue &p : patterns) {
            const QString pat = p.toString();
            patternStrs << pat;
            if (globMatch(pat, m_index)) {
                ++matched;
            }
        }
        if (matched == 0) {
            continue;
        }

        m_templatesTable->insertRow(row);
        m_templatesTable->setItem(row, 0, new QTableWidgetItem(name));
        m_templatesTable->setItem(row, 1, new QTableWidgetItem(patternStrs.join(QLatin1String(", "))));
        m_templatesTable->setItem(row, 2, new QTableWidgetItem(QString::number(priority)));
        m_templatesTable->setItem(row, 3, new QTableWidgetItem(QStringLiteral("%1 / %2").arg(matched).arg(patterns.size())));
        ++row;
    }
    if (row == 0) {
        m_templatesTable->setRowCount(1);
        auto *empty = new QTableWidgetItem(QStringLiteral("没有匹配该索引的索引模板（component templates 仍可能生效）"));
        empty->setFlags(empty->flags() & ~Qt::ItemIsSelectable);
        m_templatesTable->setItem(0, 0, empty);
        m_templatesTable->setSpan(0, 0, 1, 4);
    }
    m_templatesTable->resizeRowsToContents();
}

void ServiceElasticsearchIndexStructureDialog::refreshAnalyzerList()
{
    if (m_cachedSettings.isEmpty()) {
        m_analyzers.clear();
        const ServiceResult settingsResult = ElasticsearchServiceClient::getIndexSettings(m_endpoint, m_index);
        if (settingsResult.ok) {
            m_cachedSettings = settingsResult.message;
            QJsonDocument doc = QJsonDocument::fromJson(m_cachedSettings.toUtf8());
            m_analyzers = ElasticsearchServiceClient::listAnalyzersFromSettings(doc.object());
        }
    }
    const QString currentText = m_analyzerCombo->currentText();
    m_analyzerCombo->clear();
    m_analyzerCombo->addItems({QStringLiteral("standard"),
                               QStringLiteral("simple"),
                               QStringLiteral("whitespace"),
                               QStringLiteral("keyword"),
                               QStringLiteral("ik_max_word"),
                               QStringLiteral("ik_smart")});
    for (const QString &name : m_analyzers) {
        if (m_analyzerCombo->findText(name) < 0) {
            m_analyzerCombo->addItem(name);
        }
    }
    if (!currentText.isEmpty()) {
        m_analyzerCombo->setCurrentText(currentText);
    }
}

void ServiceElasticsearchIndexStructureDialog::onAnalyzeText()
{
    const QString text = m_analyzeTextInput->text();
    if (text.isEmpty()) {
        m_analyzeStatusLabel->setText(QStringLiteral("请输入要分词的文本。"));
        m_analyzeStatusLabel->show();
        m_tokensTable->setRowCount(0);
        return;
    }
    const QString analyzer = m_analyzerCombo->currentText().trimmed();
    const ServiceResult result = ElasticsearchServiceClient::analyzeText(m_endpoint, m_index, analyzer, text);
    m_tokensTable->setRowCount(0);
    if (!result.ok) {
        m_analyzeStatusLabel->setText(QStringLiteral("分词失败：%1").arg(result.message));
        m_analyzeStatusLabel->show();
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(result.message.toUtf8());
    if (!doc.isObject()) {
        m_analyzeStatusLabel->setText(QStringLiteral("分词响应不是 JSON 对象。"));
        m_analyzeStatusLabel->show();
        return;
    }
    const QJsonArray tokens = doc.object().value(QStringLiteral("tokens")).toArray();
    if (tokens.isEmpty()) {
        m_analyzeStatusLabel->setText(QStringLiteral("分词器未生成任何 token（可能该分词器不适用于此文本）。"));
        m_analyzeStatusLabel->show();
        return;
    }
    m_tokensTable->setRowCount(tokens.size());
    int row = 0;
    for (const QJsonValue &v : tokens) {
        const QJsonObject t = v.toObject();
        m_tokensTable->setItem(row, 0, new QTableWidgetItem(QString::number(row + 1)));
        m_tokensTable->setItem(row, 1, new QTableWidgetItem(t.value(QStringLiteral("token")).toString()));
        m_tokensTable->setItem(row, 2, new QTableWidgetItem(QString::number(t.value(QStringLiteral("start_offset")).toInt())));
        m_tokensTable->setItem(row, 3, new QTableWidgetItem(QString::number(t.value(QStringLiteral("end_offset")).toInt())));
        m_tokensTable->setItem(row, 4, new QTableWidgetItem(t.value(QStringLiteral("type")).toString()));
        ++row;
    }
    m_tokensTable->resizeRowsToContents();
    m_analyzeStatusLabel->setText(QStringLiteral("分词器：%1    共 %2 个 token")
                                      .arg(analyzer.isEmpty() ? QStringLiteral("(默认)") : analyzer)
                                      .arg(tokens.size()));
    m_analyzeStatusLabel->show();
}

QString ServiceElasticsearchIndexStructureDialog::prettyJson(const QString &raw)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(raw.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return raw;
    }
    return document.toJson(QJsonDocument::Indented);
}
