#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteExecutor.h"
#include "adapters/remote/RemoteFileBrowser.h"

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include <mutex>

struct SshInteractiveInvocation {
    QString program;
    QStringList args;
    QProcessEnvironment env;
};

class SshClient final
{
public:
    explicit SshClient(RemoteConnectionContext context, HostKeyPromptHandler hostKeyPrompt = {});
    ~SshClient();

    bool connect(QString *error);
    void disconnect();
    bool isConnected() const { return m_connected; }

    // 构造一个交互式 ssh -tt 调用（程序名/参数/环境），复用同一套认证与连接参数。
    // askpass 临时脚本会在本实例析构时清理，调用方需保活本 SshClient 直至进程结束。
    bool buildInteractiveInvocation(SshInteractiveInvocation *out, QString *error);

    RemoteCommandResult execute(const QString &command, int timeoutSec);
    UploadResult uploadFile(const QString &localPath, const QString &remotePath);
    UploadResult uploadFile(const QString &localPath, const QString &remotePath, bool ensureRemoteDir);
    RemoteFileListResult listDirectory(const QString &remotePath);
    RemoteFileReadResult readFile(const QString &remotePath);
    RemoteFileReadResult readFileTail(const QString &remotePath, int lineCount);
    RemoteFileWriteResult writeFile(const QString &remotePath, const QString &content);
    RemoteFileOperationResult createDirectory(const QString &remotePath);
    RemoteFileOperationResult deleteEntry(const QString &remotePath);
    RemoteFileOperationResult renameEntry(const QString &sourcePath, const QString &destPath);
    RemoteFileOperationResult copyEntry(const QString &sourcePath, const QString &destPath);

private:
    struct ProcessResult {
        bool ok = false;
        int exitCode = -1;
        QString stdoutText;
        QString stderrText;
        QString error;
    };

    QString sshProgram() const;
    QString scpProgram() const;
    QString sftpProgram() const;
    QString target() const;
    QStringList baseSshArgs() const;
    QStringList baseScpArgs() const;
    QString knownHostsPath() const;
    QString expandUserPath(const QString &path) const;
    QString normalizeRemotePath(const QString &path) const;
    QString shellQuote(const QString &value) const;
    QString remoteShellQuote(const QString &value) const;
    ProcessResult runProcess(const QString &program,
                             const QStringList &arguments,
                             int timeoutSec,
                             const QByteArray &stdinData = {}) const;
    ProcessResult runSsh(const QString &remoteCommand, int timeoutSec) const;
    ProcessResult runSftpBatch(const QStringList &commands, int timeoutSec) const;
    bool ensureAskPass(QString *error) const;
    void cleanupAskPass() const;
    bool ensureSession(QString *error);

    RemoteConnectionContext m_context;
    HostKeyPromptHandler m_hostKeyPrompt;
    bool m_connected = false;
    mutable QString m_askPassScriptPath;
    mutable std::recursive_mutex m_ioMutex;
};
