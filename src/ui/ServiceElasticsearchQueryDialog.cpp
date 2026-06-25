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

    auto *formPanel = new QWidget(this);
    auto *formLayout = new QVBoxLayout(formPanel);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(PageLayout::Space12);

    QFormLayout *form = nullptr;
    formLayout->addWidget(PageLayout::wrapDialogFormSection(QStringLiteral("查询条件"), formPanel, &form));

    m_indexName = new QLineEdit(formPanel);
    PageLayout::configureFormInput(m_indexName);
    form->addRow(QStringLiteral("索引名称"), m_indexName);

    m_queryMode = new QComboBox(formPanel);
    m_queryMode->addItem(QStringLiteral("查询全部 (match_all)"), static_cast<int>(QueryMode::MatchAll));
    m_queryMode->addItem(QStringLiteral("query_string"), static_cast<int>(QueryMode::QueryString));
    m_queryMode->addItem(QStringLiteral("Query DSL (JSON)"), static_cast<int>(QueryMode::QueryDsl));
    PageLayout::configureFormInput(m_queryMode);
    form->addRow(QStringLiteral("查询方式"), m_queryMode);

    m_queryText = new QPlainTextEdit(formPanel);
    m_queryText->setObjectName(QStringLiteral("deployLog"));
    m_queryText->setPlaceholderText(QStringLiteral("query_string 示例: status:200 AND message:error"));
    m_queryText->setMinimumHeight(140);
    form->addRow(QStringLiteral("查询内容"), m_queryText);

    m_page = new QSpinBox(formPanel);
    m_page->setMinimum(1);
    m_page->setMaximum(100000);
    m_page->setValue(1);
    PageLayout::configureFormInput(m_page);
    form->addRow(QStringLiteral("当前页号"), m_page);

    m_pageSize = new QSpinBox(formPanel);
    m_pageSize->setMinimum(1);
    m_pageSize->setMaximum(1000);
    m_pageSize->setValue(10);
    PageLayout::configureFormInput(m_pageSize);
    form->addRow(QStringLiteral("每页大小"), m_pageSize);

    auto *formActions = new QHBoxLayout;
    formActions->setSpacing(PageLayout::Space8);
    m_searchButton = new QPushButton(QStringLiteral("查询"), formPanel);
    m_searchButton->setObjectName(QStringLiteral("primaryButton"));
    m_searchButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    auto *cancelButton = new QPushButton(QStringLiteral("关闭"), formPanel);
    cancelButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    connect(m_searchButton, &QPushButton::clicked, this, &ServiceElasticsearchQueryDialog::onSearch);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_queryMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &ServiceElasticsearchQueryDialog::onQueryModeChanged);
    formActions->addWidget(m_searchButton);
    formActions->addWidget(cancelButton);
    formActions->addStretch();
    formLayout->addLayout(formActions);

    auto *hint = new QLabel(QStringLiteral("Query DSL 模式下可填写完整 _search 请求体 JSON；"
                                           "未写 from/size 时将自动补充分页参数。"),
                            formPanel);
    hint->setWordWrap(true);
    hint->setObjectName(QStringLiteral("sectionLabel"));
    formLayout->addWidget(hint);

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

    body->addWidget(formPanel, 2);
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
