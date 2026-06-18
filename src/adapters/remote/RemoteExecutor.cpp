#include "adapters/remote/RemoteExecutor.h"

#include "adapters/ssh/SshRemoteExecutor.h"
#include "adapters/winrm/WinRmRemoteExecutor.h"

std::unique_ptr<RemoteExecutor> createRemoteExecutor(const RemoteConnectionContext &context)
{
    const QString os = context.serverConfig.value(QStringLiteral("os")).toString();
    if (os == QStringLiteral("linux")) {
        return std::make_unique<SshRemoteExecutor>(context);
    }
    if (os == QStringLiteral("windows")) {
        return std::make_unique<WinRmRemoteExecutor>(context);
    }
    return nullptr;
}
