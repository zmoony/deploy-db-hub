#pragma once

#include "adapters/remote/RemoteFileBrowser.h"
#include "adapters/remote/StubRemoteVfs.h"

#include <QJsonObject>

class StubRemoteFileBrowser final : public RemoteFileBrowser
{
public:
    explicit StubRemoteFileBrowser(const QJsonObject &serverConfig);

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

    QJsonObject m_serverConfig;
    QJsonObject m_remoteFiles;
    StubRemoteVfs m_vfs;
};
