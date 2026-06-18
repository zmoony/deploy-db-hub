#include "adapters/winrm/WinRmClient.h"

#include "infra/RemoteLogPath.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QCryptographicHash>

namespace {

struct CliProcessResult {
    bool ok = false;
    int exitCode = -1;
    QString stdoutText;
    QString stderrText;
    QString error;
};

CliProcessResult runProcess(const QString &program, const QStringList &arguments, int timeoutSec)
{
    CliProcessResult result;
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(program, arguments);
    if (!process.waitForStarted(10000)) {
        result.error = QStringLiteral("无法启动 %1：%2").arg(program, process.errorString());
        return result;
    }
    if (!process.waitForFinished(qMax(1, timeoutSec) * 1000)) {
        process.kill();
        result.error = QStringLiteral("%1 执行超时（%2 秒）").arg(program).arg(timeoutSec);
        return result;
    }
    result.exitCode = process.exitCode();
    result.stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    result.stderrText = QString::fromUtf8(process.readAllStandardError());
    result.ok = result.exitCode == 0;
    if (!result.ok && result.error.isEmpty()) {
        result.error = result.stderrText.trimmed().isEmpty()
            ? QStringLiteral("%1 退出码 %2").arg(program).arg(result.exitCode)
            : result.stderrText.trimmed();
    }
    return result;
}

QString powerShellSingleQuote(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\''), QStringLiteral("''"));
    return QStringLiteral("'%1'").arg(escaped);
}

}

WinRmClient::WinRmClient(RemoteConnectionContext context)
    : m_context(std::move(context))
{
}

QString WinRmClient::endpoint() const
{
    const QJsonObject winrm = m_context.serverConfig.value(QStringLiteral("winrm")).toObject();
    const QString scheme = winrm.value(QStringLiteral("scheme")).toString(QStringLiteral("https"));
    const QString host = m_context.serverConfig.value(QStringLiteral("host")).toString();
    const int port = m_context.serverConfig.value(QStringLiteral("port")).toInt(5986);
    return QStringLiteral("%1://%2:%3/wsman").arg(scheme, host).arg(port);
}

WinRmClient::ProcessResult WinRmClient::runWinRs(const QString &command, int timeoutSec) const
{
    if (m_context.password.isEmpty()) {
        return {false, -1, {}, {}, QStringLiteral("未提供 WinRM 密码")};
    }

    const QString username = m_context.serverConfig.value(QStringLiteral("username")).toString();
    const QString host = m_context.serverConfig.value(QStringLiteral("host")).toString();
    const QJsonObject winrm = m_context.serverConfig.value(QStringLiteral("winrm")).toObject();
    const int port = m_context.serverConfig.value(QStringLiteral("port")).toInt(5986);
    const bool tlsVerify = winrm.value(QStringLiteral("tlsVerify")).toBool(true);

    QStringList args;
    args << QStringLiteral("-r:%1:%2").arg(host).arg(port);
    args << QStringLiteral("-un:%1").arg(username);
    args << QStringLiteral("-pw:%1").arg(m_context.password);
    if (!tlsVerify) {
        args << QStringLiteral("-skipCA") << QStringLiteral("-skipCN");
    }
    args << command;

    const CliProcessResult process = runProcess(QStringLiteral("winrs"), args, timeoutSec);
    return {process.ok, process.exitCode, process.stdoutText, process.stderrText, process.error};
}

RemoteCommandResult WinRmClient::execute(const QString &command, int timeoutSec)
{
    RemoteCommandResult result;
    const ProcessResult process = runWinRs(command, timeoutSec);
    result.ok = process.ok;
    result.exitCode = process.exitCode;
    result.stdoutText = process.stdoutText;
    result.stderrText = process.stderrText;
    result.error = process.error;
    return result;
}

UploadResult WinRmClient::uploadFile(const QString &localPath, const QString &remotePath)
{
    UploadResult result;
    QFile input(localPath);
    if (!input.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("无法读取本地文件：%1").arg(localPath);
        return result;
    }

    const QByteArray data = input.readAll();
    result.totalBytes = data.size();
    const qint64 maxBytes = 500LL * 1024LL * 1024LL;
    if (result.totalBytes > maxBytes) {
        result.error = QStringLiteral("文件超过 WinRM 上传上限 500MB");
        return result;
    }

    const QString remoteNative = QDir::toNativeSeparators(remotePath);
    const QString tempPath = remoteNative + QStringLiteral(".deployhub.tmp");
    const QString sha256 = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());

    RemoteCommandResult init = execute(
        QStringLiteral("powershell -NoProfile -Command \""
                       "if (Test-Path '%1') { Remove-Item -Force '%1' }"
                       "New-Item -ItemType File -Path '%1' -Force | Out-Null\"")
            .arg(tempPath),
        60);
    if (!init.ok) {
        result.error = init.error;
        return result;
    }

    const int chunkSize = 512 * 1024;
    qint64 offset = 0;
    while (offset < data.size()) {
        const QByteArray chunk = data.mid(static_cast<int>(offset), chunkSize);
        const QString encoded = QString::fromLatin1(chunk.toBase64());
        RemoteCommandResult chunkResult = execute(
            QStringLiteral("powershell -NoProfile -Command \""
                           "$bytes = [Convert]::FromBase64String('%1');"
                           "$stream = [System.IO.File]::Open('%2', [System.IO.FileMode]::Append);"
                           "try { $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }\"")
                .arg(encoded, tempPath),
            120);
        if (!chunkResult.ok) {
            execute(QStringLiteral("powershell -NoProfile -Command \"Remove-Item -Force '%1' -ErrorAction SilentlyContinue\"")
                        .arg(tempPath),
                    30);
            result.error = chunkResult.error;
            return result;
        }
        offset += chunk.size();
        result.bytesSent = offset;
    }

    RemoteCommandResult finalize = execute(
        QStringLiteral("powershell -NoProfile -Command \""
                       "$actual = (Get-FileHash '%1' -Algorithm SHA256).Hash.ToLowerInvariant();"
                       "if ($actual -ne '%2') { exit 2 }"
                       "Move-Item -Force '%1' '%3'\"")
            .arg(tempPath, sha256, remoteNative),
        120);
    if (!finalize.ok) {
        result.error = finalize.error.isEmpty() ? finalize.stdoutText : finalize.error;
        return result;
    }

    result.ok = true;
    return result;
}

RemoteFileReadResult WinRmClient::readFile(const QString &remotePath)
{
    return readFileTail(remotePath, 20000);
}

RemoteFileReadResult WinRmClient::readFileTail(const QString &remotePath, int lineCount)
{
    RemoteFileReadResult result;
    const QString normalized = QDir::fromNativeSeparators(remotePath.trimmed());
    if (normalized.isEmpty()) {
        result.error = QStringLiteral("日志路径不能为空");
        return result;
    }

    const int count = qMax(1, lineCount);
    QString command;
    const RemoteLogGlobSpec globSpec = parseRemoteLogGlobPath(normalized);
    if (globSpec.isGlob()) {
        command = winRmTailLatestMatchingFileCommand(globSpec, count);
    } else {
        command = QStringLiteral("powershell -NoProfile -Command \""
                                 "Get-Content -LiteralPath %1 -Tail %2\"")
            .arg(powerShellSingleQuote(QDir::toNativeSeparators(normalized)), QString::number(count));
    }

    const RemoteCommandResult output = execute(command, 30);
    if (!output.ok) {
        result.error = output.error.isEmpty() ? output.stderrText : output.error;
        return result;
    }

    result.ok = true;
    result.remotePath = normalized;
    result.content = output.stdoutText;
    if (result.content.endsWith(QLatin1Char('\n'))) {
        result.content.chop(1);
    }
    return result;
}
