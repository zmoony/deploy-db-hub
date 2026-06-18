#include "adapters/remote/RemoteMonitor.h"

#include "adapters/ssh/SshRemoteMonitor.h"
#include "adapters/winrm/WinRmRemoteMonitor.h"

#include <memory>

std::unique_ptr<RemoteMonitor> createRemoteMonitor(const RemoteConnectionContext &context)
{
    const QString os = context.serverConfig.value(QStringLiteral("os")).toString();
    if (os == QStringLiteral("windows")) {
        return createWinRmRemoteMonitor(context);
    }
    return createSshRemoteMonitor(context);
}
