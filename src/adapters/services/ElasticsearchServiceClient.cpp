#include "adapters/services/ElasticsearchServiceClient.h"

#include "infra/ServiceDefaults.h"

#include <QEventLoop>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
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
    QNetworkReply *indicesReply = manager.get(makeRequest(endpoint, QStringLiteral("_cat/indices?format=json&bytes=b&h=index,health,docs.count,store.size,is_closed")));
    const ServiceResult indicesResult = waitReply(&manager, indicesReply);
    indicesReply->deleteLater();
    if (!indicesResult.ok) {
        return indicesResult;
    }

    QNetworkReply *aliasReply = manager.get(makeRequest(endpoint, QStringLiteral("_cat/aliases?format=json&h=alias,index")));
    const ServiceResult aliasResult = waitReply(&manager, aliasReply);
    aliasReply->deleteLater();

    QSet<QString> aliasNames;
    QHash<QString, QStringList> aliasTargets;
    if (aliasResult.ok) {
        const QJsonDocument aliasDoc = QJsonDocument::fromJson(aliasResult.message.toUtf8());
        if (aliasDoc.isArray()) {
            for (const QJsonValue &v : aliasDoc.array()) {
                const QJsonObject item = v.toObject();
                const QString aliasName = item.value(QStringLiteral("alias")).toString();
                const QString indexName = item.value(QStringLiteral("index")).toString();
                if (aliasName.isEmpty() || indexName.isEmpty() || aliasName == indexName) {
                    continue;
                }
                aliasNames.insert(aliasName);
                aliasTargets[aliasName].append(indexName);
            }
        }
    }

    const QJsonDocument document = QJsonDocument::fromJson(indicesResult.message.toUtf8());
    if (!document.isArray()) {
        return {false, QStringLiteral("索引列表响应不是 JSON 数组"), {}, {}};
    }

    ServiceResult rows{true, {}, {}, {}};
    for (const QJsonValue &value : document.array()) {
        const QJsonObject item = value.toObject();
        const QString name = item.value(QStringLiteral("index")).toString();
        if (name.isEmpty() || name.startsWith(QLatin1Char('.'))) {
            continue;
        }
        const qint64 docsRaw = item.value(QStringLiteral("docs.count")).toString().toLongLong();
        const qint64 diskRaw = item.value(QStringLiteral("store.size")).toString().toLongLong();
        const bool isClosed = item.value(QStringLiteral("is_closed")).toString().trimmed()
                                  .compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), name},
            {QStringLiteral("status"), isClosed ? QStringLiteral("closed") : item.value(QStringLiteral("health")).toString()},
            {QStringLiteral("docs"), QString::number(docsRaw)},
            {QStringLiteral("disk"), humanBytes(diskRaw)},
            {QStringLiteral("docsRaw"), docsRaw},
            {QStringLiteral("diskRaw"), diskRaw},
            {QStringLiteral("kind"), QStringLiteral("index")}
        });
    }

    for (const QString &aliasName : aliasNames) {
        QStringList targets = aliasTargets.value(aliasName);
        std::sort(targets.begin(), targets.end(), [](const QString &a, const QString &b) {
            const int dateA = indexSuffix(a).isEmpty() ? 0 : 1;
            const int dateB = indexSuffix(b).isEmpty() ? 0 : 1;
            if (dateA != dateB) {
                return dateA > dateB;
            }
            return a < b;
        });
        const QString targetSummary = targets.size() > 3
            ? QStringLiteral("%1 等 %2 个").arg(targets.first()).arg(targets.size())
            : targets.join(QLatin1String(", "));
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), aliasName},
            {QStringLiteral("status"), QStringLiteral("alias")},
            {QStringLiteral("docs"), QStringLiteral("-")},
            {QStringLiteral("disk"), QStringLiteral("-")},
            {QStringLiteral("docsRaw"), 0},
            {QStringLiteral("diskRaw"), 0},
            {QStringLiteral("kind"), QStringLiteral("alias")},
            {QStringLiteral("targets"), targetSummary}
        });
    }

    return rows;
}

QVector<QJsonObject> ElasticsearchServiceClient::organizeIndexRows(const QVector<QJsonObject> &flatRows)
{
    QVector<QJsonObject> organized;
    QVector<QJsonObject> groupable;

    for (const QJsonObject &row : flatRows) {
        const QString kind = row.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("alias")) {
            QJsonObject aliasRow = row;
            aliasRow.insert(QStringLiteral("rowKind"), QStringLiteral("alias"));
            aliasRow.insert(QStringLiteral("queryIndex"), row.value(QStringLiteral("name")).toString());
            organized.append(aliasRow);
            continue;
        }
        groupable.append(row);
    }

    QHash<QString, QVector<QJsonObject>> groups;
    for (const QJsonObject &row : groupable) {
        const QString name = row.value(QStringLiteral("name")).toString();
        const QString base = indexBaseName(name);
        groups[base].append(row);
    }

    QStringList groupKeys = groups.keys();
    groupKeys.sort(Qt::CaseInsensitive);

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
        for (const QJsonObject &member : members) {
            docsRaw += member.value(QStringLiteral("docsRaw")).toVariant().toLongLong();
            diskRaw += member.value(QStringLiteral("diskRaw")).toVariant().toLongLong();
            status = worstHealth(status, member.value(QStringLiteral("status")).toString());
        }

        organized.append(QJsonObject{
            {QStringLiteral("rowKind"), QStringLiteral("group")},
            {QStringLiteral("groupKey"), groupKey},
            {QStringLiteral("name"), groupKey},
            {QStringLiteral("queryIndex"), groupKey + QStringLiteral("*")},
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
        {QStringLiteral("track_total_hits"), true},
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

ServiceResult ElasticsearchServiceClient::getIndexMapping(const ServiceEndpoint &endpoint,
                                                          const QString &index)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint,
                                                   QStringLiteral("%1/_mapping?pretty").arg(index)));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    return result;
}

ServiceResult ElasticsearchServiceClient::getIndexSettings(const ServiceEndpoint &endpoint,
                                                           const QString &index)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint,
                                                   QStringLiteral("%1/_settings?pretty&flat_settings=true").arg(index)));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    return result;
}

ServiceResult ElasticsearchServiceClient::getAliasesForIndex(const ServiceEndpoint &endpoint,
                                                             const QString &index)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint,
                                                   QStringLiteral("%1/_alias?pretty").arg(index)));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    return result;
}

ServiceResult ElasticsearchServiceClient::listIndexTemplates(const ServiceEndpoint &endpoint)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint,
                                                   QStringLiteral("_index_template?pretty&filter_path=index_templates.name,index_templates.index_template.index_patterns,index_templates.index_template.priority,index_templates.index_template.template.settings,index_templates.index_template.template.mappings")));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    return result;
}

ServiceResult ElasticsearchServiceClient::analyzeText(const ServiceEndpoint &endpoint,
                                                      const QString &index,
                                                      const QString &analyzer,
                                                      const QString &text)
{
    QJsonObject body{
        {QStringLiteral("text"), text}
    };
    if (analyzer.startsWith(QLatin1String("field:"))) {
        body.insert(QStringLiteral("field"), analyzer.mid(6));
    } else if (!analyzer.isEmpty()) {
        body.insert(QStringLiteral("analyzer"), analyzer);
    }
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.post(makeRequest(endpoint,
                                                    QStringLiteral("%1/_analyze?pretty").arg(index)),
                                        QJsonDocument(body).toJson(QJsonDocument::Compact));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    return result;
}

QStringList ElasticsearchServiceClient::listAnalyzersFromSettings(const QJsonObject &settingsRoot)
{
    QStringList result;
    if (settingsRoot.isEmpty()) {
        return result;
    }

    auto collectAnalyzers = [&result](const QJsonObject &analysis) {
        const QJsonObject analyzers = analysis.value(QStringLiteral("analyzer")).toObject();
        for (const QString &key : analyzers.keys()) {
            if (!result.contains(key)) {
                result.append(key);
            }
        }
    };

    const QJsonObject indexSettings = settingsRoot.value(QStringLiteral("settings")).toObject();
    if (!indexSettings.isEmpty()) {
        collectAnalyzers(indexSettings.value(QStringLiteral("index")).toObject().value(QStringLiteral("analysis")).toObject());
    }
    for (auto it = settingsRoot.begin(); it != settingsRoot.end(); ++it) {
        const QJsonObject perIndex = it.value().toObject();
        const QJsonObject settings = perIndex.value(QStringLiteral("settings")).toObject();
        const QJsonObject analysis = settings.value(QStringLiteral("index")).toObject().value(QStringLiteral("analysis")).toObject();
        collectAnalyzers(analysis);
    }
    result.removeDuplicates();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QString ElasticsearchServiceClient::kibanaUrl(const ServiceEndpoint &endpoint)
{
    const int port = endpoint.kibanaPort > 0
        ? endpoint.kibanaPort
        : ServiceDefaults::kibanaPort(QStringLiteral("elasticsearch"));
    return QStringLiteral("%1://%2:%3")
        .arg(baseScheme(endpoint), endpoint.host, QString::number(port));
}
