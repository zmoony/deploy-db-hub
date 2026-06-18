#pragma once

#include <QObject>

class LocalLogCatalogTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsLogsRelativePath();
    void rejectsInvalidRelativePath();
    void clearsLogFilesInDirectory();
};
