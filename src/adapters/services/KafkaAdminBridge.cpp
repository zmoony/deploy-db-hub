#include "adapters/services/KafkaAdminBridge.h"

#include "adapters/remote/RemoteExecutor.h"
#include "infra/RemoteOutputCleaner.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString shellQuote(const QString &value)
{
    QString escaped = value;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'") + escaped + QLatin1Char('\'');
}

QString bootstrapServer(const ServiceEndpoint &endpoint)
{
    return QStringLiteral("127.0.0.1:%1").arg(endpoint.port);
}

QString runnerJarPath()
{
    const QString bundled =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("tools/kafka-admin-runner.jar"));
    if (QFileInfo::exists(bundled)) {
        return bundled;
    }
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("../tools/kafka-admin-runner/kafka-admin-runner.jar"));
}

QString remoteRunnerPath()
{
    return QStringLiteral("~/.deploy-hub/tools/kafka-admin-runner.jar");
}

KafkaAdminPayload parsePayload(const QByteArray &output, const QString &fallbackError)
{
    KafkaAdminPayload payload;
    const QJsonDocument document = QJsonDocument::fromJson(output.trimmed());
    if (!document.isObject()) {
        payload.message = fallbackError.isEmpty() ? QStringLiteral("Kafka Admin 执行失败") : fallbackError;
        return payload;
    }
    const QJsonObject object = document.object();
    payload.ok = object.value(QStringLiteral("ok")).toBool();
    payload.message = object.value(QStringLiteral("message")).toString();
    const auto parseArray = [](const QJsonArray &array) {
        QVector<QJsonObject> rows;
        for (const QJsonValue &value : array) {
            if (value.isObject()) {
                rows.append(value.toObject());
            }
        }
        return rows;
    };
    payload.rows = parseArray(object.value(QStringLiteral("rows")).toArray());
    payload.stats = parseArray(object.value(QStringLiteral("stats")).toArray());
    payload.partitions = parseArray(object.value(QStringLiteral("partitions")).toArray());
    if (!payload.ok && payload.message.isEmpty()) {
        payload.message = fallbackError;
    }
    return payload;
}

bool ensureRunnerUploaded(RemoteExecutor *executor)
{
    const QString localJar = runnerJarPath();
    if (!QFileInfo::exists(localJar)) {
        return false;
    }
    const qint64 localSize = QFileInfo(localJar).size();
    // Skip re-upload when remote jar already exists with matching size.
    const QString remotePath = remoteRunnerPath();
    const QString checkCmd =
        QStringLiteral("test -f %1 && wc -c < %1 2>/dev/null || echo missing").arg(remotePath);
    const RemoteCommandResult check = executor->execute(checkCmd, 10);
    if (check.ok) {
        bool ok = false;
        const qint64 remoteSize = check.stdoutText.trimmed().toLongLong(&ok);
        if (ok && remoteSize > 0 && remoteSize == localSize) {
            return true;
        }
    }
    const UploadResult upload = executor->uploadFile(localJar, remotePath);
    return upload.ok;
}

QString mergedRemoteOutput(const RemoteCommandResult &commandResult)
{
    QString output = commandResult.stdoutText.trimmed();
    const QString stderrText = RemoteOutputCleaner::stripSshBanner(commandResult.stderrText.trimmed());
    if (!stderrText.isEmpty()) {
        if (!output.isEmpty()) {
            output += QLatin1Char('\n');
        }
        output += stderrText;
    }
    return output;
}

}

bool KafkaAdminBridge::isAvailable()
{
    return QFileInfo::exists(runnerJarPath());
}

KafkaAdminPayload KafkaAdminBridge::run(const ServiceEndpoint &endpoint,
                                        const RemoteConnectionContext &remote,
                                        const QStringList &args,
                                        int timeoutSec)
{
    KafkaAdminPayload payload;
    if (endpoint.installPath.isEmpty()) {
        payload.message = QStringLiteral("Kafka 节点未配置安装路径");
        return payload;
    }
    if (remote.serverConfig.value(QStringLiteral("os")).toString() != QStringLiteral("linux")) {
        payload.message = QStringLiteral("Kafka Admin 当前仅支持 Linux SSH 节点");
        return payload;
    }
    if (!isAvailable()) {
        payload.message = QStringLiteral("缺少 kafka-admin-runner.jar");
        return payload;
    }

    auto executor = createRemoteExecutor(remote);
    if (!executor || !ensureRunnerUploaded(executor.get())) {
        payload.message = QStringLiteral("无法上传 Kafka Admin Runner");
        return payload;
    }

    QStringList escapedArgs;
    escapedArgs << QStringLiteral("--bootstrap-server") << bootstrapServer(endpoint);
    escapedArgs.append(args);

    QStringList quotedArgs;
    for (const QString &arg : escapedArgs) {
        quotedArgs << shellQuote(arg);
    }

    const QString inner = QStringLiteral(
                              "INSTALL=%1\n"
                              "JAR=%2\n"
                              "mkdir -p ~/.deploy-hub/tools\n"
                              "command -v java >/dev/null 2>&1 || { echo '{\"ok\":false,\"message\":\"remote java not found\"}'; exit 1; }\n"
                              "java -cp \"$INSTALL/libs/*:$JAR\" KafkaAdminRunner %3")
                              .arg(shellQuote(endpoint.installPath),
                                   shellQuote(remoteRunnerPath()),
                                   quotedArgs.join(QLatin1Char(' ')));

    const QString command = QStringLiteral("export LANG=en; bash --noprofile --norc -c %1").arg(shellQuote(inner));
    const RemoteCommandResult commandResult = executor->execute(command, timeoutSec);
    const QString merged = mergedRemoteOutput(commandResult);
    if (!commandResult.ok) {
        payload.message = RemoteOutputCleaner::normalizeRemoteError(
            commandResult.error.isEmpty() ? merged : commandResult.error);
        return payload;
    }
    payload = parsePayload(commandResult.stdoutText.toUtf8(), merged);
    if (!payload.ok && merged.contains(QStringLiteral("Permission denied"), Qt::CaseInsensitive)) {
        payload.message = RemoteOutputCleaner::normalizeRemoteError(merged);
    }
    return payload;
}
