#include "adapters/remote/StubRemoteFileBrowser.h"

#include <QFileInfo>

namespace {

bool operationEnabled(const QJsonObject &remoteFiles, const QString &name)
{
    const QJsonObject operations = remoteFiles.value(QStringLiteral("operations")).toObject();
    return operations.value(name).toBool(true);
}

}

StubRemoteFileBrowser::StubRemoteFileBrowser(const QJsonObject &serverConfig)
    : m_serverConfig(serverConfig)
    , m_remoteFiles(serverConfig.value(QStringLiteral("remoteFiles")).toObject())
    , m_vfs(serverConfig)
{
}

bool StubRemoteFileBrowser::operationEnabled(const QString &name) const
{
    return ::operationEnabled(m_remoteFiles, name);
}

RemoteFileListResult StubRemoteFileBrowser::listDirectory(const QString &remotePath)
{
    if (!m_remoteFiles.value(QStringLiteral("enabled")).toBool(true)) {
        return {false, {}, {}, QStringLiteral("remote file browser is disabled in server config")};
    }
    if (!operationEnabled(QStringLiteral("listDir"))) {
        return {false, {}, {}, QStringLiteral("listDir operation is disabled in server config")};
    }
    return m_vfs.listDirectory(remotePath);
}

RemoteFileOperationResult StubRemoteFileBrowser::createDirectory(const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("mkdir"))) {
        return {false, QStringLiteral("mkdir operation is disabled in server config")};
    }
    return m_vfs.createDirectory(remotePath);
}

RemoteFileUploadResult StubRemoteFileBrowser::uploadFile(const QString &localPath, const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("upload"))) {
        return {false, 0, 0, {}, QStringLiteral("upload operation is disabled in server config")};
    }

    const qint64 maxBytes = m_remoteFiles.value(QStringLiteral("maxUploadSizeMb")).toInt(500) * 1024LL * 1024LL;
    QFileInfo localFile(localPath);
    if (localFile.size() > maxBytes) {
        return {false, 0, 0, {}, QStringLiteral("file exceeds maxUploadSizeMb limit")};
    }
    return m_vfs.uploadFile(localPath, remotePath);
}

RemoteFileReadResult StubRemoteFileBrowser::readFile(const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("read"))) {
        return {false, {}, {}, QStringLiteral("read operation is disabled in server config")};
    }
    return m_vfs.readFile(remotePath);
}

RemoteFileReadResult StubRemoteFileBrowser::readFileTail(const QString &remotePath, int lineCount)
{
    if (!operationEnabled(QStringLiteral("read"))) {
        return {false, {}, {}, QStringLiteral("read operation is disabled in server config")};
    }
    return m_vfs.readFileTail(remotePath, lineCount);
}

RemoteFileWriteResult StubRemoteFileBrowser::writeFile(const QString &remotePath, const QString &content)
{
    if (!operationEnabled(QStringLiteral("write"))) {
        return {false, {}, QStringLiteral("write operation is disabled in server config")};
    }
    return m_vfs.writeFile(remotePath, content);
}

RemoteFileOperationResult StubRemoteFileBrowser::deleteEntry(const QString &remotePath)
{
    if (!operationEnabled(QStringLiteral("delete"))) {
        return {false, QStringLiteral("delete operation is disabled in server config")};
    }
    return m_vfs.deleteEntry(remotePath);
}

RemoteFileOperationResult StubRemoteFileBrowser::renameEntry(const QString &sourcePath, const QString &destPath)
{
    Q_UNUSED(sourcePath);
    Q_UNUSED(destPath);
    if (!operationEnabled(QStringLiteral("rename"))) {
        return {false, QStringLiteral("rename operation is disabled in server config")};
    }
    return {false, QStringLiteral("rename is not supported by stub remote vfs")};
}

RemoteFileOperationResult StubRemoteFileBrowser::copyEntry(const QString &sourcePath, const QString &destPath)
{
    Q_UNUSED(sourcePath);
    Q_UNUSED(destPath);
    return {false, QStringLiteral("copy is not supported by stub remote vfs")};
}
