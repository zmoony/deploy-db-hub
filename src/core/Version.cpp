#include "core/Version.h"

#include <QRegularExpression>

namespace Version {

QString fromDateTime(const QDateTime &dateTime)
{
    return dateTime.toString(QStringLiteral("yyyyMMdd-HHmmss"));
}

bool isValid(const QString &version)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9]{8}-[0-9]{6}$"));
    return pattern.match(version).hasMatch();
}

}
