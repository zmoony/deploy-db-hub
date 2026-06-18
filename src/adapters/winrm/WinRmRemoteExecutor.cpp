#include "adapters/winrm/WinRmRemoteExecutor.h"

#include "adapters/winrm/WinRmClient.h"

WinRmRemoteExecutor::WinRmRemoteExecutor(RemoteConnectionContext context)
    : m_context(std::move(context))
{
}

RemoteCommandResult WinRmRemoteExecutor::testConnection()
{
    return execute(QStringLiteral("hostname"), 15);
}

RemoteCommandResult WinRmRemoteExecutor::execute(const QString &command, int timeoutSec)
{
    WinRmClient client(m_context);
    return client.execute(command, timeoutSec);
}

UploadResult WinRmRemoteExecutor::uploadFile(const QString &localPath, const QString &remotePath)
{
    WinRmClient client(m_context);
    return client.uploadFile(localPath, remotePath);
}
