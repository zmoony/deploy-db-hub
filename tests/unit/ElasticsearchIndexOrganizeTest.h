#pragma once

#include <QObject>

class ElasticsearchIndexOrganizeTest final : public QObject
{
    Q_OBJECT

private slots:
    void groupsTemplateSubIndices();
    void keepsStandaloneIndexFlat();
};
