#pragma once

#include <QString>
#include <QStringList>

struct LocalLogCleanupResult {
    int removedFiles = 0;
    int failedFiles = 0;
    qint64 freedBytes = 0;
};

class LocalLogCatalog final
{
public:
    static bool isValidRelativePath(const QString &relativePath);
    static QString absolutePath(const QString &relativePath);
    static QStringList listRelativeLogFiles();
    static QString loadContent(const QString &relativePath, QString *error);
    static bool canOpen(const QString &relativePath);
    static LocalLogCleanupResult clearDirectory(const QString &directory);
    static LocalLogCleanupResult clearAll();
};
