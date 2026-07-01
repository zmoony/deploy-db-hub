#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteExecutor.h"
#include "adapters/remote/RemoteFileBrowser.h"

#include <QString>

class WinRmClient final
{
public:
    explicit WinRmClient(RemoteConnectionContext context);

    RemoteCommandResult execute(const QString &command, int timeoutSec);
    UploadResult uploadFile(const QString &localPath, const QString &remotePath);
    UploadResult uploadFile(const QString &localPath,
                            const QString &remotePath,
                            bool ensureRemoteDir);
    RemoteFileReadResult readFile(const QString &remotePath);
    RemoteFileReadResult readFileTail(const QString &remotePath, int lineCount);

private:
    struct ProcessResult {
        bool ok = false;
        int exitCode = -1;
        QString stdoutText;
        QString stderrText;
        QString error;
    };

    QString endpoint() const;
    ProcessResult runWinRs(const QString &command, int timeoutSec) const;

    RemoteConnectionContext m_context;
};
