#include "adapters/ssh/SshRemoteExecutor.h"

#include "adapters/ssh/SshClient.h"

SshRemoteExecutor::SshRemoteExecutor(RemoteConnectionContext context)
    : m_context(std::move(context))
{
}

RemoteCommandResult SshRemoteExecutor::testConnection()
{
    return execute(QStringLiteral("echo deploy-hub-ping"), 10);
}

RemoteCommandResult SshRemoteExecutor::execute(const QString &command, int timeoutSec)
{
    SshClient client(m_context);
    return client.execute(command, timeoutSec);
}

UploadResult SshRemoteExecutor::uploadFile(const QString &localPath, const QString &remotePath)
{
    return uploadFile(localPath, remotePath, true);
}

UploadResult SshRemoteExecutor::uploadFile(const QString &localPath,
                                           const QString &remotePath,
                                           bool ensureRemoteDir)
{
    SshClient client(m_context);
    return client.uploadFile(localPath, remotePath, ensureRemoteDir);
}
