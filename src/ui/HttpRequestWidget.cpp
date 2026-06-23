#include "ui/HttpRequestWidget.h"

#include "infra/DataPaths.h"
#include "ui/PageLayout.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStringList>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>

namespace {

QPushButton *makeButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setMinimumHeight(PageLayout::DialogFieldHeight);
    return button;
}

}

HttpRequestWidget::HttpRequestWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(PageLayout::Space12);
    layout->addWidget(PageLayout::makeHeaderBlock(
        QStringLiteral("HTTP 请求调试"),
        QStringLiteral("支持请求头编辑、分组历史记录；可加载、删除、复制为 cURL。"),
        this));

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setChildrenCollapsible(false);

    auto *historyPanel = new QWidget(splitter);
    historyPanel->setObjectName(QStringLiteral("httpHistoryPanel"));
    historyPanel->setAttribute(Qt::WA_StyledBackground, true);
    historyPanel->setMinimumWidth(200);
    auto *historyLayout = new QVBoxLayout(historyPanel);
    historyLayout->setContentsMargins(PageLayout::Space12, PageLayout::Space12,
                                      PageLayout::Space12, PageLayout::Space12);
    historyLayout->setSpacing(PageLayout::Space8);
    historyLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("历史记录（按分组）"), historyPanel));
    m_historyTree = new QTreeWidget(historyPanel);
    m_historyTree->setHeaderHidden(true);
    historyLayout->addWidget(m_historyTree, 1);
    auto *historyButtons = new QWidget(historyPanel);
    auto *historyButtonsLayout = new QHBoxLayout(historyButtons);
    historyButtonsLayout->setContentsMargins(0, 0, 0, 0);
    historyButtonsLayout->setSpacing(PageLayout::Space6);
    auto *loadButton = makeButton(QStringLiteral("加载"), historyButtons);
    auto *deleteButton = makeButton(QStringLiteral("删除"), historyButtons);
    auto *clearButton = makeButton(QStringLiteral("清空"), historyButtons);
    historyButtonsLayout->addWidget(loadButton);
    historyButtonsLayout->addWidget(deleteButton);
    historyButtonsLayout->addWidget(clearButton);
    historyLayout->addWidget(historyButtons);

    auto *requestPanel = new QWidget(splitter);
    auto *requestLayout = new QVBoxLayout(requestPanel);
    requestLayout->setContentsMargins(0, 0, 0, 0);
    requestLayout->setSpacing(PageLayout::Space8);

    auto *topRow = new QWidget(requestPanel);
    auto *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(PageLayout::Space8);
    m_method = new QComboBox(topRow);
    m_method->addItems({QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
                        QStringLiteral("DELETE"), QStringLiteral("PATCH"), QStringLiteral("HEAD")});
    PageLayout::configureFormInput(m_method);
    m_url = new QLineEdit(topRow);
    m_url->setPlaceholderText(QStringLiteral("https://example.com/api"));
    PageLayout::configureFormInput(m_url);
    m_sendButton = makeButton(QStringLiteral("发送"), topRow);
    topLayout->addWidget(m_method);
    topLayout->addWidget(m_url, 1);
    topLayout->addWidget(m_sendButton);
    requestLayout->addWidget(topRow);

    auto *groupRow = new QWidget(requestPanel);
    auto *groupLayout = new QHBoxLayout(groupRow);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(PageLayout::Space8);
    groupLayout->addWidget(new QLabel(QStringLiteral("分组"), groupRow));
    m_group = new QLineEdit(groupRow);
    m_group->setText(QStringLiteral("默认分组"));
    PageLayout::configureFormInput(m_group);
    auto *copyCurlButton = makeButton(QStringLiteral("复制为 cURL"), groupRow);
    groupLayout->addWidget(m_group, 1);
    groupLayout->addWidget(copyCurlButton);
    requestLayout->addWidget(groupRow);

    requestLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("请求头"), requestPanel));
    m_headers = new QTableWidget(requestPanel);
    m_headers->setColumnCount(2);
    m_headers->setHorizontalHeaderLabels({QStringLiteral("Header"), QStringLiteral("Value")});
    m_headers->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_headers->horizontalHeader()->setStretchLastSection(true);
    m_headers->setMaximumHeight(160);
    requestLayout->addWidget(m_headers);
    auto *headerButtons = new QWidget(requestPanel);
    auto *headerButtonsLayout = new QHBoxLayout(headerButtons);
    headerButtonsLayout->setContentsMargins(0, 0, 0, 0);
    headerButtonsLayout->setSpacing(PageLayout::Space6);
    auto *addHeaderButton = makeButton(QStringLiteral("添加头"), headerButtons);
    auto *removeHeaderButton = makeButton(QStringLiteral("删除选中头"), headerButtons);
    headerButtonsLayout->addWidget(addHeaderButton);
    headerButtonsLayout->addWidget(removeHeaderButton);
    headerButtonsLayout->addStretch();
    requestLayout->addWidget(headerButtons);

    requestLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("请求 Body"), requestPanel));
    m_body = new QPlainTextEdit(requestPanel);
    m_body->setPlaceholderText(QStringLiteral("请求 Body，GET 可留空"));
    m_body->setMinimumHeight(100);
    requestLayout->addWidget(m_body, 1);

    requestLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("响应"), requestPanel));
    m_response = new QPlainTextEdit(requestPanel);
    m_response->setReadOnly(true);
    m_response->setMinimumHeight(140);
    requestLayout->addWidget(m_response, 1);

    m_status = new QLabel(requestPanel);
    m_status->setObjectName(QStringLiteral("toolMessage"));
    requestLayout->addWidget(m_status);

    splitter->addWidget(historyPanel);
    splitter->addWidget(requestPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setSizes({260, 760});
    layout->addWidget(splitter, 1);

    addHeaderRow(QStringLiteral("Content-Type"), QStringLiteral("application/json"));

    connect(addHeaderButton, &QPushButton::clicked, this, [this]() { addHeaderRow(); });
    connect(removeHeaderButton, &QPushButton::clicked, this, [this]() {
        const int row = m_headers->currentRow();
        if (row >= 0) {
            m_headers->removeRow(row);
        }
    });
    connect(m_sendButton, &QPushButton::clicked, this, &HttpRequestWidget::sendRequest);

    connect(copyCurlButton, &QPushButton::clicked, this, [this]() {
        QStringList parts;
        parts.append(QStringLiteral("curl -X %1").arg(m_method->currentText()));
        const QString serialized = serializeHeaders();
        const QStringList headerLines = serialized.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &line : headerLines) {
            parts.append(QStringLiteral("-H '%1'").arg(line));
        }
        const QString body = m_body->toPlainText();
        if (!body.isEmpty()) {
            parts.append(QStringLiteral("--data '%1'").arg(body));
        }
        parts.append(QStringLiteral("'%1'").arg(m_url->text().trimmed()));
        if (QClipboard *clipboard = QApplication::clipboard()) {
            clipboard->setText(parts.join(QLatin1Char(' ')));
        }
        m_status->setText(QStringLiteral("已复制 cURL 命令"));
    });

    connect(loadButton, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem *item = m_historyTree->currentItem();
        if (item == nullptr) {
            return;
        }
        const int index = item->data(0, Qt::UserRole).toInt();
        if (index >= 0 && index < m_history.size()) {
            applyEntry(m_history.at(index));
        }
    });
    connect(m_historyTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int) {
        if (item == nullptr) {
            return;
        }
        const QVariant data = item->data(0, Qt::UserRole);
        if (!data.isValid()) {
            return;
        }
        const int index = data.toInt();
        if (index >= 0 && index < m_history.size()) {
            applyEntry(m_history.at(index));
        }
    });
    connect(deleteButton, &QPushButton::clicked, this, [this]() {
        QTreeWidgetItem *item = m_historyTree->currentItem();
        if (item == nullptr) {
            return;
        }
        const QVariant data = item->data(0, Qt::UserRole);
        if (!data.isValid()) {
            return;
        }
        const int index = data.toInt();
        if (index >= 0 && index < m_history.size()) {
            m_history.removeAt(index);
            saveHistory();
            refreshHistoryTree();
        }
    });
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        m_history.clear();
        saveHistory();
        refreshHistoryTree();
    });

    loadHistory();
    refreshHistoryTree();
}

void HttpRequestWidget::addHeaderRow(const QString &key, const QString &value)
{
    const int row = m_headers->rowCount();
    m_headers->insertRow(row);
    m_headers->setItem(row, 0, new QTableWidgetItem(key));
    m_headers->setItem(row, 1, new QTableWidgetItem(value));
}

QString HttpRequestWidget::serializeHeaders() const
{
    QStringList lines;
    for (int row = 0; row < m_headers->rowCount(); ++row) {
        const QTableWidgetItem *keyItem = m_headers->item(row, 0);
        const QTableWidgetItem *valueItem = m_headers->item(row, 1);
        const QString key = keyItem != nullptr ? keyItem->text().trimmed() : QString();
        const QString value = valueItem != nullptr ? valueItem->text() : QString();
        if (!key.isEmpty()) {
            lines.append(QStringLiteral("%1: %2").arg(key, value));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

void HttpRequestWidget::applyHeaders(const QString &serialized)
{
    m_headers->setRowCount(0);
    const QStringList lines = serialized.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        addHeaderRow(line.left(colon).trimmed(), line.mid(colon + 1).trimmed());
    }
}

HttpHistoryEntry HttpRequestWidget::currentEntry() const
{
    HttpHistoryEntry entry;
    entry.group = m_group->text().trimmed().isEmpty() ? QStringLiteral("默认分组") : m_group->text().trimmed();
    entry.method = m_method->currentText();
    entry.url = m_url->text().trimmed();
    entry.headers = serializeHeaders();
    entry.body = m_body->toPlainText();
    entry.name = QStringLiteral("%1 %2").arg(entry.method, entry.url);
    return entry;
}

void HttpRequestWidget::applyEntry(const HttpHistoryEntry &entry)
{
    const int methodIndex = m_method->findText(entry.method);
    if (methodIndex >= 0) {
        m_method->setCurrentIndex(methodIndex);
    }
    m_url->setText(entry.url);
    if (!entry.group.isEmpty()) {
        m_group->setText(entry.group);
    }
    applyHeaders(entry.headers);
    m_body->setPlainText(entry.body);
    m_status->setText(QStringLiteral("已加载历史请求"));
}

void HttpRequestWidget::sendRequest()
{
    const QUrl requestUrl(m_url->text().trimmed());
    if (!requestUrl.isValid() || requestUrl.scheme().isEmpty()) {
        m_response->setPlainText(QStringLiteral("URL 无效"));
        return;
    }

    auto *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(requestUrl);
    const QStringList headerLines = serializeHeaders().split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : headerLines) {
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon < 0) {
            continue;
        }
        request.setRawHeader(line.left(colon).trimmed().toUtf8(), line.mid(colon + 1).trimmed().toUtf8());
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
    m_response->setPlainText(QStringLiteral("请求中..."));
    m_status->setText(QStringLiteral("请求已发送"));

    // Save to history immediately so the request is recorded regardless of result.
    m_history.prepend(currentEntry());
    if (m_history.size() > 100) {
        m_history.resize(100);
    }
    saveHistory();
    refreshHistoryTree();

    QObject::connect(reply, &QNetworkReply::finished, this,
                     [this, reply, manager]() {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QStringList lines;
        lines.append(QStringLiteral("HTTP %1").arg(status == 0 ? QStringLiteral("-") : QString::number(status)));
        if (reply->error() != QNetworkReply::NoError) {
            lines.append(QStringLiteral("错误: %1").arg(reply->errorString()));
        }
        lines.append(QString());
        const auto headerPairs = reply->rawHeaderPairs();
        for (const auto &pair : headerPairs) {
            lines.append(QStringLiteral("%1: %2")
                .arg(QString::fromUtf8(pair.first), QString::fromUtf8(pair.second)));
        }
        lines.append(QString());
        lines.append(QString::fromUtf8(reply->readAll()));
        m_response->setPlainText(lines.join(QLatin1Char('\n')));
        m_status->setText(QStringLiteral("完成"));
        m_sendButton->setEnabled(true);
        reply->deleteLater();
        manager->deleteLater();
    });
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
        entry.body = object.value(QStringLiteral("body")).toString();
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
        object.insert(QStringLiteral("body"), entry.body);
        array.append(object);
    }
    QFile file(historyFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    }
}

void HttpRequestWidget::refreshHistoryTree()
{
    m_historyTree->clear();
    QHash<QString, QTreeWidgetItem *> groups;
    for (int i = 0; i < m_history.size(); ++i) {
        const HttpHistoryEntry &entry = m_history.at(i);
        const QString groupName = entry.group.isEmpty() ? QStringLiteral("默认分组") : entry.group;
        QTreeWidgetItem *groupItem = groups.value(groupName, nullptr);
        if (groupItem == nullptr) {
            groupItem = new QTreeWidgetItem(m_historyTree);
            groupItem->setText(0, groupName);
            groups.insert(groupName, groupItem);
        }
        auto *child = new QTreeWidgetItem(groupItem);
        child->setText(0, entry.name.isEmpty()
            ? QStringLiteral("%1 %2").arg(entry.method, entry.url)
            : entry.name);
        child->setData(0, Qt::UserRole, i);
    }
    m_historyTree->expandAll();
}
