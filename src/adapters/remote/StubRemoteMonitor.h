#pragma once

#include "adapters/remote/RemoteMonitor.h"

class StubRemoteMonitor final : public RemoteMonitor
{
public:
    explicit StubRemoteMonitor(const QJsonObject &serverConfig);

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

    QJsonObject m_serverConfig;
};
