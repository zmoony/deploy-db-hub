#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteFileBrowser.h"

#include "adapters/ssh/SshClient.h"

#include <QJsonObject>

class SftpRemoteFileBrowser final : public RemoteFileBrowser
{
public:
    SftpRemoteFileBrowser(RemoteConnectionContext context, HostKeyPromptHandler hostKeyPrompt = {});

    RemoteFileListResult listDirectory(const QString &remotePath) override;
    RemoteFileOperationResult createDirectory(const QString &remotePath) override;
    RemoteFileUploadResult uploadFile(const QString &localPath, const QString &remotePath) override;
    RemoteFileReadResult readFile(const QString &remotePath) override;
    RemoteFileReadResult readFileTail(const QString &remotePath, int lineCount) override;
    RemoteFileWriteResult writeFile(const QString &remotePath, const QString &content) override;
    RemoteFileOperationResult deleteEntry(const QString &remotePath) override;
    RemoteFileOperationResult renameEntry(const QString &sourcePath, const QString &destPath) override;
    RemoteFileOperationResult copyEntry(const QString &sourcePath, const QString &destPath) override;

private:
    bool operationEnabled(const QString &name) const;

    RemoteConnectionContext m_context;
    HostKeyPromptHandler m_hostKeyPrompt;
    QJsonObject m_remoteFiles;
    SshClient m_client;
};
