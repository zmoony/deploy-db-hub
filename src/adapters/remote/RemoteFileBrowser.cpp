#include "adapters/remote/RemoteFileBrowser.h"

#include "adapters/ssh/SftpRemoteFileBrowser.h"
#include "adapters/winrm/WinRmLogFileBrowser.h"

std::unique_ptr<RemoteFileBrowser> createRemoteFileBrowser(const RemoteConnectionContext &context,
                                                           HostKeyPromptHandler hostKeyPrompt)
{
    const QString os = context.serverConfig.value(QStringLiteral("os")).toString();
    if (os != QStringLiteral("linux")) {
        return nullptr;
    }
    return std::make_unique<SftpRemoteFileBrowser>(context, std::move(hostKeyPrompt));
}

std::unique_ptr<RemoteFileBrowser> createRemoteLogFileBrowser(const RemoteConnectionContext &context,
                                                              HostKeyPromptHandler hostKeyPrompt)
{
    const QString os = context.serverConfig.value(QStringLiteral("os")).toString();
    if (os == QStringLiteral("linux")) {
        return std::make_unique<SftpRemoteFileBrowser>(context, std::move(hostKeyPrompt));
    }
    if (os == QStringLiteral("windows")) {
        Q_UNUSED(hostKeyPrompt);
        return std::make_unique<WinRmLogFileBrowser>(context);
    }
    return nullptr;
}
