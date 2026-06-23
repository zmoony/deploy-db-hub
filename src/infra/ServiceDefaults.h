#pragma once

#include <QString>

namespace ServiceDefaults {

QString installPath(const QString &productKey);
QString storagePath(const QString &productKey);
QString defaultInfo(const QString &productKey);

}
