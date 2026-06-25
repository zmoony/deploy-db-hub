#include "ui/HttpRequestWidget.h"

#include "infra/DataPaths.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStackedWidget>
#include <QSyntaxHighlighter>
#include <QTableWidget>
#include <QTextCharFormat>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace {

class JsonSyntaxHighlighter final : public QSyntaxHighlighter
{
public:
    explicit JsonSyntaxHighlighter(QTextDocument *document)
        : QSyntaxHighlighter(document)
    {
        QTextCharFormat keyFormat;
        keyFormat.setForeground(QColor(QStringLiteral("#7C3AED")));
        keyFormat.setFontWeight(QFont::DemiBold);
        m_rules.append({QRegularExpression(QStringLiteral("\"(?:\\\\.|[^\"\\\\])*\"(?=\\s*:)")), keyFormat});

        QTextCharFormat stringFormat;
        stringFormat.setForeground(QColor(QStringLiteral("#059669")));
        m_rules.append({QRegularExpression(QStringLiteral("\"(?:\\\\.|[^\"\\\\])*\"")), stringFormat});

        QTextCharFormat numberFormat;
        numberFormat.setForeground(QColor(QStringLiteral("#D97706")));
        m_rules.append({QRegularExpression(QStringLiteral("\\b-?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?\\b")),
                        numberFormat});

        QTextCharFormat literalFormat;
        literalFormat.setForeground(QColor(QStringLiteral("#6D28D9")));
        literalFormat.setFontWeight(QFont::DemiBold);
        m_rules.append({QRegularExpression(QStringLiteral("\\b(true|false|null)\\b")), literalFormat});
    }

protected:
    void highlightBlock(const QString &text) override
    {
        for (const Rule &rule : m_rules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                const QRegularExpressionMatch match = it.next();
                setFormat(match.capturedStart(), match.capturedLength(), rule.format);
            }
        }
    }

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<Rule> m_rules;
};

QPushButton *makeCompactButton(const QString &text, QWidget *parent, const QString &objectName = QString())
{
    auto *button = new QPushButton(text, parent);
    if (!objectName.isEmpty()) {
        button->setObjectName(objectName);
    }
    button->setMinimumHeight(32);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

QPlainTextEdit *makeCodeEditor(QWidget *parent, bool readOnly = false)
{
    auto *editor = new QPlainTextEdit(parent);
    editor->setObjectName(QStringLiteral("httpCodeEditor"));
    editor->setReadOnly(readOnly);
    editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    editor->setTabStopDistance(4 * editor->fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    if (!readOnly) {
        editor->setPlaceholderText(QStringLiteral("{ \"key\": \"value\" }"));
    }
    new JsonSyntaxHighlighter(editor->document());
    return editor;
}

QString formatHistoryLine(const HttpHistoryEntry &entry)
{
    const QString url = entry.url;
    const int slash = url.indexOf(QStringLiteral("//"));
    QString path = slash >= 0 ? url.mid(slash + 2) : url;
    const int pathSlash = path.indexOf(QLatin1Char('/'));
    if (pathSlash >= 0) {
        path = path.mid(pathSlash);
    } else {
        path = QStringLiteral("/");
    }
    if (path.length() > 28) {
        path = path.left(25) + QStringLiteral("...");
    }

    QString statusPart;
    if (entry.statusCode > 0) {
        statusPart = QStringLiteral("  %1").arg(entry.statusCode);
        if (entry.elapsedMs > 0) {
            statusPart += QStringLiteral("  %1ms").arg(entry.elapsedMs);
        }
    }
    return QStringLiteral("%1  %2%3").arg(entry.method, path, statusPart);
}

QJsonObject headersToJson(const QString &serialized)
{
    QJsonObject object;
    const QStringList lines = serialized.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        object.insert(line.left(colon).trimmed(), line.mid(colon + 1).trimmed());
    }
    return object;
}

QString headersFromReply(const QList<QNetworkReply::RawHeaderPair> &pairs)
{
    QStringList lines;
    for (const auto &pair : pairs) {
        lines.append(QStringLiteral("%1: %2")
                         .arg(QString::fromUtf8(pair.first), QString::fromUtf8(pair.second)));
    }
    return lines.join(QLatin1Char('\n'));
}

QString cookiesFromReply(const QList<QNetworkReply::RawHeaderPair> &pairs)
{
    QStringList cookies;
    for (const auto &pair : pairs) {
        if (QString::fromUtf8(pair.first).compare(QStringLiteral("Set-Cookie"), Qt::CaseInsensitive) == 0) {
            cookies.append(QString::fromUtf8(pair.second));
        }
    }
    return cookies.join(QLatin1Char('\n'));
}

QString formatResponseBody(const QByteArray &payload)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error == QJsonParseError::NoError && (document.isObject() || document.isArray())) {
        return QString::fromUtf8(document.toJson(QJsonDocument::Indented));
    }
    return QString::fromUtf8(payload);
}

QString statusBadgeObjectName(int statusCode)
{
    if (statusCode >= 200 && statusCode < 300) {
        return QStringLiteral("httpStatusOk");
    }
    if (statusCode >= 400) {
        return QStringLiteral("httpStatusError");
    }
    return QStringLiteral("httpStatusNeutral");
}

QString statusBadgeText(int statusCode)
{
    if (statusCode <= 0) {
        return QStringLiteral("-");
    }
    return QString::number(statusCode);
}

}

HttpRequestWidget::HttpRequestWidget(QWidget *parent)
    : QWidget(parent)
{
    setProperty("fitFirstScreen", true);
    setObjectName(QStringLiteral("httpRequestWidget"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("httpMainSplitter"));
    splitter->setChildrenCollapsible(false);
    splitter->setHandleWidth(5);

    auto *historyPanel = new QWidget(splitter);
    historyPanel->setObjectName(QStringLiteral("httpHistoryPanel"));
    historyPanel->setAttribute(Qt::WA_StyledBackground, true);
    historyPanel->setMinimumWidth(200);
    historyPanel->setMaximumWidth(280);
    auto *historyLayout = new QVBoxLayout(historyPanel);
    historyLayout->setContentsMargins(8, 8, 8, 8);
    historyLayout->setSpacing(6);

    auto *historyHeader = new QHBoxLayout;
    historyHeader->setContentsMargins(0, 0, 0, 0);
    historyHeader->setSpacing(6);
    auto *historyTitle = new QLabel(QStringLiteral("历史记录"), historyPanel);
    historyTitle->setObjectName(QStringLiteral("httpSectionTitle"));
    historyHeader->addWidget(historyTitle);
    historyHeader->addStretch();
    historyLayout->addLayout(historyHeader);

    m_historySearch = new QLineEdit(historyPanel);
    m_historySearch->setObjectName(QStringLiteral("httpHistorySearch"));
    m_historySearch->setPlaceholderText(QStringLiteral("搜索 URL / 方法"));
    m_historySearch->setClearButtonEnabled(true);
    PageLayout::configureFormInput(m_historySearch);
    historyLayout->addWidget(m_historySearch);

    m_historyList = new QListWidget(historyPanel);
    m_historyList->setObjectName(QStringLiteral("httpHistoryList"));
    m_historyList->setUniformItemSizes(true);
    m_historyList->setSpacing(0);
    m_historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    historyLayout->addWidget(m_historyList, 1);

    auto *clearHistoryButton = makeCompactButton(QStringLiteral("清空"), historyPanel);
    clearHistoryButton->setObjectName(QStringLiteral("httpGhostButton"));
    auto *historyFooter = new QHBoxLayout;
    historyFooter->addStretch();
    historyFooter->addWidget(clearHistoryButton);
    historyLayout->addLayout(historyFooter);

    auto *workspace = new QWidget(splitter);
    workspace->setObjectName(QStringLiteral("httpWorkspace"));
    workspace->setAttribute(Qt::WA_StyledBackground, true);
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(0, 0, 0, 0);
    workspaceLayout->setSpacing(8);

    auto *urlRow = new QWidget(workspace);
    auto *urlLayout = new QHBoxLayout(urlRow);
    urlLayout->setContentsMargins(0, 0, 0, 0);
    urlLayout->setSpacing(8);
    m_method = new QComboBox(urlRow);
    m_method->setObjectName(QStringLiteral("httpMethodCombo"));
    m_method->addItems({QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
                        QStringLiteral("DELETE"), QStringLiteral("PATCH"), QStringLiteral("HEAD")});
    m_method->setFixedWidth(88);
    PageLayout::configureFormInput(m_method);
    m_url = new QLineEdit(urlRow);
    m_url->setObjectName(QStringLiteral("httpUrlInput"));
    m_url->setPlaceholderText(QStringLiteral("https://api.example.com/v1/users?page=1&limit=20"));
    PageLayout::configureFormInput(m_url);
    m_sendButton = makeCompactButton(QStringLiteral("发送"), urlRow, QStringLiteral("httpSendButton"));
    auto *exportButton = makeCompactButton(QStringLiteral("导出 JSON"), urlRow, QStringLiteral("httpGhostButton"));
    urlLayout->addWidget(m_method);
    urlLayout->addWidget(m_url, 1);
    urlLayout->addWidget(m_sendButton);
    urlLayout->addWidget(exportButton);
    workspaceLayout->addWidget(urlRow);

    auto *requestStack = new QStackedWidget(workspace);
    requestStack->setObjectName(QStringLiteral("httpRequestStack"));
    QButtonGroup *requestTabGroup = nullptr;
    const QStringList requestTabLabels = {
        QStringLiteral("请求头"),
        QStringLiteral("参数"),
        QStringLiteral("Body"),
        QStringLiteral("认证")
    };
    workspaceLayout->addWidget(makeCapsuleTabBar(requestTabLabels, requestStack, &requestTabGroup));

    auto *headersPage = makeKeyValueSection(QStringLiteral("添加"), &m_headers, requestStack);
    auto *paramsPage = makeKeyValueSection(QStringLiteral("添加"), &m_params, requestStack);
    requestStack->addWidget(headersPage);
    requestStack->addWidget(paramsPage);

    auto *bodyPage = new QWidget(requestStack);
    auto *bodyLayout = new QVBoxLayout(bodyPage);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(6);
    auto *bodyTitle = new QLabel(QStringLiteral("请求 Body"), bodyPage);
    bodyTitle->setObjectName(QStringLiteral("httpSectionTitle"));
    bodyLayout->addWidget(bodyTitle);
    m_body = makeCodeEditor(bodyPage);
    m_body->setPlaceholderText(QStringLiteral("请求 Body，GET 可留空"));
    bodyLayout->addWidget(m_body, 1);
    requestStack->addWidget(bodyPage);

    auto *authPage = new QWidget(requestStack);
    auto *authLayout = new QVBoxLayout(authPage);
    authLayout->setContentsMargins(0, 0, 0, 0);
    authLayout->setSpacing(8);
    auto *authTitle = new QLabel(QStringLiteral("认证"), authPage);
    authTitle->setObjectName(QStringLiteral("httpSectionTitle"));
    authLayout->addWidget(authTitle);
    auto *authForm = new QWidget(authPage);
    auto *authFormLayout = new QHBoxLayout(authForm);
    authFormLayout->setContentsMargins(0, 0, 0, 0);
    authFormLayout->setSpacing(8);
    m_authType = new QComboBox(authForm);
    m_authType->addItems({QStringLiteral("None"), QStringLiteral("Bearer"), QStringLiteral("Basic")});
    PageLayout::configureFormInput(m_authType);
    m_authType->setFixedWidth(120);
    m_authValue = new QLineEdit(authForm);
    m_authValue->setPlaceholderText(QStringLiteral("Token 或 username:password"));
    PageLayout::configureFormInput(m_authValue);
    authFormLayout->addWidget(new QLabel(QStringLiteral("类型"), authForm));
    authFormLayout->addWidget(m_authType);
    authFormLayout->addWidget(m_authValue, 1);
    authLayout->addWidget(authForm);
    authLayout->addStretch();
    requestStack->addWidget(authPage);

    workspaceLayout->addWidget(requestStack, 2);

    auto *responsePanel = new QWidget(workspace);
    responsePanel->setObjectName(QStringLiteral("httpResponsePanel"));
    auto *responseLayout = new QVBoxLayout(responsePanel);
    responseLayout->setContentsMargins(0, 0, 0, 0);
    responseLayout->setSpacing(6);

    auto *responseHeaderRow = new QHBoxLayout;
    responseHeaderRow->setContentsMargins(0, 0, 0, 0);
    responseHeaderRow->setSpacing(8);
    auto *responseTitle = new QLabel(QStringLiteral("RESPONSE"), responsePanel);
    responseTitle->setObjectName(QStringLiteral("httpResponseTitle"));
    responseHeaderRow->addWidget(responseTitle);
    responseHeaderRow->addStretch();
    m_statusBadge = new QLabel(QStringLiteral("-"), responsePanel);
    m_statusBadge->setObjectName(QStringLiteral("httpStatusNeutral"));
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_elapsedLabel = new QLabel(QStringLiteral("-"), responsePanel);
    m_elapsedLabel->setObjectName(QStringLiteral("httpMetricLabel"));
    m_sizeLabel = new QLabel(QStringLiteral("-"), responsePanel);
    m_sizeLabel->setObjectName(QStringLiteral("httpMetricLabel"));
    responseHeaderRow->addWidget(m_statusBadge);
    responseHeaderRow->addWidget(m_elapsedLabel);
    responseHeaderRow->addWidget(m_sizeLabel);
    responseLayout->addLayout(responseHeaderRow);

    auto *responseStack = new QStackedWidget(responsePanel);
    QButtonGroup *responseTabGroup = nullptr;
    const QStringList responseTabLabels = {
        QStringLiteral("响应体"),
        QStringLiteral("响应头"),
        QStringLiteral("Cookies")
    };
    responseLayout->addWidget(makeCapsuleTabBar(responseTabLabels, responseStack, &responseTabGroup));

    m_responseBody = makeCodeEditor(responseStack, true);
    m_responseHeaders = makeCodeEditor(responseStack, true);
    m_responseCookies = makeCodeEditor(responseStack, true);
    responseStack->addWidget(m_responseBody);
    responseStack->addWidget(m_responseHeaders);
    responseStack->addWidget(m_responseCookies);
    responseLayout->addWidget(responseStack, 1);
    workspaceLayout->addWidget(responsePanel, 3);

    splitter->addWidget(historyPanel);
    splitter->addWidget(workspace);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({220, 860});
    layout->addWidget(splitter, 1);

    addKeyValueRow(m_headers, QStringLiteral("Content-Type"), QStringLiteral("application/json"));
    applyMethodStyle();

    connect(m_method, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        applyMethodStyle();
    });
    connect(m_sendButton, &QPushButton::clicked, this, &HttpRequestWidget::sendRequest);
    connect(exportButton, &QPushButton::clicked, this, &HttpRequestWidget::exportRequestJson);
    connect(clearHistoryButton, &QPushButton::clicked, this, [this]() {
        m_history.clear();
        saveHistory();
        refreshHistoryList();
    });
    connect(m_historyList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (item == nullptr) {
            return;
        }
        const int index = item->data(Qt::UserRole).toInt();
        if (index >= 0 && index < m_history.size()) {
            applyEntry(m_history.at(index));
        }
    });
    connect(m_historySearch, &QLineEdit::textChanged, this, [this](const QString &) {
        refreshHistoryList();
    });

    loadHistory();
    refreshHistoryList();
}

QWidget *HttpRequestWidget::makeCapsuleTabBar(const QStringList &labels,
                                              QStackedWidget *stack,
                                              QButtonGroup **groupOut)
{
    auto *tabBar = new QWidget;
    tabBar->setObjectName(QStringLiteral("httpCapsuleTabBar"));
    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(4, 4, 4, 4);
    tabLayout->setSpacing(4);

    auto *tabGroup = new QButtonGroup(tabBar);
    tabGroup->setExclusive(true);
    for (int i = 0; i < labels.size(); ++i) {
        auto *button = new QPushButton(labels.at(i), tabBar);
        button->setObjectName(QStringLiteral("httpCapsuleTabButton"));
        button->setCheckable(true);
        button->setChecked(i == 0);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        tabGroup->addButton(button, i);
        tabLayout->addWidget(button);
    }
    tabLayout->addStretch();

    if (groupOut != nullptr) {
        *groupOut = tabGroup;
    }
    if (stack != nullptr) {
        connect(tabGroup, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);
    }
    return tabBar;
}

QWidget *HttpRequestWidget::makeKeyValueSection(const QString &addLabel,
                                                QTableWidget **tableOut,
                                                QWidget *parent)
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *table = new QTableWidget(page);
    table->setObjectName(QStringLiteral("httpKeyValueTable"));
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("Key"), QStringLiteral("Value"), QStringLiteral("")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    table->setColumnWidth(2, 40);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setAlternatingRowColors(true);
    PageLayout::configureDataTable(table);
    layout->addWidget(table, 1);

    auto *actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(6);
    auto *addButton = makeCompactButton(addLabel, page, QStringLiteral("httpGhostButton"));
    auto *removeButton = makeCompactButton(QStringLiteral("删除选中"), page, QStringLiteral("httpGhostButton"));
    actions->addWidget(addButton);
    actions->addWidget(removeButton);
    actions->addStretch();
    layout->addLayout(actions);

    connect(addButton, &QPushButton::clicked, this, [this, table]() {
        addKeyValueRow(table);
    });
    connect(removeButton, &QPushButton::clicked, this, [this, table]() {
        removeSelectedKeyValueRow(table);
    });

    if (tableOut != nullptr) {
        *tableOut = table;
    }
    return page;
}

void HttpRequestWidget::addKeyValueRow(QTableWidget *table, const QString &key, const QString &value)
{
    if (table == nullptr) {
        return;
    }
    const int row = table->rowCount();
    table->insertRow(row);
    table->setItem(row, 0, new QTableWidgetItem(key));
    table->setItem(row, 1, new QTableWidgetItem(value));
    auto *removeButton = makeCompactButton(QStringLiteral("×"), table, QStringLiteral("httpRowRemoveButton"));
    removeButton->setFixedSize(28, 28);
    connect(removeButton, &QPushButton::clicked, this, [this, table, removeButton]() {
        for (int i = 0; i < table->rowCount(); ++i) {
            if (table->cellWidget(i, 2) == removeButton) {
                table->removeRow(i);
                break;
            }
        }
    });
    table->setCellWidget(row, 2, removeButton);
}

void HttpRequestWidget::removeSelectedKeyValueRow(QTableWidget *table)
{
    if (table == nullptr) {
        return;
    }
    const int row = table->currentRow();
    if (row >= 0) {
        table->removeRow(row);
    }
}

QString HttpRequestWidget::serializeKeyValueTable(const QTableWidget *table) const
{
    if (table == nullptr) {
        return {};
    }
    QStringList lines;
    for (int row = 0; row < table->rowCount(); ++row) {
        const QTableWidgetItem *keyItem = table->item(row, 0);
        const QTableWidgetItem *valueItem = table->item(row, 1);
        const QString key = keyItem != nullptr ? keyItem->text().trimmed() : QString();
        const QString value = valueItem != nullptr ? valueItem->text() : QString();
        if (!key.isEmpty()) {
            lines.append(QStringLiteral("%1: %2").arg(key, value));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

void HttpRequestWidget::applyKeyValueTable(QTableWidget *table, const QString &serialized)
{
    if (table == nullptr) {
        return;
    }
    table->setRowCount(0);
    const QStringList lines = serialized.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        addKeyValueRow(table, line.left(colon).trimmed(), line.mid(colon + 1).trimmed());
    }
}

QString HttpRequestWidget::buildRequestUrl() const
{
    QString baseUrl = m_url->text().trimmed();
    const int queryIndex = baseUrl.indexOf(QLatin1Char('?'));
    if (queryIndex >= 0) {
        baseUrl = baseUrl.left(queryIndex);
    }

    QUrl url(baseUrl);
    QUrlQuery query;
    for (int row = 0; row < m_params->rowCount(); ++row) {
        const QTableWidgetItem *keyItem = m_params->item(row, 0);
        const QTableWidgetItem *valueItem = m_params->item(row, 1);
        const QString key = keyItem != nullptr ? keyItem->text().trimmed() : QString();
        if (!key.isEmpty()) {
            query.addQueryItem(key, valueItem != nullptr ? valueItem->text() : QString());
        }
    }
    url.setQuery(query);
    return url.toString(QUrl::FullyEncoded);
}

void HttpRequestWidget::applyMethodStyle()
{
    if (m_method == nullptr) {
        return;
    }
    m_method->setProperty("httpVerb", m_method->currentText());
    m_method->style()->unpolish(m_method);
    m_method->style()->polish(m_method);
}

HttpHistoryEntry HttpRequestWidget::currentEntry() const
{
    HttpHistoryEntry entry;
    entry.group = QStringLiteral("默认分组");
    entry.method = m_method->currentText();
    entry.url = m_url->text().trimmed();
    entry.headers = serializeKeyValueTable(m_headers);
    entry.params = serializeKeyValueTable(m_params);
    entry.body = m_body->toPlainText();
    entry.authType = m_authType->currentText();
    entry.authValue = m_authValue->text();
    entry.name = QStringLiteral("%1 %2").arg(entry.method, entry.url);
    return entry;
}

void HttpRequestWidget::applyEntry(const HttpHistoryEntry &entry)
{
    const int methodIndex = m_method->findText(entry.method);
    if (methodIndex >= 0) {
        m_method->setCurrentIndex(methodIndex);
    }
    applyMethodStyle();

    QUrl url(entry.url);
    m_url->setText(url.toString(QUrl::RemoveQuery));
    m_params->setRowCount(0);
    const QUrlQuery query(url);
    const QList<QPair<QString, QString>> items = query.queryItems();
    for (const auto &item : items) {
        addKeyValueRow(m_params, item.first, item.second);
    }

    applyKeyValueTable(m_headers, entry.headers);
    if (!entry.params.isEmpty()) {
        applyKeyValueTable(m_params, entry.params);
    }
    m_body->setPlainText(entry.body);

    const int authIndex = m_authType->findText(entry.authType.isEmpty() ? QStringLiteral("None") : entry.authType);
    if (authIndex >= 0) {
        m_authType->setCurrentIndex(authIndex);
    }
    m_authValue->setText(entry.authValue);
}

void HttpRequestWidget::sendRequest()
{
    const QUrl requestUrl(buildRequestUrl());
    if (!requestUrl.isValid() || requestUrl.scheme().isEmpty()) {
        m_responseBody->setPlainText(QStringLiteral("URL 无效"));
        return;
    }

    auto *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(requestUrl);
    const QStringList headerLines = serializeKeyValueTable(m_headers).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : headerLines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        request.setRawHeader(line.left(colon).trimmed().toUtf8(), line.mid(colon + 1).trimmed().toUtf8());
    }

    const QString authType = m_authType->currentText();
    const QString authValue = m_authValue->text().trimmed();
    if (authType == QStringLiteral("Bearer") && !authValue.isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(authValue).toUtf8());
    } else if (authType == QStringLiteral("Basic") && !authValue.isEmpty()) {
        request.setRawHeader("Authorization",
                             QStringLiteral("Basic %1")
                                 .arg(QString::fromUtf8(authValue.toUtf8().toBase64()))
                                 .toUtf8());
    }

    const QByteArray payload = m_body->toPlainText().toUtf8();
    const QString verb = m_method->currentText();
    QNetworkReply *reply = nullptr;
    if (verb == QStringLiteral("GET")) {
        reply = manager->get(request);
    } else if (verb == QStringLiteral("POST")) {
        reply = manager->post(request, payload);
    } else if (verb == QStringLiteral("PUT")) {
        reply = manager->put(request, payload);
    } else if (verb == QStringLiteral("HEAD")) {
        reply = manager->head(request);
    } else {
        reply = manager->sendCustomRequest(request, verb.toUtf8(), payload);
    }

    m_sendButton->setEnabled(false);
    m_responseBody->setPlainText(QStringLiteral("请求中..."));
    m_responseHeaders->clear();
    m_responseCookies->clear();
    m_statusBadge->setText(QStringLiteral("..."));
    m_statusBadge->setObjectName(QStringLiteral("httpStatusNeutral"));
    m_statusBadge->style()->unpolish(m_statusBadge);
    m_statusBadge->style()->polish(m_statusBadge);
    m_elapsedLabel->setText(QStringLiteral("-"));
    m_sizeLabel->setText(QStringLiteral("-"));

    m_history.prepend(currentEntry());
    if (m_history.size() > 100) {
        m_history.resize(100);
    }
    saveHistory();
    refreshHistoryList();

    auto *timer = new QElapsedTimer;
    timer->start();
    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, reply, manager, timer]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const qint64 elapsed = timer->elapsed();
        delete timer;

        const QByteArray payload = reply->readAll();
        const auto headerPairs = reply->rawHeaderPairs();
        m_lastStatusCode = status;
        m_lastElapsedMs = elapsed;
        m_lastSizeBytes = payload.size();
        m_lastResponseBody = formatResponseBody(payload);
        m_lastResponseHeaders = headersFromReply(headerPairs);
        m_lastResponseCookies = cookiesFromReply(headerPairs);

        m_responseBody->setPlainText(m_lastResponseBody);
        m_responseHeaders->setPlainText(m_lastResponseHeaders);
        m_responseCookies->setPlainText(m_lastResponseCookies.isEmpty()
                                            ? QStringLiteral("(无 Set-Cookie)")
                                            : m_lastResponseCookies);

        m_statusBadge->setText(statusBadgeText(status));
        m_statusBadge->setObjectName(statusBadgeObjectName(status));
        m_statusBadge->style()->unpolish(m_statusBadge);
        m_statusBadge->style()->polish(m_statusBadge);
        m_elapsedLabel->setText(elapsed > 0 ? QStringLiteral("%1ms").arg(elapsed) : QStringLiteral("-"));
        if (payload.size() < 1024) {
            m_sizeLabel->setText(QStringLiteral("%1B").arg(payload.size()));
        } else if (payload.size() < 1024 * 1024) {
            m_sizeLabel->setText(QStringLiteral("%1KB").arg(payload.size() / 1024.0, 0, 'f', 1));
        } else {
            m_sizeLabel->setText(QStringLiteral("%1MB").arg(payload.size() / (1024.0 * 1024.0), 0, 'f', 2));
        }

        if (reply->error() != QNetworkReply::NoError && status == 0) {
            m_responseBody->setPlainText(QStringLiteral("错误: %1").arg(reply->errorString()));
        }

        updateLatestHistoryResult(status, elapsed);
        m_sendButton->setEnabled(true);
        reply->deleteLater();
        manager->deleteLater();
    });
}

void HttpRequestWidget::updateLatestHistoryResult(int statusCode, qint64 elapsedMs)
{
    if (m_history.isEmpty()) {
        return;
    }
    m_history[0].statusCode = statusCode;
    m_history[0].elapsedMs = elapsedMs;
    saveHistory();
    refreshHistoryList();
}

void HttpRequestWidget::exportRequestJson()
{
    QJsonObject root;
    root.insert(QStringLiteral("method"), m_method->currentText());
    root.insert(QStringLiteral("url"), buildRequestUrl());
    root.insert(QStringLiteral("headers"), headersToJson(serializeKeyValueTable(m_headers)));
    root.insert(QStringLiteral("params"), headersToJson(serializeKeyValueTable(m_params)));
    root.insert(QStringLiteral("body"), m_body->toPlainText());
    root.insert(QStringLiteral("authType"), m_authType->currentText());
    root.insert(QStringLiteral("authValue"), m_authValue->text());

    QJsonObject response;
    response.insert(QStringLiteral("status"), m_lastStatusCode);
    response.insert(QStringLiteral("elapsedMs"), m_lastElapsedMs);
    response.insert(QStringLiteral("sizeBytes"), m_lastSizeBytes);
    response.insert(QStringLiteral("headers"), headersToJson(m_lastResponseHeaders));
    response.insert(QStringLiteral("body"), m_lastResponseBody);
    response.insert(QStringLiteral("cookies"), m_lastResponseCookies);
    root.insert(QStringLiteral("response"), response);

    const QString suggested = QDir(DataPaths::configDir()).filePath(
        QStringLiteral("http-request-%1.json")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    const QString path = QFileDialog::getSaveFileName(this,
                                                      QStringLiteral("导出 HTTP JSON"),
                                                      suggested,
                                                      QStringLiteral("JSON 文件 (*.json)"));
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QString HttpRequestWidget::historyFilePath() const
{
    QString dir = DataPaths::configDir();
    if (dir.isEmpty()) {
        dir = QDir::currentPath();
    }
    return QDir(dir).filePath(QStringLiteral("http-history.json"));
}

void HttpRequestWidget::loadHistory()
{
    m_history.clear();
    QFile file(historyFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        return;
    }
    const QJsonArray array = document.array();
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        HttpHistoryEntry entry;
        entry.group = object.value(QStringLiteral("group")).toString();
        entry.name = object.value(QStringLiteral("name")).toString();
        entry.method = object.value(QStringLiteral("method")).toString();
        entry.url = object.value(QStringLiteral("url")).toString();
        entry.headers = object.value(QStringLiteral("headers")).toString();
        entry.params = object.value(QStringLiteral("params")).toString();
        entry.body = object.value(QStringLiteral("body")).toString();
        entry.authType = object.value(QStringLiteral("authType")).toString();
        entry.authValue = object.value(QStringLiteral("authValue")).toString();
        entry.statusCode = object.value(QStringLiteral("statusCode")).toInt();
        entry.elapsedMs = object.value(QStringLiteral("elapsedMs")).toVariant().toLongLong();
        m_history.append(entry);
    }
}

void HttpRequestWidget::saveHistory()
{
    QJsonArray array;
    for (const HttpHistoryEntry &entry : m_history) {
        QJsonObject object;
        object.insert(QStringLiteral("group"), entry.group);
        object.insert(QStringLiteral("name"), entry.name);
        object.insert(QStringLiteral("method"), entry.method);
        object.insert(QStringLiteral("url"), entry.url);
        object.insert(QStringLiteral("headers"), entry.headers);
        object.insert(QStringLiteral("params"), entry.params);
        object.insert(QStringLiteral("body"), entry.body);
        object.insert(QStringLiteral("authType"), entry.authType);
        object.insert(QStringLiteral("authValue"), entry.authValue);
        object.insert(QStringLiteral("statusCode"), entry.statusCode);
        object.insert(QStringLiteral("elapsedMs"), entry.elapsedMs);
        array.append(object);
    }
    QFile file(historyFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void HttpRequestWidget::refreshHistoryList()
{
    if (m_historyList == nullptr) {
        return;
    }
    m_historyList->clear();
    const QString keyword = m_historySearch != nullptr ? m_historySearch->text().trimmed() : QString();
    for (int i = 0; i < m_history.size(); ++i) {
        const HttpHistoryEntry &entry = m_history.at(i);
        const QString haystack = QStringLiteral("%1 %2").arg(entry.method, entry.url);
        if (!keyword.isEmpty() && !haystack.contains(keyword, Qt::CaseInsensitive)) {
            continue;
        }
        auto *item = new QListWidgetItem(formatHistoryLine(entry), m_historyList);
        item->setData(Qt::UserRole, i);
        item->setSizeHint(QSize(0, 30));
        item->setData(Qt::UserRole + 1, entry.method);
    }
}
