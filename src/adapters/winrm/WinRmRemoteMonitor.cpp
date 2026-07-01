#include "adapters/winrm/WinRmRemoteMonitor.h"

#include "adapters/winrm/WinRmClient.h"
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

QString powerShellSingleQuote(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("''"));
    return QStringLiteral("'%1'").arg(value);
}

ServiceControlResult runServiceCommand(const RemoteConnectionContext &context,
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

    WinRmClient client(context);
    const QString remoteBaseDir = projectConfig.value(QStringLiteral("deploy")).toObject()
                                      .value(QStringLiteral("remoteBaseDir")).toString();
    const QString finalCommand = wrapCommandWithWorkingDirectory(QStringLiteral("windows"), commandText, remoteBaseDir);
    const RemoteCommandResult command = client.execute(finalCommand, 45);
    result.ok = command.ok;
    result.output = command.stdoutText.trimmed();
    result.error = command.ok
        ? QString()
        : (command.error.isEmpty() ? command.stderrText.trimmed() : command.error);
    return result;
}

}

WinRmRemoteMonitor::WinRmRemoteMonitor(RemoteConnectionContext context)
    : m_context(std::move(context))
{
}

bool WinRmRemoteMonitor::monitoringEnabled(const QJsonObject &serverConfig) const
{
    return ::monitoringEnabled(serverConfig);
}

bool WinRmRemoteMonitor::allowKillProcess(const QJsonObject &serverConfig) const
{
    return ::allowKillProcess(serverConfig);
}

ServiceStatusResult WinRmRemoteMonitor::queryServiceStatus(const QJsonObject &serverConfig,
                                                           const QJsonObject &projectConfig)
{
    ServiceStatusResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const ProjectServiceConfig service = projectServiceConfig(projectConfig);
    WinRmClient client(m_context);
    const QString commandText = service.statusCommand.isEmpty()
        ? defaultWindowsServiceStatusCommand(projectConfig)
        : renderProjectServiceCommand(service.statusCommand, projectConfig);
    const RemoteCommandResult command = client.execute(commandText, 30);

    if (!command.ok) {
        result.state = ServiceRunState::ServerAbnormal;
        result.message = command.error;
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
    } else {
        result.state = ServiceRunState::Stopped;
        result.message = QStringLiteral("%1 未运行").arg(service.serviceMatch);
    }
    result.ok = true;
    return result;
}

ServerMetricsResult WinRmRemoteMonitor::queryMetrics(const QJsonObject &serverConfig)
{
    ServerMetricsResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    WinRmClient client(m_context);
    const RemoteCommandResult command = client.execute(
        QStringLiteral("powershell -NoProfile -Command \""
                       "$cpu = (Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average;"
                       "$os = Get-CimInstance Win32_OperatingSystem;"
                       "$disk = Get-CimInstance Win32_LogicalDisk -Filter \\\"DeviceID='C:'\\\";"
                       "Write-Output ('CPU:' + [math]::Round($cpu,1));"
                       "Write-Output ('MEM:' + [math]::Round(($os.TotalVisibleMemorySize-$os.FreePhysicalMemory)/1024) + '/' + [math]::Round($os.TotalVisibleMemorySize/1024));"
                       "Write-Output ('DISK:' + [math]::Round(($disk.Size-$disk.FreeSpace)/1GB) + '/' + [math]::Round($disk.Size/1GB))\""),
        45);

    if (!command.ok) {
        result.error = command.error;
        return result;
    }

    const QStringList lines = command.stdoutText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
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
                result.diskUsedGb = parts.at(0).trimmed().toLongLong();
                result.diskTotalGb = parts.at(1).trimmed().toLongLong();
            }
        }
    }

    result.ok = true;
    return result;
}

ProcessListResult WinRmRemoteMonitor::listTopProcesses(const QJsonObject &serverConfig, int limit)
{
    ProcessListResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const int effectiveLimit = processListLimit(serverConfig, limit);
    WinRmClient client(m_context);
    const RemoteCommandResult command = client.execute(
        QStringLiteral("powershell -NoProfile -Command \""
                       "Get-Process | Sort-Object CPU -Descending | Select-Object -First %1 "
                       "Id,ProcessName,CPU,WorkingSet,Path | ConvertTo-Csv -NoTypeInformation\"")
            .arg(effectiveLimit),
        45);

    if (!command.ok) {
        result.error = command.error;
        return result;
    }

    const QStringList lines = command.stdoutText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (int i = 1; i < lines.size(); ++i) {
        const QStringList fields = lines.at(i).split(QLatin1Char(','));
        if (fields.size() < 4) {
            continue;
        }
        RemoteProcessEntry entry;
        entry.pid = fields.at(0).trimmed().remove(QLatin1Char('"')).toInt();
        entry.name = fields.at(1).trimmed().remove(QLatin1Char('"'));
        entry.cpuPercent = fields.at(2).trimmed().remove(QLatin1Char('"')).toDouble();
        const qint64 workingSet = fields.at(3).trimmed().remove(QLatin1Char('"')).toLongLong();
        entry.memoryPercent = workingSet > 0 ? workingSet / (1024.0 * 1024.0 * 16.0) : 0.0;
        entry.commandLine = fields.size() > 4 ? fields.mid(4).join(QLatin1Char(',')).remove(QLatin1Char('"')) : entry.name;
        entry.user = QStringLiteral("SYSTEM");
        result.processes.append(entry);
    }

    result.ok = true;
    return result;
}

KillProcessResult WinRmRemoteMonitor::killProcess(const QJsonObject &serverConfig, int pid)
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

    WinRmClient client(m_context);
    const RemoteCommandResult command = client.execute(
        QStringLiteral("powershell -NoProfile -Command \"Stop-Process -Id %1 -Force\"").arg(pid),
        20);
    if (!command.ok) {
        result.error = command.error;
        return result;
    }

    result.ok = true;
    return result;
}

ServiceControlResult WinRmRemoteMonitor::startService(const QJsonObject &serverConfig,
                                                      const QJsonObject &projectConfig)
{
    if (!monitoringEnabled(serverConfig)) {
        return {false, {}, QStringLiteral("服务器监控未启用")};
    }
    return runServiceCommand(m_context,
                             projectConfig,
                             projectServiceConfig(projectConfig).startCommand,
                             QStringLiteral("项目未配置远程启动命令。请编辑项目并填写「远程服务命令」里的启动命令；「本地脚本」仅在一键部署时上传执行。"));
}

ServiceControlResult WinRmRemoteMonitor::stopService(const QJsonObject &serverConfig,
                                                     const QJsonObject &projectConfig)
{
    if (!monitoringEnabled(serverConfig)) {
        return {false, {}, QStringLiteral("服务器监控未启用")};
    }
    const ProjectServiceConfig config = projectServiceConfig(projectConfig);
    if (!config.stopCommand.isEmpty()) {
        return runServiceCommand(m_context, projectConfig, config.stopCommand, QStringLiteral("项目未配置停止命令"));
    }

    ServiceStatusResult status = queryServiceStatus(serverConfig, projectConfig);
    if (!status.ok) {
        return {false, {}, status.error};
    }
    if (status.pid <= 0) {
        return {true, QStringLiteral("服务未运行"), {}};
    }

    WinRmClient client(m_context);
    const RemoteCommandResult command = client.execute(
        QStringLiteral("powershell -NoProfile -Command \"Stop-Process -Id %1 -Force\"").arg(status.pid),
        20);
    return {command.ok,
            command.stdoutText.trimmed(),
            command.ok ? QString() : (command.error.isEmpty() ? command.stderrText.trimmed() : command.error)};
}

ServiceControlResult WinRmRemoteMonitor::restartService(const QJsonObject &serverConfig,
                                                        const QJsonObject &projectConfig)
{
    if (!monitoringEnabled(serverConfig)) {
        return {false, {}, QStringLiteral("服务器监控未启用")};
    }
    const ProjectServiceConfig config = projectServiceConfig(projectConfig);
    if (!config.restartCommand.isEmpty()) {
        return runServiceCommand(m_context, projectConfig, config.restartCommand, QStringLiteral("项目未配置重启命令"));
    }
    ServiceControlResult stop = stopService(serverConfig, projectConfig);
    if (!stop.ok) {
        return stop;
    }
    return startService(serverConfig, projectConfig);
}

std::unique_ptr<RemoteMonitor> createWinRmRemoteMonitor(const RemoteConnectionContext &context)
{
    return std::make_unique<WinRmRemoteMonitor>(context);
}
