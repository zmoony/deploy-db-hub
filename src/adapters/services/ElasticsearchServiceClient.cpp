#include "adapters/services/ElasticsearchServiceClient.h"

#include "infra/ServiceDefaults.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

namespace {

QString baseScheme(const ServiceEndpoint &endpoint)
{
    if (endpoint.authFingerprint.contains(QStringLiteral("https"), Qt::CaseInsensitive)) {
        return QStringLiteral("https");
    }
    return QStringLiteral("http");
}

QUrl makeUrl(const ServiceEndpoint &endpoint, const QString &path)
{
    const QString normalizedPath = path.startsWith(QLatin1Char('/')) ? path.mid(1) : path;
    return QUrl(QStringLiteral("%1://%2:%3/%4")
                    .arg(baseScheme(endpoint), endpoint.host, QString::number(endpoint.port), normalizedPath));
}

QNetworkRequest makeRequest(const ServiceEndpoint &endpoint, const QString &path)
{
    QNetworkRequest request(makeUrl(endpoint, path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!endpoint.username.isEmpty()) {
        const QByteArray token = QStringLiteral("%1:%2")
                                     .arg(endpoint.username, endpoint.password)
                                     .toUtf8()
                                     .toBase64();
        request.setRawHeader("Authorization", "Basic " + token);
    }
    return request;
}

ServiceResult waitReply(QNetworkAccessManager *manager, QNetworkReply *reply)
{
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray body = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        return {false, reply->errorString() + QStringLiteral(": ") + QString::fromUtf8(body), {}, {}};
    }
    if (status >= 400) {
        return {false, QStringLiteral("HTTP %1: %2").arg(status).arg(QString::fromUtf8(body)), {}, {}};
    }
    return {true, QString::fromUtf8(body), {}, {}};
}

QString humanBytes(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    }
    if (bytes < 1024LL * 1024 * 1024) {
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

QString indexBaseName(const QString &name)
{
    static const QRegularExpression suffixPattern(QStringLiteral("20\\d{4}$"));
    const QRegularExpressionMatch match = suffixPattern.match(name);
    if (match.hasMatch()) {
        return name.left(name.size() - 6);
    }
    return name;
}

QString indexSuffix(const QString &name)
{
    static const QRegularExpression suffixPattern(QStringLiteral("(20\\d{4})$"));
    const QRegularExpressionMatch match = suffixPattern.match(name);
    return match.hasMatch() ? match.captured(1) : QString();
}

int healthRank(const QString &status)
{
    const QString normalized = status.trimmed().toLower();
    if (normalized == QStringLiteral("red")) {
        return 3;
    }
    if (normalized == QStringLiteral("yellow")) {
        return 2;
    }
    return 1;
}

QString worstHealth(const QString &left, const QString &right)
{
    return healthRank(left) >= healthRank(right) ? left : right;
}

}

ServiceResult ElasticsearchServiceClient::ping(const ServiceEndpoint &endpoint)
{
    const ServiceResult health = clusterHealth(endpoint);
    if (!health.ok) {
        return health;
    }
    return health;
}

ServiceResult ElasticsearchServiceClient::clusterHealth(const ServiceEndpoint &endpoint)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint, QStringLiteral("_cluster/health?level=indices")));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    if (!result.ok) {
        return result;
    }

    const QJsonObject root = QJsonDocument::fromJson(result.message.toUtf8()).object();
    const QString clusterName = root.value(QStringLiteral("cluster_name")).toString();
    const QString status = root.value(QStringLiteral("status")).toString();
    return {true,
            QStringLiteral("集群 %1 状态 %2").arg(clusterName, status),
            {},
            {QJsonObject{
                {QStringLiteral("cluster"), clusterName},
                {QStringLiteral("status"), status},
                {QStringLiteral("nodes"), root.value(QStringLiteral("number_of_nodes")).toVariant().toString()},
                {QStringLiteral("indices"), root.value(QStringLiteral("active_primary_shards")).toVariant().toString()}
            }}};
}

ServiceResult ElasticsearchServiceClient::listIndices(const ServiceEndpoint &endpoint)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint, QStringLiteral("_cat/indices?format=json&bytes=b")));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    if (!result.ok) {
        return result;
    }

    const QJsonDocument document = QJsonDocument::fromJson(result.message.toUtf8());
    if (!document.isArray()) {
        return {false, QStringLiteral("索引列表响应不是 JSON 数组"), {}, {}};
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QJsonValue &value : document.array()) {
        const QJsonObject item = value.toObject();
        const qint64 docsRaw = item.value(QStringLiteral("docs.count")).toString().toLongLong();
        const qint64 diskRaw = item.value(QStringLiteral("store.size")).toString().toLongLong();
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), item.value(QStringLiteral("index")).toString()},
            {QStringLiteral("status"), item.value(QStringLiteral("health")).toString()},
            {QStringLiteral("docs"), QString::number(docsRaw)},
            {QStringLiteral("disk"), humanBytes(diskRaw)},
            {QStringLiteral("docsRaw"), docsRaw},
            {QStringLiteral("diskRaw"), diskRaw}
        });
    }
    return rows;
}

QVector<QJsonObject> ElasticsearchServiceClient::organizeIndexRows(const QVector<QJsonObject> &flatRows)
{
    QHash<QString, QVector<QJsonObject>> groups;
    for (const QJsonObject &row : flatRows) {
        const QString name = row.value(QStringLiteral("name")).toString();
        const QString base = indexBaseName(name);
        groups[base].append(row);
    }

    QStringList groupKeys = groups.keys();
    groupKeys.sort(Qt::CaseInsensitive);

    QVector<QJsonObject> organized;
    for (const QString &groupKey : groupKeys) {
        QVector<QJsonObject> members = groups.value(groupKey);
        std::sort(members.begin(), members.end(), [](const QJsonObject &left, const QJsonObject &right) {
            const QString leftSuffix = indexSuffix(left.value(QStringLiteral("name")).toString());
            const QString rightSuffix = indexSuffix(right.value(QStringLiteral("name")).toString());
            if (leftSuffix.isEmpty() != rightSuffix.isEmpty()) {
                return leftSuffix.isEmpty();
            }
            return leftSuffix < rightSuffix;
        });

        if (members.size() == 1) {
            QJsonObject row = members.first();
            row.insert(QStringLiteral("rowKind"), QStringLiteral("index"));
            row.insert(QStringLiteral("queryIndex"), row.value(QStringLiteral("name")).toString());
            organized.append(row);
            continue;
        }

        qint64 docsRaw = 0;
        qint64 diskRaw = 0;
        QString status = members.first().value(QStringLiteral("status")).toString();
        QString queryIndex = members.first().value(QStringLiteral("name")).toString();
        for (const QJsonObject &member : members) {
            docsRaw += member.value(QStringLiteral("docsRaw")).toVariant().toLongLong();
            diskRaw += member.value(QStringLiteral("diskRaw")).toVariant().toLongLong();
            status = worstHealth(status, member.value(QStringLiteral("status")).toString());
            if (indexSuffix(member.value(QStringLiteral("name")).toString()).isEmpty()) {
                queryIndex = member.value(QStringLiteral("name")).toString();
            }
        }

        organized.append(QJsonObject{
            {QStringLiteral("rowKind"), QStringLiteral("group")},
            {QStringLiteral("groupKey"), groupKey},
            {QStringLiteral("name"), groupKey},
            {QStringLiteral("queryIndex"), queryIndex},
            {QStringLiteral("status"), status},
            {QStringLiteral("docs"), QString::number(docsRaw)},
            {QStringLiteral("disk"), humanBytes(diskRaw)},
            {QStringLiteral("docsRaw"), docsRaw},
            {QStringLiteral("diskRaw"), diskRaw},
            {QStringLiteral("childCount"), members.size()}
        });

        for (const QJsonObject &member : members) {
            QJsonObject child = member;
            child.insert(QStringLiteral("rowKind"), QStringLiteral("child"));
            child.insert(QStringLiteral("groupKey"), groupKey);
            child.insert(QStringLiteral("queryIndex"), member.value(QStringLiteral("name")).toString());
            organized.append(child);
        }
    }
    return organized;
}

ServiceResult ElasticsearchServiceClient::searchIndexBody(const ServiceEndpoint &endpoint,
                                                          const QString &index,
                                                          const QJsonObject &body)
{
    QNetworkAccessManager manager;
    QNetworkRequest request = makeRequest(endpoint, QStringLiteral("%1/_search").arg(index));
    QNetworkReply *reply = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    if (!result.ok) {
        return result;
    }

    const QJsonDocument document = QJsonDocument::fromJson(result.message.toUtf8());
    return {true, QString::fromUtf8(document.toJson(QJsonDocument::Indented)), {}, {}};
}

ServiceResult ElasticsearchServiceClient::searchIndex(const ServiceEndpoint &endpoint,
                                                      const QString &index,
                                                      int from,
                                                      int size,
                                                      const QString &queryText)
{
    QJsonObject body{
        {QStringLiteral("from"), qMax(0, from)},
        {QStringLiteral("size"), qMax(1, size)},
        {QStringLiteral("sort"), QJsonArray{QJsonObject{{QStringLiteral("_doc"), QStringLiteral("desc")}}}}
    };
    if (!queryText.trimmed().isEmpty()) {
        body.insert(QStringLiteral("query"),
                    QJsonObject{
                        {QStringLiteral("query_string"),
                         QJsonObject{{QStringLiteral("query"), queryText.trimmed()}}}
                    });
    } else {
        body.insert(QStringLiteral("query"), QJsonObject{{QStringLiteral("match_all"), QJsonObject{}}});
    }
    return searchIndexBody(endpoint, index, body);
}

ServiceResult ElasticsearchServiceClient::indexCount(const ServiceEndpoint &endpoint, const QString &index)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint, QStringLiteral("%1/_count").arg(index)));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    if (!result.ok) {
        return result;
    }
    const QJsonObject root = QJsonDocument::fromJson(result.message.toUtf8()).object();
    return {true, root.value(QStringLiteral("count")).toVariant().toString(), {}, {}};
}

QString ElasticsearchServiceClient::kibanaUrl(const ServiceEndpoint &endpoint)
{
    const int port = endpoint.kibanaPort > 0
        ? endpoint.kibanaPort
        : ServiceDefaults::kibanaPort(QStringLiteral("elasticsearch"));
    return QStringLiteral("%1://%2:%3")
        .arg(baseScheme(endpoint), endpoint.host, QString::number(port));
}
