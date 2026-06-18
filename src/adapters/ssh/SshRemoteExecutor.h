#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteExecutor.h"

class SshRemoteExecutor final : public RemoteExecutor
{
public:
    explicit SshRemoteExecutor(RemoteConnectionContext context);

    RemoteCommandResult testConnection() override;
    RemoteCommandResult execute(const QString &command, int timeoutSec) override;
    UploadResult uploadFile(const QString &localPath, const QString &remotePath) override;

private:
    RemoteConnectionContext m_context;
};
