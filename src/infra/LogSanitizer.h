#pragma once

#include <QString>

class LogSanitizer final
{
public:
    static QString sanitize(const QString &text);
};
