#include "adapters/services/ElasticsearchServiceClient.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

QNetworkRequest makeRequest(const ServiceEndpoint &endpoint, const QString &path)
{
    const QUrl url(QStringLiteral("http://%1:%2/%3").arg(endpoint.host).arg(endpoint.port).arg(path));
    QNetworkRequest request(url);
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
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

}

ServiceResult ElasticsearchServiceClient::ping(const ServiceEndpoint &endpoint)
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(makeRequest(endpoint, QString()));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    if (!result.ok) {
        return result;
    }
    return {true, QStringLiteral("集群可达"), {}, {}};
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
        rows.rows.append(QJsonObject{
            {QStringLiteral("name"), item.value(QStringLiteral("index")).toString()},
            {QStringLiteral("description"), item.value(QStringLiteral("index")).toString()},
            {QStringLiteral("status"), item.value(QStringLiteral("health")).toString()},
            {QStringLiteral("docs"), item.value(QStringLiteral("docs.count")).toString()},
            {QStringLiteral("disk"), humanBytes(item.value(QStringLiteral("store.size")).toString().toLongLong())}
        });
    }
    return rows;
}

ServiceResult ElasticsearchServiceClient::searchIndex(const ServiceEndpoint &endpoint, const QString &index, int size)
{
    QNetworkAccessManager manager;
    const QJsonObject body{
        {QStringLiteral("size"), size},
        {QStringLiteral("sort"), QJsonArray{QJsonObject{{QStringLiteral("_doc"), QStringLiteral("desc")}}}}
    };
    QNetworkRequest request = makeRequest(endpoint, QStringLiteral("%1/_search").arg(index));
    QNetworkReply *reply = manager.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    const ServiceResult result = waitReply(&manager, reply);
    reply->deleteLater();
    if (!result.ok) {
        return result;
    }

    const QJsonObject root = QJsonDocument::fromJson(result.message.toUtf8()).object();
    const QJsonArray hits = root.value(QStringLiteral("hits")).toObject().value(QStringLiteral("hits")).toArray();
    QStringList lines;
    for (const QJsonValue &hit : hits) {
        lines.append(QJsonDocument(hit.toObject().value(QStringLiteral("_source")).toObject())
                       .toJson(QJsonDocument::Compact));
    }
    return {true, lines.join(QStringLiteral("\n\n")), {}, {}};
}

QString ElasticsearchServiceClient::kibanaUrl(const ServiceEndpoint &endpoint)
{
    return QStringLiteral("http://%1:%2/app/kibana").arg(endpoint.host).arg(endpoint.port + 1);
}
