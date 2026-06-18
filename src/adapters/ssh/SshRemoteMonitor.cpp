#include "adapters/ssh/SshRemoteMonitor.h"

#include "adapters/ssh/SshClient.h"
#include "infra/ProjectServiceConfig.h"

#include <QFileInfo>
#include <QRegularExpression>

namespace {

bool monitoringEnabled(const QJsonObject &serverConfig)
{
    const QJsonObject monitoring = serverConfig.value(QStringLiteral("monitoring")).toObject();
    if (monitoring.isEmpty()) {
        return true;
    }
    return monitoring.value(QStringLiteral("enabled")).toBool(true);
}

bool allowKillProcess(const QJsonObject &serverConfig)
{
    const QJsonObject monitoring = serverConfig.value(QStringLiteral("monitoring")).toObject();
    return monitoring.value(QStringLiteral("allowKillProcess")).toBool(true);
}

int processListLimit(const QJsonObject &serverConfig, int requested)
{
    const QJsonObject monitoring = serverConfig.value(QStringLiteral("monitoring")).toObject();
    const int configured = monitoring.value(QStringLiteral("processListLimit")).toInt(20);
    return qMax(1, qMin(requested > 0 ? requested : configured, configured));
}

QString metricsCommand()
{
    return QStringLiteral(
        "free -m | awk '/^Mem:/{printf \"MEM:%s/%s\\n\",$3,$2}'; "
        "df -BM / | awk 'NR==2{gsub(/M/,\"\",$3); gsub(/M/,\"\",$2); printf \"DISK:%s/%s\\n\",$3,$2}'; "
        "grep '^cpu ' /proc/stat | awk '{total=0; for(i=2;i<=NF;i++) total+=$i; idle=$5; printf \"CPU:%.1f\\n\", (total-idle)*100/total}'");
}

ServerMetricsResult parseMetricsOutput(const QString &output)
{
    ServerMetricsResult result;
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith(QStringLiteral("CPU:"))) {
            result.cpuUsagePercent = line.section(QLatin1Char(':'), 1).trimmed().toDouble();
        } else if (line.startsWith(QStringLiteral("MEM:"))) {
            const QStringList parts = line.section(QLatin1Char(':'), 1).split(QLatin1Char('/'));
            if (parts.size() == 2) {
                result.memoryUsedMb = parts.at(0).trimmed().toLongLong();
                result.memoryTotalMb = parts.at(1).trimmed().toLongLong();
            }
        } else if (line.startsWith(QStringLiteral("DISK:"))) {
            const QStringList parts = line.section(QLatin1Char(':'), 1).split(QLatin1Char('/'));
            if (parts.size() == 2) {
                result.diskUsedGb = parts.at(0).trimmed().toLongLong() / 1024;
                result.diskTotalGb = parts.at(1).trimmed().toLongLong() / 1024;
            }
        }
    }
    result.ok = true;
    return result;
}

}

SshRemoteMonitor::SshRemoteMonitor(RemoteConnectionContext context)
    : m_context(std::move(context))
    , m_client(m_context)
{
}

bool SshRemoteMonitor::monitoringEnabled(const QJsonObject &serverConfig) const
{
    return ::monitoringEnabled(serverConfig);
}

bool SshRemoteMonitor::allowKillProcess(const QJsonObject &serverConfig) const
{
    return ::allowKillProcess(serverConfig);
}

ServiceStatusResult SshRemoteMonitor::queryServiceStatus(const QJsonObject &serverConfig,
                                                         const QJsonObject &projectConfig)
{
    ServiceStatusResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const ProjectServiceConfig service = projectServiceConfig(projectConfig);
    const QString commandText = service.statusCommand.isEmpty()
        ? defaultLinuxServiceStatusCommand(projectConfig)
        : renderProjectServiceCommand(service.statusCommand, projectConfig);
    const RemoteCommandResult command = m_client.execute(commandText, 20);

    if (!command.ok && command.exitCode < 0) {
        result.state = ServiceRunState::ServerAbnormal;
        result.message = command.error.isEmpty() ? command.stderrText : command.error;
        result.ok = true;
        return result;
    }

    const QString output = command.stdoutText.trimmed();
    if (output.contains(QStringLiteral("RUNNING"))) {
        static const QRegularExpression pidPattern(QStringLiteral(R"(RUNNING[:\s]+(\d+))"));
        const QRegularExpressionMatch match = pidPattern.match(output);
        if (match.hasMatch()) {
            result.pid = match.captured(1).toInt();
        }
        result.state = ServiceRunState::Running;
        result.message = result.pid > 0
            ? QStringLiteral("%1 正在运行，PID %2").arg(service.serviceMatch).arg(result.pid)
            : QStringLiteral("%1 正在运行").arg(service.serviceMatch);
    } else if (output.contains(QStringLiteral("STOPPED"))) {
        result.state = ServiceRunState::Stopped;
        result.message = QStringLiteral("%1 未运行").arg(service.serviceMatch);
    } else {
        result.state = ServiceRunState::CheckAbnormal;
        result.message = QStringLiteral("无法判断 %1 状态：%2").arg(service.serviceMatch, output);
    }

    result.ok = true;
    return result;
}

ServerMetricsResult SshRemoteMonitor::queryMetrics(const QJsonObject &serverConfig)
{
    ServerMetricsResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const RemoteCommandResult command = m_client.execute(metricsCommand(), 20);
    if (!command.ok) {
        result.error = command.error.isEmpty() ? command.stderrText : command.error;
        return result;
    }

    return parseMetricsOutput(command.stdoutText);
}

ProcessListResult SshRemoteMonitor::listTopProcesses(const QJsonObject &serverConfig, int limit)
{
    ProcessListResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const int effectiveLimit = processListLimit(serverConfig, limit);
    const RemoteCommandResult command = m_client.execute(
        QStringLiteral("ps -eo pid=,user=,pcpu=,pmem=,args= --sort=-pcpu | head -n %1")
            .arg(effectiveLimit + 1),
        20);

    if (!command.ok) {
        result.error = command.error.isEmpty() ? command.stderrText : command.error;
        return result;
    }

    const QStringList lines = command.stdoutText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        static const QRegularExpression pattern(QStringLiteral(R"(^(\d+)\s+(\S+)\s+([\d.]+)\s+([\d.]+)\s+(.*)$)"));
        const QRegularExpressionMatch match = pattern.match(line.trimmed());
        if (!match.hasMatch()) {
            continue;
        }

        RemoteProcessEntry entry;
        entry.pid = match.captured(1).toInt();
        entry.user = match.captured(2);
        entry.cpuPercent = match.captured(3).toDouble();
        entry.memoryPercent = match.captured(4).toDouble();
        entry.commandLine = match.captured(5).trimmed();
        entry.name = entry.commandLine.section(QLatin1Char(' '), 0, 0);
        if (entry.name.contains(QLatin1Char('/'))) {
            entry.name = QFileInfo(entry.name).fileName();
        }
        result.processes.append(entry);
        if (result.processes.size() >= effectiveLimit) {
            break;
        }
    }

    result.ok = true;
    return result;
}

ServiceControlResult runServiceCommand(SshClient *client,
                                       const QJsonObject &projectConfig,
                                       const QString &commandTemplate,
                                       const QString &missingMessage)
{
    ServiceControlResult result;
    const QString commandText = renderProjectServiceCommand(commandTemplate, projectConfig);
    if (commandText.trimmed().isEmpty()) {
        result.error = missingMessage;
        return result;
    }

    const RemoteCommandResult command = client->execute(commandText, 45);
    result.ok = command.ok;
    result.output = command.stdoutText.trimmed();
    result.error = command.ok
        ? QString()
        : (command.error.isEmpty() ? command.stderrText.trimmed() : command.error);
    return result;
}

KillProcessResult SshRemoteMonitor::killProcess(const QJsonObject &serverConfig, int pid)
{
    KillProcessResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }
    if (!allowKillProcess(serverConfig)) {
        result.error = QStringLiteral("服务器配置禁止结束进程");
        return result;
    }
    if (pid <= 0) {
        result.error = QStringLiteral("无效的进程 ID");
        return result;
    }

    const RemoteCommandResult command = m_client.execute(QStringLiteral("kill -TERM %1").arg(pid), 15);
    if (!command.ok) {
        result.error = command.error.isEmpty() ? command.stderrText : command.error;
        return result;
    }

    result.ok = true;
    return result;
}

ServiceControlResult SshRemoteMonitor::startService(const QJsonObject &serverConfig,
                                                    const QJsonObject &projectConfig)
{
    if (!monitoringEnabled(serverConfig)) {
        return {false, {}, QStringLiteral("服务器监控未启用")};
    }
    return runServiceCommand(&m_client,
                             projectConfig,
                             projectServiceConfig(projectConfig).startCommand,
                             QStringLiteral("项目未配置启动命令"));
}

ServiceControlResult SshRemoteMonitor::stopService(const QJsonObject &serverConfig,
                                                   const QJsonObject &projectConfig)
{
    if (!monitoringEnabled(serverConfig)) {
        return {false, {}, QStringLiteral("服务器监控未启用")};
    }
    const ProjectServiceConfig config = projectServiceConfig(projectConfig);
    if (!config.stopCommand.isEmpty()) {
        return runServiceCommand(&m_client, projectConfig, config.stopCommand, QStringLiteral("项目未配置停止命令"));
    }

    ServiceStatusResult status = queryServiceStatus(serverConfig, projectConfig);
    if (!status.ok) {
        return {false, {}, status.error};
    }
    if (status.pid <= 0) {
        return {true, QStringLiteral("服务未运行"), {}};
    }
    const RemoteCommandResult command = m_client.execute(QStringLiteral("kill -TERM %1").arg(status.pid), 20);
    return {command.ok,
            command.stdoutText.trimmed(),
            command.ok ? QString() : (command.error.isEmpty() ? command.stderrText.trimmed() : command.error)};
}

ServiceControlResult SshRemoteMonitor::restartService(const QJsonObject &serverConfig,
                                                      const QJsonObject &projectConfig)
{
    if (!monitoringEnabled(serverConfig)) {
        return {false, {}, QStringLiteral("服务器监控未启用")};
    }
    const ProjectServiceConfig config = projectServiceConfig(projectConfig);
    if (!config.restartCommand.isEmpty()) {
        return runServiceCommand(&m_client, projectConfig, config.restartCommand, QStringLiteral("项目未配置重启命令"));
    }
    ServiceControlResult stop = stopService(serverConfig, projectConfig);
    if (!stop.ok) {
        return stop;
    }
    return startService(serverConfig, projectConfig);
}

std::unique_ptr<RemoteMonitor> createSshRemoteMonitor(const RemoteConnectionContext &context)
{
    return std::make_unique<SshRemoteMonitor>(context);
}
