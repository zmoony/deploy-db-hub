#include "adapters/ssh/SftpRemoteFileBrowser.h"

#include "adapters/ssh/SshClient.h"

#include <QFileInfo>

SftpRemoteFileBrowser::SftpRemoteFileBrowser(RemoteConnectionContext context, HostKeyPromptHandler hostKeyPrompt)
    : m_context(std::move(context))
    , m_hostKeyPrompt(std::move(hostKeyPrompt))
    , m_remoteFiles(m_context.serverConfig.value(QStringLiteral("remoteFiles")).toObject())
    , m_client(m_context, m_hostKeyPrompt)
{
}

bool SftpRemoteFileBrowser::operationEnabled(const QString &name) const
{
    const QJsonObject operations = m_remoteFiles.value(QStringLiteral("operations")).toObject();
    return operations.value(name).toBool(true);
}

RemoteFileListResult SftpRemoteFileBrowser::listDirectory(const QString &remotePath)
{
    if (!m_remoteFiles.value(QStringLiteral("enabled")).toBool(true)) {
        return {false, {}, {}, QStringLiteral("remote file browser is disabled in server config")};
    }
    if (!operationEnabled(QStringLiteral("listDir"))) {
        return {false, {}, {}, QStringLiteral("listDir operation is disabled in server config")};
    }

    return m_client.listDirectory(remotePath);
}

RemoteFileOperationResult SftpRemoteFileBrowser::createDirectory(const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("mkdir"))) {
        return {false, QStringLiteral("mkdir operation is disabled in server config")};
    }
    return m_client.createDirectory(remotePath);
}

RemoteFileUploadResult SftpRemoteFileBrowser::uploadFile(const QString &localPath, const QString &remotePath)
{
    RemoteFileUploadResult result;
    if (!operationEnabled(QStringLiteral("upload"))) {
        result.error = QStringLiteral("upload operation is disabled in server config");
        return result;
    }

    const qint64 maxBytes = m_remoteFiles.value(QStringLiteral("maxUploadSizeMb")).toInt(500) * 1024LL * 1024LL;
    const QFileInfo localFile(localPath);
    if (localFile.size() > maxBytes) {
        result.error = QStringLiteral("file exceeds maxUploadSizeMb limit");
        return result;
    }

    const UploadResult upload = m_client.uploadFile(localPath, remotePath);
    result.ok = upload.ok;
    result.bytesSent = upload.bytesSent;
    result.totalBytes = upload.totalBytes;
    result.remotePath = remotePath;
    result.error = upload.error;
    return result;
}

RemoteFileReadResult SftpRemoteFileBrowser::readFile(const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("read"))) {
        return {false, {}, {}, QStringLiteral("read operation is disabled in server config")};
    }
    return m_client.readFile(remotePath);
}

RemoteFileReadResult SftpRemoteFileBrowser::readFileTail(const QString &remotePath, int lineCount)
{
    if (!operationEnabled(QStringLiteral("read"))) {
        return {false, {}, {}, QStringLiteral("read operation is disabled in server config")};
    }
    return m_client.readFileTail(remotePath, lineCount);
}

RemoteFileWriteResult SftpRemoteFileBrowser::writeFile(const QString &remotePath, const QString &content)
{
    if (!operationEnabled(QStringLiteral("write"))) {
        return {false, {}, QStringLiteral("write operation is disabled in server config")};
    }
    return m_client.writeFile(remotePath, content);
}

RemoteFileOperationResult SftpRemoteFileBrowser::deleteEntry(const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("delete"))) {
        return {false, QStringLiteral("delete operation is disabled in server config")};
    }
    return m_client.deleteEntry(remotePath);
}

RemoteFileOperationResult SftpRemoteFileBrowser::renameEntry(const QString &sourcePath, const QString &destPath)
{
    if (!operationEnabled(QStringLiteral("rename"))) {
        return {false, QStringLiteral("rename operation is disabled in server config")};
    }
    return m_client.renameEntry(sourcePath, destPath);
}

RemoteFileOperationResult SftpRemoteFileBrowser::copyEntry(const QString &sourcePath, const QString &destPath)
{
    return m_client.copyEntry(sourcePath, destPath);
}
