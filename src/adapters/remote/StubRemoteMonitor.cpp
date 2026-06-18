#include "adapters/remote/StubRemoteMonitor.h"

#include <QFileInfo>
#include <QtGlobal>

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

QString serviceNameFromProject(const QJsonObject &projectConfig)
{
    const QJsonObject deploy = projectConfig.value(QStringLiteral("deploy")).toObject();
    const QString restartScript = deploy.value(QStringLiteral("restartScript")).toString(QStringLiteral("app"));
    return QFileInfo(restartScript).baseName();
}

}

QString toString(ServiceRunState state)
{
    switch (state) {
    case ServiceRunState::Running:
        return QStringLiteral("running");
    case ServiceRunState::Stopped:
        return QStringLiteral("stopped");
    case ServiceRunState::CheckAbnormal:
        return QStringLiteral("check-abnormal");
    case ServiceRunState::ServerAbnormal:
        return QStringLiteral("server-abnormal");
    default:
        return QStringLiteral("unknown");
    }
}

QString serviceRunStateLabel(ServiceRunState state)
{
    switch (state) {
    case ServiceRunState::Running:
        return QStringLiteral("正在运行");
    case ServiceRunState::Stopped:
        return QStringLiteral("已停止");
    case ServiceRunState::CheckAbnormal:
        return QStringLiteral("检测异常");
    case ServiceRunState::ServerAbnormal:
        return QStringLiteral("服务器异常");
    default:
        return QStringLiteral("未知");
    }
}

StubRemoteMonitor::StubRemoteMonitor(const QJsonObject &serverConfig)
    : m_serverConfig(serverConfig)
{
}

bool StubRemoteMonitor::monitoringEnabled(const QJsonObject &serverConfig) const
{
    return ::monitoringEnabled(serverConfig);
}

bool StubRemoteMonitor::allowKillProcess(const QJsonObject &serverConfig) const
{
    return ::allowKillProcess(serverConfig);
}

ServiceStatusResult StubRemoteMonitor::queryServiceStatus(const QJsonObject &serverConfig,
                                                          const QJsonObject &projectConfig)
{
    ServiceStatusResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const QString host = serverConfig.value(QStringLiteral("host")).toString();
    if (host.isEmpty() || host == QStringLiteral("0.0.0.0")) {
        result.state = ServiceRunState::ServerAbnormal;
        result.message = QStringLiteral("无法连接目标服务器");
        result.ok = true;
        return result;
    }

    const QString projectId = projectConfig.value(QStringLiteral("id")).toString(QStringLiteral("demo"));
    const QString serviceName = serviceNameFromProject(projectConfig);
    const uint seed = qHash(projectId + host + serviceName);
    const int bucket = static_cast<int>(seed % 4);

    switch (bucket) {
    case 0:
        result.state = ServiceRunState::Running;
        result.message = QStringLiteral("服务 %1 正在运行").arg(serviceName);
        break;
    case 1:
        result.state = ServiceRunState::Stopped;
        result.message = QStringLiteral("服务 %1 未运行").arg(serviceName);
        break;
    case 2:
        result.state = ServiceRunState::CheckAbnormal;
        result.message = QStringLiteral("健康检查失败：%1 响应超时").arg(serviceName);
        break;
    default:
        result.state = ServiceRunState::ServerAbnormal;
        result.message = QStringLiteral("服务器 %1 不可达或 SSH/WinRM 异常").arg(host);
        break;
    }

    result.ok = true;
    return result;
}

ServerMetricsResult StubRemoteMonitor::queryMetrics(const QJsonObject &serverConfig)
{
    ServerMetricsResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const QString host = serverConfig.value(QStringLiteral("host")).toString();
    const uint seed = qHash(host);
    result.cpuUsagePercent = 18.0 + static_cast<int>(seed % 55);
    result.memoryTotalMb = 16384;
    result.memoryUsedMb = 4096 + static_cast<int>(seed % 8192);
    result.diskTotalGb = 256;
    result.diskUsedGb = 72 + static_cast<int>(seed % 120);
    result.ok = true;
    return result;
}

ProcessListResult StubRemoteMonitor::listTopProcesses(const QJsonObject &serverConfig, int limit)
{
    ProcessListResult result;
    if (!monitoringEnabled(serverConfig)) {
        result.error = QStringLiteral("服务器监控未启用");
        return result;
    }

    const int effectiveLimit = ::processListLimit(serverConfig, limit);
    const QString user = serverConfig.value(QStringLiteral("username")).toString(QStringLiteral("deploy"));
    const QString deployRoot = serverConfig.value(QStringLiteral("defaultRemoteBaseDir")).toString(QStringLiteral("/opt/deploy-hub"));

    struct ProcessSeed {
        QString name;
        QString user;
        QString commandLine;
        double cpu;
        double memory;
    };

    const QVector<ProcessSeed> seeds = {
        {QStringLiteral("java"), user,
         QStringLiteral("/usr/bin/java -Xms512m -Xmx1024m -jar %1/apps/demo/app.jar --spring.profiles.active=prod").arg(deployRoot),
         28.4, 12.6},
        {QStringLiteral("java"), user,
         QStringLiteral("/usr/bin/java -jar %1/apps/demo/lib/agent.jar -Dapp=demo-sidecar").arg(deployRoot),
         6.2, 3.1},
        {QStringLiteral("java"), QStringLiteral("root"),
         QStringLiteral("/usr/bin/java -jar /opt/monitoring/node-exporter-bridge.jar --port=9101"),
         4.8, 2.4},
        {QStringLiteral("nginx"), QStringLiteral("www-data"),
         QStringLiteral("nginx: master process /usr/sbin/nginx -g daemon off;"),
         3.5, 1.2},
        {QStringLiteral("sshd"), QStringLiteral("root"),
         QStringLiteral("/usr/sbin/sshd -D -o AuthorizedKeysFile=.ssh/authorized_keys"),
         1.1, 0.8},
        {QStringLiteral("systemd"), QStringLiteral("root"),
         QStringLiteral("/sbin/init"),
         0.6, 0.5},
        {QStringLiteral("mysqld"), QStringLiteral("mysql"),
         QStringLiteral("/usr/sbin/mysqld --basedir=/usr --datadir=/var/lib/mysql --plugin-dir=/usr/lib/mysql/plugin"),
         5.9, 8.7},
        {QStringLiteral("redis-server"), QStringLiteral("redis"),
         QStringLiteral("/usr/bin/redis-server 127.0.0.1:6379"),
         2.2, 1.9},
        {QStringLiteral("node"), user,
         QStringLiteral("/usr/bin/node /opt/frontend/admin/server.js --env production"),
         7.4, 4.3},
        {QStringLiteral("dockerd"), QStringLiteral("root"),
         QStringLiteral("/usr/bin/dockerd -H fd:// --containerd=/run/containerd/containerd.sock"),
         3.1, 5.5},
        {QStringLiteral("bash"), user,
         QStringLiteral("-bash"),
         0.3, 0.2},
        {QStringLiteral("python3"), user,
         QStringLiteral("python3 /opt/scripts/health-check.py --target demo --interval 30"),
         1.8, 1.1}
    };

    for (int i = 0; i < effectiveLimit; ++i) {
        const ProcessSeed &seed = seeds.at(i % seeds.size());
        RemoteProcessEntry entry;
        entry.pid = 1200 + i * 37;
        entry.name = seed.name;
        entry.user = seed.user;
        entry.commandLine = seed.commandLine;
        entry.cpuPercent = qMax(0.3, seed.cpu - i * 0.4);
        entry.memoryPercent = qMax(0.2, seed.memory - i * 0.3);
        result.processes.append(entry);
    }

    result.ok = true;
    return result;
}

KillProcessResult StubRemoteMonitor::killProcess(const QJsonObject &serverConfig, int pid)
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

    result.ok = true;
    return result;
}

ServiceControlResult StubRemoteMonitor::startService(const QJsonObject &serverConfig,
                                                     const QJsonObject &projectConfig)
{
    Q_UNUSED(serverConfig);
    Q_UNUSED(projectConfig);
    return {true, QStringLiteral("stub service started"), {}};
}

ServiceControlResult StubRemoteMonitor::stopService(const QJsonObject &serverConfig,
                                                    const QJsonObject &projectConfig)
{
    Q_UNUSED(serverConfig);
    Q_UNUSED(projectConfig);
    return {true, QStringLiteral("stub service stopped"), {}};
}

ServiceControlResult StubRemoteMonitor::restartService(const QJsonObject &serverConfig,
                                                       const QJsonObject &projectConfig)
{
    Q_UNUSED(serverConfig);
    Q_UNUSED(projectConfig);
    return {true, QStringLiteral("stub service restarted"), {}};
}
