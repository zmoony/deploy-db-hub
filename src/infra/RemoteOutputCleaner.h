#pragma once

#include <QString>

namespace RemoteOutputCleaner {

QString stripSshBanner(const QString &text);
QString normalizeRemoteError(const QString &text);

}
