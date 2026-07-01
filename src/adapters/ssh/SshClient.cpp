#include "adapters/ssh/SshClient.h"

#include "infra/DataPaths.h"
#include "infra/RemoteLogPath.h"
#include "infra/RemoteOutputCleaner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>

#include <mutex>

namespace {

bool isNavEntry(const QString &name)
{
    return name == QStringLiteral(".") || name == QStringLiteral("..");
}

QString joinRemotePath(const QString &parent, const QString &name)
{
    if (parent == QStringLiteral("/")) {
        return QStringLiteral("/") + name;
    }
    return parent + QLatin1Char('/') + name;
}

}

SshClient::SshClient(RemoteConnectionContext context, HostKeyPromptHandler hostKeyPrompt)
    : m_context(std::move(context))
    , m_hostKeyPrompt(std::move(hostKeyPrompt))
{
}

SshClient::~SshClient()
{
    cleanupAskPass();
}

QString SshClient::sshProgram() const
{
    return QStringLiteral("ssh");
}

QString SshClient::scpProgram() const
{
    return QStringLiteral("scp");
}

QString SshClient::sftpProgram() const
{
    return QStringLiteral("sftp");
}

QString SshClient::target() const
{
    const QString user = m_context.serverConfig.value(QStringLiteral("username")).toString();
    const QString host = m_context.serverConfig.value(QStringLiteral("host")).toString();
    return QStringLiteral("%1@%2").arg(user, host);
}

QString SshClient::knownHostsPath() const
{
    return QDir(DataPaths::configDir()).filePath(QStringLiteral("known_hosts"));
}

QString SshClient::expandUserPath(const QString &path) const
{
    if (!path.startsWith(QLatin1Char('~'))) {
        return path;
    }
    return QDir::home().filePath(path.mid(1));
}

QString SshClient::normalizeRemotePath(const QString &path) const
{
    QString normalized = path.trimmed();
    if (normalized.isEmpty()) {
        return QStringLiteral("/");
    }
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (normalized.size() > 1 && normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    return normalized;
}

QString SshClient::shellQuote(const QString &value) const
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'%1'").arg(escaped);
}

QString SshClient::remoteShellQuote(const QString &value) const
{
    return shellQuote(value);
}

bool SshClient::ensureAskPass(QString *error) const
{
    if (!m_askPassScriptPath.isEmpty()) {
        return true;
    }

    const QJsonObject auth = m_context.serverConfig.value(QStringLiteral("auth")).toObject();
    if (auth.value(QStringLiteral("mode")).toString() == QStringLiteral("ssh-key")) {
        return true;
    }
    if (m_context.password.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("未提供服务器密码");
        }
        return false;
    }

    auto *tempFile = new QTemporaryFile(QDir::temp().filePath(QStringLiteral("deploy-hub-askpass-XXXXXX.bat")));
    if (!tempFile->open()) {
        if (error != nullptr) {
            *error = QStringLiteral("无法创建 SSH_ASKPASS 脚本");
        }
        delete tempFile;
        return false;
    }

    QTextStream stream(tempFile);
    stream << QStringLiteral("@echo off\r\n");
    stream << QStringLiteral("echo %1\r\n").arg(m_context.password);
    tempFile->close();
    tempFile->setAutoRemove(false);
    m_askPassScriptPath = tempFile->fileName();
    delete tempFile;
    return true;
}

void SshClient::cleanupAskPass() const
{
    if (!m_askPassScriptPath.isEmpty()) {
        QFile::remove(m_askPassScriptPath);
        m_askPassScriptPath.clear();
    }
}

QStringList SshClient::baseSshArgs() const
{
    const int port = m_context.serverConfig.value(QStringLiteral("port")).toInt(22);
    const QJsonObject auth = m_context.serverConfig.value(QStringLiteral("auth")).toObject();
    const bool keyAuth = auth.value(QStringLiteral("mode")).toString() == QStringLiteral("ssh-key");

    QStringList args;
    args << QStringLiteral("-p") << QString::number(port);
    args << QStringLiteral("-o") << QStringLiteral("UserKnownHostsFile=%1").arg(knownHostsPath());
    args << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=accept-new");
    args << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=10");
    args << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=30");
#ifndef Q_OS_WIN
    const QString controlPath = QDir(DataPaths::configDir()).filePath(QStringLiteral("ssh-%C"));
    args << QStringLiteral("-o") << QStringLiteral("ControlMaster=auto");
    args << QStringLiteral("-o") << QStringLiteral("ControlPath=%1").arg(controlPath);
    args << QStringLiteral("-o") << QStringLiteral("ControlPersist=120");
#endif

    if (keyAuth) {
        const QString keyPath = expandUserPath(
            auth.value(QStringLiteral("sshPrivateKeyPath")).toString(QStringLiteral("~/.ssh/id_ed25519")));
        args << QStringLiteral("-i") << keyPath;
        args << QStringLiteral("-o") << QStringLiteral("IdentitiesOnly=yes");
    } else {
        args << QStringLiteral("-o") << QStringLiteral("PreferredAuthentications=password");
        args << QStringLiteral("-o") << QStringLiteral("PubkeyAuthentication=no");
        args << QStringLiteral("-o") << QStringLiteral("NumberOfPasswordPrompts=1");
    }

    return args;
}

SshClient::ProcessResult SshClient::runProcess(const QString &program,
                                               const QStringList &arguments,
                                               int timeoutSec,
                                               const QByteArray &stdinData) const
{
    ProcessResult result;
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    const QJsonObject auth = m_context.serverConfig.value(QStringLiteral("auth")).toObject();
    if (auth.value(QStringLiteral("mode")).toString() != QStringLiteral("ssh-key")) {
        if (!m_askPassScriptPath.isEmpty()) {
            env.insert(QStringLiteral("SSH_ASKPASS"), m_askPassScriptPath);
            env.insert(QStringLiteral("SSH_ASKPASS_REQUIRE"), QStringLiteral("force"));
            env.insert(QStringLiteral("DISPLAY"), QStringLiteral(":0"));
        }
    }

    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(program, arguments);

    if (!process.waitForStarted(10000)) {
        result.error = QStringLiteral("无法启动 %1：%2").arg(program, process.errorString());
        return result;
    }

    if (!stdinData.isEmpty()) {
        process.write(stdinData);
        process.closeWriteChannel();
    }

    if (!process.waitForFinished(qMax(1, timeoutSec) * 1000)) {
        process.kill();
        result.error = QStringLiteral("%1 执行超时（%2 秒）").arg(program).arg(timeoutSec);
        return result;
    }

    result.exitCode = process.exitCode();
    result.stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    result.stderrText = RemoteOutputCleaner::stripSshBanner(QString::fromUtf8(process.readAllStandardError()));
    result.ok = result.exitCode == 0;
    if (!result.ok && result.error.isEmpty()) {
        result.error = result.stderrText.trimmed().isEmpty()
            ? QStringLiteral("%1 退出码 %2").arg(program).arg(result.exitCode)
            : RemoteOutputCleaner::normalizeRemoteError(result.stderrText.trimmed());
    }
    return result;
}

QStringList SshClient::baseScpArgs() const
{
    const int port = m_context.serverConfig.value(QStringLiteral("port")).toInt(22);
    const QJsonObject auth = m_context.serverConfig.value(QStringLiteral("auth")).toObject();
    const bool keyAuth = auth.value(QStringLiteral("mode")).toString() == QStringLiteral("ssh-key");

    QStringList args;
    args << QStringLiteral("-P") << QString::number(port);
    args << QStringLiteral("-o") << QStringLiteral("UserKnownHostsFile=%1").arg(knownHostsPath());
    args << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=accept-new");
    args << QStringLiteral("-o") << QStringLiteral("ConnectTimeout=10");
    args << QStringLiteral("-o") << QStringLiteral("ServerAliveInterval=30");
#ifndef Q_OS_WIN
    const QString controlPath = QDir(DataPaths::configDir()).filePath(QStringLiteral("ssh-%C"));
    args << QStringLiteral("-o") << QStringLiteral("ControlMaster=auto");
    args << QStringLiteral("-o") << QStringLiteral("ControlPath=%1").arg(controlPath);
    args << QStringLiteral("-o") << QStringLiteral("ControlPersist=120");
#endif

    if (keyAuth) {
        const QString keyPath = expandUserPath(
            auth.value(QStringLiteral("sshPrivateKeyPath")).toString(QStringLiteral("~/.ssh/id_ed25519")));
        args << QStringLiteral("-i") << keyPath;
        args << QStringLiteral("-o") << QStringLiteral("IdentitiesOnly=yes");
    } else {
        args << QStringLiteral("-o") << QStringLiteral("PreferredAuthentications=password");
        args << QStringLiteral("-o") << QStringLiteral("PubkeyAuthentication=no");
        args << QStringLiteral("-o") << QStringLiteral("NumberOfPasswordPrompts=1");
    }

    return args;
}

SshClient::ProcessResult SshClient::runSsh(const QString &remoteCommand, int timeoutSec) const
{
    QStringList args = baseSshArgs();
    args << target();
    args << remoteCommand;
    return runProcess(sshProgram(), args, timeoutSec);
}

SshClient::ProcessResult SshClient::runSftpBatch(const QStringList &commands, int timeoutSec) const
{
    QTemporaryFile batchFile;
    batchFile.setAutoRemove(true);
    if (!batchFile.open()) {
        return {false, -1, {}, {}, QStringLiteral("无法创建 SFTP 批处理脚本")};
    }

    QTextStream stream(&batchFile);
    for (const QString &command : commands) {
        stream << command << QLatin1Char('\n');
    }
    batchFile.close();

    QStringList args = baseSshArgs();
    args << QStringLiteral("-b") << batchFile.fileName();
    args << target();
    return runProcess(sftpProgram(), args, timeoutSec);
}

bool SshClient::buildInteractiveInvocation(SshInteractiveInvocation *out, QString *error)
{
    if (out == nullptr) {
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);

    QString askPassError;
    if (!ensureAskPass(&askPassError)) {
        if (error != nullptr) {
            *error = askPassError;
        }
        return false;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QJsonObject auth = m_context.serverConfig.value(QStringLiteral("auth")).toObject();
    if (auth.value(QStringLiteral("mode")).toString() != QStringLiteral("ssh-key")) {
        if (!m_askPassScriptPath.isEmpty()) {
            env.insert(QStringLiteral("SSH_ASKPASS"), m_askPassScriptPath);
            env.insert(QStringLiteral("SSH_ASKPASS_REQUIRE"), QStringLiteral("force"));
            env.insert(QStringLiteral("DISPLAY"), QStringLiteral(":0"));
        }
    }

    QStringList args = baseSshArgs();
    args << QStringLiteral("-tt");
    args << target();

    out->program = sshProgram();
    out->args = args;
    out->env = env;
    return true;
}

bool SshClient::ensureSession(QString *error)
{
    QString askPassError;
    if (!ensureAskPass(&askPassError)) {
        if (error != nullptr) {
            *error = askPassError;
        }
        return false;
    }

    if (m_connected) {
        return true;
    }

    const ProcessResult ping = runSsh(QStringLiteral("echo deploy-hub-ping"), 15);
    if (!ping.ok) {
        if (error != nullptr) {
            *error = ping.error;
        }
        m_connected = false;
        return false;
    }

    m_connected = true;
    return true;
}

bool SshClient::connect(QString *error)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    return ensureSession(error);
}

void SshClient::disconnect()
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    cleanupAskPass();
    m_connected = false;
}

RemoteCommandResult SshClient::execute(const QString &command, int timeoutSec)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteCommandResult result;
    QString sessionError;
    if (!ensureSession(&sessionError)) {
        result.error = sessionError;
        return result;
    }

    const ProcessResult process = runSsh(command, timeoutSec);

    result.ok = process.ok;
    result.exitCode = process.exitCode;
    result.stdoutText = process.stdoutText;
    result.stderrText = process.stderrText;
    result.error = process.error;
    if (!result.ok) {
        m_connected = false;
    }
    return result;
}

UploadResult SshClient::uploadFile(const QString &localPath, const QString &remotePath)
{
    return uploadFile(localPath, remotePath, true);
}

UploadResult SshClient::uploadFile(const QString &localPath,
                                   const QString &remotePath,
                                   bool ensureRemoteDir)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    UploadResult result;
    QFileInfo localFile(localPath);
    if (!localFile.exists() || !localFile.isFile()) {
        result.error = QStringLiteral("本地文件不存在：%1").arg(localPath);
        return result;
    }

    QString sessionError;
    if (!ensureSession(&sessionError)) {
        result.error = sessionError;
        return result;
    }

    const QString normalizedRemote = normalizeRemotePath(remotePath);
    if (ensureRemoteDir) {
        const int slashIndex = normalizedRemote.lastIndexOf(QLatin1Char('/'));
        if (slashIndex > 0) {
            createDirectory(normalizedRemote.left(slashIndex));
        }
    }

    QStringList args = baseScpArgs();
    args << localPath;
    args << QStringLiteral("%1:%2").arg(target(), normalizedRemote);

    const ProcessResult process = runProcess(scpProgram(), args, 1800);

    result.totalBytes = localFile.size();
    result.bytesSent = process.ok ? result.totalBytes : 0;
    result.ok = process.ok;
    result.error = process.error;
    if (!result.ok) {
        m_connected = false;
    }
    return result;
}

RemoteFileListResult SshClient::listDirectory(const QString &remotePath)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileListResult result;
    const QString currentPath = normalizeRemotePath(remotePath);
    result.currentPath = currentPath;

    const RemoteCommandResult command = execute(
        QStringLiteral("LC_ALL=C ls -1Ap %1").arg(remoteShellQuote(currentPath)),
        30);
    if (!command.ok) {
        result.error = command.error.isEmpty() ? command.stderrText : command.error;
        return result;
    }

    if (currentPath != QStringLiteral("/")) {
        RemoteFileEntry parentEntry;
        parentEntry.name = QStringLiteral("..");
        parentEntry.path = joinRemotePath(currentPath, QStringLiteral(".."));
        parentEntry.type = RemoteFileType::Directory;
        result.entries.append(parentEntry);
    }

    RemoteFileEntry selfEntry;
    selfEntry.name = QStringLiteral(".");
    selfEntry.path = joinRemotePath(currentPath, QStringLiteral("."));
    selfEntry.type = RemoteFileType::Directory;
    result.entries.append(selfEntry);

    const QStringList lines = command.stdoutText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || isNavEntry(line)) {
            continue;
        }
        const bool isDirectory = line.endsWith(QLatin1Char('/'));
        if (isDirectory) {
            line.chop(1);
        }

        RemoteFileEntry entry;
        entry.name = line;
        entry.path = joinRemotePath(currentPath, line);
        entry.type = isDirectory ? RemoteFileType::Directory : RemoteFileType::File;
        entry.sizeBytes = isDirectory ? -1 : 0;
        entry.writable = true;
        result.entries.append(entry);
    }

    result.ok = true;
    return result;
}

RemoteFileReadResult SshClient::readFile(const QString &remotePath)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileReadResult result;
    const QString normalized = normalizeRemotePath(remotePath);
    const RemoteCommandResult command = execute(
        QStringLiteral("cat %1").arg(remoteShellQuote(normalized)),
        60);
    if (!command.ok) {
        result.error = command.error.isEmpty() ? command.stderrText : command.error;
        return result;
    }

    result.ok = true;
    result.remotePath = normalized;
    result.content = command.stdoutText;
    return result;
}

RemoteFileReadResult SshClient::readFileTail(const QString &remotePath, int lineCount)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileReadResult result;
    const QString normalized = normalizeRemotePath(remotePath);
    const int count = qMax(1, lineCount);
    QString commandText = QStringLiteral("tail -n %1 -- %2").arg(count).arg(remoteShellQuote(normalized));
    const RemoteLogGlobSpec globSpec = parseRemoteLogGlobPath(normalized);
    if (globSpec.isGlob()) {
        commandText = sshTailLatestMatchingFileCommand(globSpec, count);
    }
    const RemoteCommandResult command = execute(commandText, 30);
    if (!command.ok) {
        result.error = command.error.isEmpty() ? command.stderrText : command.error;
        return result;
    }

    result.ok = true;
    result.remotePath = normalized;
    result.content = command.stdoutText;
    if (result.content.endsWith(QLatin1Char('\n'))) {
        result.content.chop(1);
    }
    return result;
}

RemoteFileWriteResult SshClient::writeFile(const QString &remotePath, const QString &content)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileWriteResult result;
    const QString normalized = normalizeRemotePath(remotePath);

    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (!tempFile.open()) {
        result.error = QStringLiteral("无法创建临时文件");
        return result;
    }
    tempFile.write(content.toUtf8());
    tempFile.close();

    const UploadResult upload = uploadFile(tempFile.fileName(), normalized);
    result.ok = upload.ok;
    result.remotePath = normalized;
    result.error = upload.error;
    return result;
}

RemoteFileOperationResult SshClient::createDirectory(const QString &remotePath)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileOperationResult result;
    const QString normalized = normalizeRemotePath(remotePath);
    const RemoteCommandResult command = execute(
        QStringLiteral("mkdir -p %1").arg(remoteShellQuote(normalized)),
        30);
    result.ok = command.ok;
    result.error = command.error.isEmpty() ? command.stderrText : command.error;
    return result;
}

RemoteFileOperationResult SshClient::deleteEntry(const QString &remotePath)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileOperationResult result;
    const QString normalized = normalizeRemotePath(remotePath);
    if (normalized == QStringLiteral("/")) {
        result.error = QStringLiteral("不能删除根目录");
        return result;
    }

    const RemoteCommandResult typeCheck = execute(
        QStringLiteral("if [ -d %1 ]; then echo DIR; else echo FILE; fi").arg(remoteShellQuote(normalized)),
        20);
    if (!typeCheck.ok) {
        result.error = typeCheck.error;
        return result;
    }

    const QString remoteCommand = typeCheck.stdoutText.trimmed() == QStringLiteral("DIR")
        ? QStringLiteral("rm -rf %1").arg(remoteShellQuote(normalized))
        : QStringLiteral("rm -f %1").arg(remoteShellQuote(normalized));
    const RemoteCommandResult command = execute(remoteCommand, 30);
    result.ok = command.ok;
    result.error = command.error.isEmpty() ? command.stderrText : command.error;
    return result;
}

RemoteFileOperationResult SshClient::renameEntry(const QString &sourcePath, const QString &destPath)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileOperationResult result;
    const QString source = normalizeRemotePath(sourcePath);
    const QString dest = normalizeRemotePath(destPath);
    const RemoteCommandResult command = execute(
        QStringLiteral("mv %1 %2").arg(remoteShellQuote(source), remoteShellQuote(dest)),
        60);
    result.ok = command.ok;
    result.error = command.error.isEmpty() ? command.stderrText : command.error;
    return result;
}

RemoteFileOperationResult SshClient::copyEntry(const QString &sourcePath, const QString &destPath)
{
    std::lock_guard<std::recursive_mutex> lock(m_ioMutex);
    RemoteFileOperationResult result;
    const QString source = normalizeRemotePath(sourcePath);
    const QString dest = normalizeRemotePath(destPath);

    const RemoteCommandResult typeCheck = execute(
        QStringLiteral("if [ -d %1 ]; then echo DIR; else echo FILE; fi").arg(remoteShellQuote(source)),
        20);
    if (!typeCheck.ok) {
        result.error = typeCheck.error.isEmpty() ? typeCheck.stderrText : typeCheck.error;
        return result;
    }

    const QString remoteCommand = typeCheck.stdoutText.trimmed() == QStringLiteral("DIR")
        ? QStringLiteral("cp -r %1 %2").arg(remoteShellQuote(source), remoteShellQuote(dest))
        : QStringLiteral("cp %1 %2").arg(remoteShellQuote(source), remoteShellQuote(dest));
    const RemoteCommandResult command = execute(remoteCommand, 120);
    result.ok = command.ok;
    result.error = command.error.isEmpty() ? command.stderrText : command.error;
    return result;
}
