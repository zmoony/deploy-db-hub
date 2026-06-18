#pragma once

#include <functional>
#include <memory>

#include <QString>
#include <QVector>

#include "adapters/remote/RemoteConnection.h"

enum class RemoteFileType {
    Directory,
    File,
    Symlink,
    Unknown
};

struct RemoteFileEntry {
    QString name;
    QString path;
    RemoteFileType type = RemoteFileType::Unknown;
    qint64 sizeBytes = -1;
    bool writable = false;
};

struct RemoteFileListResult {
    bool ok = false;
    QString currentPath;
    QVector<RemoteFileEntry> entries;
    QString error;
};

struct RemoteFileOperationResult {
    bool ok = false;
    QString error;
};

struct RemoteFileUploadResult {
    bool ok = false;
    qint64 bytesSent = 0;
    qint64 totalBytes = 0;
    QString remotePath;
    QString error;
};

struct RemoteFileReadResult {
    bool ok = false;
    QString content;
    QString remotePath;
    QString error;
};

struct RemoteFileWriteResult {
    bool ok = false;
    QString remotePath;
    QString error;
};

class RemoteFileBrowser
{
public:
    virtual ~RemoteFileBrowser() = default;

    virtual RemoteFileListResult listDirectory(const QString &remotePath) = 0;
    virtual RemoteFileOperationResult createDirectory(const QString &remotePath) = 0;
    virtual RemoteFileUploadResult uploadFile(const QString &localPath, const QString &remotePath) = 0;
    virtual RemoteFileReadResult readFile(const QString &remotePath) = 0;
    virtual RemoteFileReadResult readFileTail(const QString &remotePath, int lineCount) = 0;
    virtual RemoteFileWriteResult writeFile(const QString &remotePath, const QString &content) = 0;
    virtual RemoteFileOperationResult deleteEntry(const QString &remotePath) = 0;
    virtual RemoteFileOperationResult renameEntry(const QString &sourcePath, const QString &destPath) = 0;
    virtual RemoteFileOperationResult copyEntry(const QString &sourcePath, const QString &destPath) = 0;
};

std::unique_ptr<RemoteFileBrowser> createRemoteFileBrowser(const RemoteConnectionContext &context,
                                                           HostKeyPromptHandler hostKeyPrompt = {});
std::unique_ptr<RemoteFileBrowser> createRemoteLogFileBrowser(const RemoteConnectionContext &context,
                                                              HostKeyPromptHandler hostKeyPrompt = {});
