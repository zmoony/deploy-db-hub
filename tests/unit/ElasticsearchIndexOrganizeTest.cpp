#include "ElasticsearchIndexOrganizeTest.h"

#include "adapters/services/ElasticsearchServiceClient.h"

#include <QJsonObject>
#include <QTest>

namespace {

QJsonObject makeIndexRow(const QString &name, qint64 docs, qint64 disk, const QString &status = QStringLiteral("green"))
{
    return QJsonObject{
        {QStringLiteral("name"), name},
        {QStringLiteral("status"), status},
        {QStringLiteral("docs"), QString::number(docs)},
        {QStringLiteral("disk"), QString::number(disk)},
        {QStringLiteral("docsRaw"), docs},
        {QStringLiteral("diskRaw"), disk}
    };
}

}

void ElasticsearchIndexOrganizeTest::groupsTemplateSubIndices()
{
    const QVector<QJsonObject> flat{
        makeIndexRow(QStringLiteral("index_face202606"), 100, 200),
        makeIndexRow(QStringLiteral("index_face202603"), 50, 80),
        makeIndexRow(QStringLiteral("index_face"), 0, 2),
        makeIndexRow(QStringLiteral("index_person202603"), 10, 20)
    };

    const QVector<QJsonObject> organized = ElasticsearchServiceClient::organizeIndexRows(flat);
    QCOMPARE(organized.size(), 5);

    QCOMPARE(organized.at(0).value(QStringLiteral("rowKind")).toString(), QStringLiteral("group"));
    QCOMPARE(organized.at(0).value(QStringLiteral("name")).toString(), QStringLiteral("index_face"));
    QCOMPARE(organized.at(0).value(QStringLiteral("childCount")).toInt(), 3);
    QCOMPARE(organized.at(0).value(QStringLiteral("docsRaw")).toVariant().toLongLong(), 150LL);

    QCOMPARE(organized.at(1).value(QStringLiteral("rowKind")).toString(), QStringLiteral("child"));
    QCOMPARE(organized.at(1).value(QStringLiteral("name")).toString(), QStringLiteral("index_face"));
    QCOMPARE(organized.at(2).value(QStringLiteral("name")).toString(), QStringLiteral("index_face202603"));
    QCOMPARE(organized.at(3).value(QStringLiteral("name")).toString(), QStringLiteral("index_face202606"));

    QCOMPARE(organized.at(4).value(QStringLiteral("rowKind")).toString(), QStringLiteral("index"));
    QCOMPARE(organized.at(4).value(QStringLiteral("name")).toString(), QStringLiteral("index_person202603"));
}

void ElasticsearchIndexOrganizeTest::keepsStandaloneIndexFlat()
{
    const QVector<QJsonObject> flat{
        makeIndexRow(QStringLiteral("index_vehicle"), 0, 2),
        makeIndexRow(QStringLiteral("index_vehicle202603"), 889197, 290700000)
    };

    const QVector<QJsonObject> organized = ElasticsearchServiceClient::organizeIndexRows(flat);
    QCOMPARE(organized.size(), 3);
    QCOMPARE(organized.at(0).value(QStringLiteral("rowKind")).toString(), QStringLiteral("group"));
    QCOMPARE(organized.at(0).value(QStringLiteral("queryIndex")).toString(), QStringLiteral("index_vehicle*"));
    QCOMPARE(organized.at(1).value(QStringLiteral("name")).toString(), QStringLiteral("index_vehicle"));
    QCOMPARE(organized.at(2).value(QStringLiteral("name")).toString(), QStringLiteral("index_vehicle202603"));
}
