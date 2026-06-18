#pragma once

#include <QString>

class LogPath final
{
public:
    static bool isValidDeployId(const QString &deployId);
    static bool isValidRelativePath(const QString &relativePath);
    static QString relativePath(const QString &deployId);
    static QString absolutePath(const QString &dataDir, const QString &relativePath);
};
