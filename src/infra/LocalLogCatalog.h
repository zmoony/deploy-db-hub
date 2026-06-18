#pragma once

#include <QString>
#include <QStringList>

class LocalLogCatalog final
{
public:
    static bool isValidRelativePath(const QString &relativePath);
    static QString absolutePath(const QString &relativePath);
    static QStringList listRelativeLogFiles();
    static QString loadContent(const QString &relativePath, QString *error);
    static bool canOpen(const QString &relativePath);
};
