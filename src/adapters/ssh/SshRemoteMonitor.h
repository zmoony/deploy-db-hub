#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteMonitor.h"

#include "adapters/ssh/SshClient.h"

class SshRemoteMonitor final : public RemoteMonitor
{
public:
    explicit SshRemoteMonitor(RemoteConnectionContext context);

    ServiceStatusResult queryServiceStatus(const QJsonObject &serverConfig,
                                           const QJsonObject &projectConfig) override;
    ServerMetricsResult queryMetrics(const QJsonObject &serverConfig) override;
    ProcessListResult listTopProcesses(const QJsonObject &serverConfig, int limit) override;
    KillProcessResult killProcess(const QJsonObject &serverConfig, int pid) override;
    ServiceControlResult startService(const QJsonObject &serverConfig, const QJsonObject &projectConfig) override;
    ServiceControlResult stopService(const QJsonObject &serverConfig, const QJsonObject &projectConfig) override;
    ServiceControlResult restartService(const QJsonObject &serverConfig, const QJsonObject &projectConfig) override;

private:
    bool monitoringEnabled(const QJsonObject &serverConfig) const;
    bool allowKillProcess(const QJsonObject &serverConfig) const;

    RemoteConnectionContext m_context;
    SshClient m_client;
};

std::unique_ptr<RemoteMonitor> createSshRemoteMonitor(const RemoteConnectionContext &context);
