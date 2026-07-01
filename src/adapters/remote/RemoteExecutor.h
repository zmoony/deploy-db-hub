#pragma once

#include "adapters/remote/RemoteConnection.h"

#include <memory>
#include <QString>

struct RemoteCommandResult {
    bool ok = false;
    int exitCode = -1;
    QString stdoutText;
    QString stderrText;
    QString error;
};

struct UploadResult {
    bool ok = false;
    qint64 bytesSent = 0;
    qint64 totalBytes = 0;
    QString error;
};

class RemoteExecutor
{
public:
    virtual ~RemoteExecutor() = default;

    virtual RemoteCommandResult testConnection() = 0;
    virtual RemoteCommandResult execute(const QString &command, int timeoutSec) = 0;
    virtual UploadResult uploadFile(const QString &localPath, const QString &remotePath) = 0;
    virtual UploadResult uploadFile(const QString &localPath,
                                   const QString &remotePath,
                                   bool ensureRemoteDir) = 0;
};

std::unique_ptr<RemoteExecutor> createRemoteExecutor(const RemoteConnectionContext &context);
