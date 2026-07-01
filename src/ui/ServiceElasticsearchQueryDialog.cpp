#include "ui/ServiceElasticsearchQueryDialog.h"

#include "adapters/services/ElasticsearchServiceClient.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

enum class QueryMode {
    MatchAll = 0,
    QueryString = 1,
    QueryDsl = 2
};

}

ServiceElasticsearchQueryDialog::ServiceElasticsearchQueryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("索引查询"));
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *body = new QHBoxLayout;
    body->setSpacing(PageLayout::Space16);

    auto *formContainer = new QWidget(this);
    auto *formContainerLayout = new QVBoxLayout(formContainer);
    formContainerLayout->setContentsMargins(0, 0, 0, 0);
    formContainerLayout->setSpacing(PageLayout::Space12);

    auto *formCard = new QFrame(formContainer);
    formCard->setObjectName(QStringLiteral("dialogFormPanel"));
    formCard->setAttribute(Qt::WA_StyledBackground, true);
    auto *formCardLayout = new QVBoxLayout(formCard);
    formCardLayout->setContentsMargins(0, 0, 0, 0);
    formCardLayout->setSpacing(0);

    auto *formHeader = new QWidget(formCard);
    formHeader->setObjectName(QStringLiteral("dialogFormPanelHeader"));
    formHeader->setAttribute(Qt::WA_StyledBackground, true);
    auto *formHeaderLayout = new QHBoxLayout(formHeader);
    formHeaderLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space12, PageLayout::Space16, PageLayout::Space12);
    formHeaderLayout->setSpacing(0);
    auto *formHeaderTitle = new QLabel(QStringLiteral("查询条件"), formHeader);
    formHeaderTitle->setObjectName(QStringLiteral("dialogFormPanelTitle"));
    formHeaderLayout->addWidget(formHeaderTitle);
    formHeaderLayout->addStretch();
    formCardLayout->addWidget(formHeader);

    auto *formBody = new QWidget(formCard);
    auto *form = new QFormLayout(formBody);
    form->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    form->setVerticalSpacing(PageLayout::Space12);
    form->setHorizontalSpacing(PageLayout::Space16);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setRowWrapPolicy(QFormLayout::DontWrapRows);
    formCardLayout->addWidget(formBody);

    m_indexName = new QLineEdit(formBody);
    m_indexName->setPlaceholderText(QStringLiteral("索引名 / 别名 / 模板前缀（自动加 *）"));
    PageLayout::configureFormInput(m_indexName);
    form->addRow(QStringLiteral("索引名称"), m_indexName);

    m_queryMode = new QComboBox(formBody);
    m_queryMode->addItem(QStringLiteral("查询全部 (match_all)"), static_cast<int>(QueryMode::MatchAll));
    m_queryMode->addItem(QStringLiteral("query_string"), static_cast<int>(QueryMode::QueryString));
    m_queryMode->addItem(QStringLiteral("Query DSL (JSON)"), static_cast<int>(QueryMode::QueryDsl));
    PageLayout::configureFormInput(m_queryMode);
    form->addRow(QStringLiteral("查询方式"), m_queryMode);

    m_queryText = new QPlainTextEdit(formBody);
    m_queryText->setObjectName(QStringLiteral("deployLog"));
    m_queryText->setPlaceholderText(QStringLiteral("query_string 示例: status:200 AND message:error"));
    m_queryText->setMinimumHeight(140);
    form->addRow(QStringLiteral("查询内容"), m_queryText);

    auto *pagingRow = new QWidget(formBody);
    auto *pagingLayout = new QHBoxLayout(pagingRow);
    pagingLayout->setContentsMargins(0, 0, 0, 0);
    pagingLayout->setSpacing(PageLayout::Space8);

    m_page = new QSpinBox(pagingRow);
    m_page->setMinimum(1);
    m_page->setMaximum(100000);
    m_page->setValue(1);
    m_page->setSuffix(QStringLiteral(" 页"));
    PageLayout::configureFormInput(m_page);

    m_pageSize = new QSpinBox(pagingRow);
    m_pageSize->setMinimum(1);
    m_pageSize->setMaximum(1000);
    m_pageSize->setValue(10);
    m_pageSize->setSuffix(QStringLiteral(" 条"));
    PageLayout::configureFormInput(m_pageSize);

    pagingLayout->addWidget(new QLabel(QStringLiteral("当前页号"), pagingRow));
    pagingLayout->addWidget(m_page);
    pagingLayout->addSpacing(PageLayout::Space16);
    pagingLayout->addWidget(new QLabel(QStringLiteral("每页大小"), pagingRow));
    pagingLayout->addWidget(m_pageSize);
    pagingLayout->addStretch();
    form->addRow(QStringLiteral("分页"), pagingRow);

    auto *formActions = new QHBoxLayout;
    formActions->setSpacing(PageLayout::Space8);
    m_searchButton = new QPushButton(QStringLiteral("查询"), formBody);
    m_searchButton->setObjectName(QStringLiteral("primaryButton"));
    m_searchButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    auto *cancelButton = new QPushButton(QStringLiteral("关闭"), formBody);
    cancelButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    connect(m_searchButton, &QPushButton::clicked, this, &ServiceElasticsearchQueryDialog::onSearch);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_queryMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &ServiceElasticsearchQueryDialog::onQueryModeChanged);
    formActions->addWidget(m_searchButton);
    formActions->addWidget(cancelButton);
    formActions->addStretch();
    form->addRow(QString(), formActions);

    formContainerLayout->addWidget(formCard);

    QLabel *hintText = nullptr;
    formContainerLayout->addWidget(
        PageLayout::wrapDialogHintSection(QStringLiteral("使用提示"), formContainer, &hintText));
    if (hintText != nullptr) {
        hintText->setText(
            QStringLiteral(
                "• 索引名称支持直接填写「具体索引」、「别名」或「模板前缀」（自动补 *）。\n"
                "• 三种查询方式：\n"
                "    1) match_all — 列出该索引最近 N 条文档（无需填写查询内容）。\n"
                "    2) query_string — 类 Lucene 语法，例如 status:active AND level:>5。\n"
                "    3) Query DSL — 完整 JSON；未写 from/size 时将自动补充分页参数。\n"
                "• 模板组行点击「查询」会按组名前缀（如 index_face*）一次查询所有日期子索引。"));
    }
    formContainerLayout->addStretch();

    auto *resultPanel = new QWidget(this);
    auto *resultLayout = new QVBoxLayout(resultPanel);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(PageLayout::Space8);

    auto *resultHeader = new QHBoxLayout;
    resultHeader->addWidget(PageLayout::makeSectionLabel(QStringLiteral("查询结果"), resultPanel));
    resultHeader->addStretch();
    auto *copyButton = new QPushButton(QStringLiteral("复制"), resultPanel);
    copyButton->setFlat(true);
    copyButton->setCursor(Qt::PointingHandCursor);
    connect(copyButton, &QPushButton::clicked, this, &ServiceElasticsearchQueryDialog::onCopyResult);
    resultHeader->addWidget(copyButton);
    resultLayout->addLayout(resultHeader);

    m_result = new QPlainTextEdit(resultPanel);
    m_result->setObjectName(QStringLiteral("deployLog"));
    m_result->setReadOnly(true);
    m_result->setPlaceholderText(QStringLiteral("点击「查询」后在此显示 JSON 结果"));
    resultLayout->addWidget(m_result, 1);

    body->addWidget(formContainer, 2);
    body->addWidget(resultPanel, 3);
    layout->addLayout(body, 1);

    PageLayout::applyModalDialog(this);
    resize(1080, 680);
    onQueryModeChanged(m_queryMode->currentIndex());
}

void ServiceElasticsearchQueryDialog::setEndpoint(const ServiceEndpoint &endpoint)
{
    m_endpoint = endpoint;
}

void ServiceElasticsearchQueryDialog::setIndexName(const QString &indexName)
{
    m_indexName->setText(indexName);
}

void ServiceElasticsearchQueryDialog::onQueryModeChanged(int index)
{
    const auto mode = static_cast<QueryMode>(m_queryMode->itemData(index).toInt());
    if (mode == QueryMode::MatchAll) {
        m_queryText->clear();
        m_queryText->setEnabled(false);
        m_queryText->setPlaceholderText(QStringLiteral("查询全部时无需填写查询内容"));
        return;
    }
    m_queryText->setEnabled(true);
    if (mode == QueryMode::QueryString) {
        m_queryText->setPlaceholderText(QStringLiteral("示例: field:value AND other:*  或  _id:abc123"));
    } else {
        m_queryText->setPlaceholderText(QStringLiteral(
            "示例:\n"
            "{\n"
            "  \"query\": { \"term\": { \"status\": \"ok\" } }\n"
            "}"));
    }
}

QJsonObject ServiceElasticsearchQueryDialog::buildSearchBody(int *fromOut, int *sizeOut, QString *error) const
{
    const int from = (m_page->value() - 1) * m_pageSize->value();
    const int size = m_pageSize->value();
    if (fromOut != nullptr) {
        *fromOut = from;
    }
    if (sizeOut != nullptr) {
        *sizeOut = size;
    }

    const auto mode = static_cast<QueryMode>(m_queryMode->currentData().toInt());
    if (mode == QueryMode::MatchAll) {
        return QJsonObject{
            {QStringLiteral("from"), from},
            {QStringLiteral("size"), size},
            {QStringLiteral("track_total_hits"), true},
            {QStringLiteral("query"), QJsonObject{{QStringLiteral("match_all"), QJsonObject{}}}}
        };
    }

    const QString text = m_queryText->toPlainText().trimmed();
    if (text.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请填写查询内容。");
        }
        return {};
    }

    if (mode == QueryMode::QueryString) {
        return QJsonObject{
            {QStringLiteral("from"), from},
            {QStringLiteral("size"), size},
            {QStringLiteral("track_total_hits"), true},
            {QStringLiteral("query"),
             QJsonObject{{QStringLiteral("query_string"), QJsonObject{{QStringLiteral("query"), text}}}}}
        };
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("Query DSL JSON 解析失败: %1").arg(parseError.errorString());
        }
        return {};
    }

    QJsonObject body = document.object();
    body.insert(QStringLiteral("from"), from);
    body.insert(QStringLiteral("size"), size);
    body.insert(QStringLiteral("track_total_hits"), true);
    return body;
}

void ServiceElasticsearchQueryDialog::onSearch()
{
    const QString index = m_indexName->text().trimmed();
    if (index.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写索引名称。"));
        return;
    }

    QString error;
    const QJsonObject body = buildSearchBody(nullptr, nullptr, &error);
    if (body.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), error);
        return;
    }

    m_searchButton->setEnabled(false);
    const ServiceResult result = ElasticsearchServiceClient::searchIndexBody(m_endpoint, index, body);
    m_searchButton->setEnabled(true);
    if (!result.ok) {
        m_result->setPlainText(result.message);
        return;
    }
    m_result->setPlainText(result.message);
}

void ServiceElasticsearchQueryDialog::onCopyResult()
{
    if (m_result->toPlainText().isEmpty()) {
        return;
    }
    QApplication::clipboard()->setText(m_result->toPlainText());
}
