#include "adapters/winrm/WinRmLogFileBrowser.h"

namespace {

QString unsupported()
{
    return QStringLiteral("Windows 远程日志查看仅支持读取日志文件。");
}

}

WinRmLogFileBrowser::WinRmLogFileBrowser(RemoteConnectionContext context)
    : m_client(std::move(context))
{
}

RemoteFileListResult WinRmLogFileBrowser::listDirectory(const QString &remotePath)
{
    Q_UNUSED(remotePath);
    return {false, {}, {}, unsupported()};
}

RemoteFileOperationResult WinRmLogFileBrowser::createDirectory(const QString &remotePath)
{
    Q_UNUSED(remotePath);
    return {false, unsupported()};
}

RemoteFileUploadResult WinRmLogFileBrowser::uploadFile(const QString &localPath, const QString &remotePath)
{
    Q_UNUSED(localPath);
    Q_UNUSED(remotePath);
    return {false, 0, 0, {}, unsupported()};
}

RemoteFileReadResult WinRmLogFileBrowser::readFile(const QString &remotePath)
{
    return m_client.readFile(remotePath);
}

RemoteFileReadResult WinRmLogFileBrowser::readFileTail(const QString &remotePath, int lineCount)
{
    return m_client.readFileTail(remotePath, lineCount);
}

RemoteFileWriteResult WinRmLogFileBrowser::writeFile(const QString &remotePath, const QString &content)
{
    Q_UNUSED(remotePath);
    Q_UNUSED(content);
    return {false, {}, unsupported()};
}

RemoteFileOperationResult WinRmLogFileBrowser::deleteEntry(const QString &remotePath)
{
    Q_UNUSED(remotePath);
    return {false, unsupported()};
}

RemoteFileOperationResult WinRmLogFileBrowser::renameEntry(const QString &sourcePath, const QString &destPath)
{
    Q_UNUSED(sourcePath);
    Q_UNUSED(destPath);
    return {false, unsupported()};
}

RemoteFileOperationResult WinRmLogFileBrowser::copyEntry(const QString &sourcePath, const QString &destPath)
{
    Q_UNUSED(sourcePath);
    Q_UNUSED(destPath);
    return {false, unsupported()};
}
