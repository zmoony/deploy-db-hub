#pragma once

#include <QDateTime>
#include <QString>

namespace Version {
QString fromDateTime(const QDateTime &dateTime);
bool isValid(const QString &version);
}
