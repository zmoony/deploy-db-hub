#pragma once

#include <QString>
#include <QVector>

#include <memory>

#include <QJsonObject>

#include "adapters/remote/RemoteConnection.h"

enum class ServiceRunState {
    Running,
    Stopped,
    CheckAbnormal,
    ServerAbnormal,
    Unknown
};

QString toString(ServiceRunState state);
QString serviceRunStateLabel(ServiceRunState state);

struct ServiceStatusResult {
    bool ok = false;
    ServiceRunState state = ServiceRunState::Unknown;
    int pid = 0;
    QString message;
    QString error;
};

struct ServerMetricsResult {
    bool ok = false;
    double cpuUsagePercent = 0.0;
    qint64 memoryUsedMb = 0;
    qint64 memoryTotalMb = 0;
    qint64 diskUsedGb = 0;
    qint64 diskTotalGb = 0;
    QString error;
};

struct RemoteProcessEntry {
    int pid = 0;
    QString name;
    QString user;
    QString commandLine;
    double cpuPercent = 0.0;
    double memoryPercent = 0.0;
};

struct ProcessListResult {
    bool ok = false;
    QVector<RemoteProcessEntry> processes;
    QString error;
};

struct KillProcessResult {
    bool ok = false;
    QString error;
};

struct ServiceControlResult {
    bool ok = false;
    QString output;
    QString error;
};

class RemoteMonitor
{
public:
    virtual ~RemoteMonitor() = default;

    virtual ServiceStatusResult queryServiceStatus(const QJsonObject &serverConfig,
                                                   const QJsonObject &projectConfig) = 0;
    virtual ServerMetricsResult queryMetrics(const QJsonObject &serverConfig) = 0;
    virtual ProcessListResult listTopProcesses(const QJsonObject &serverConfig, int limit) = 0;
    virtual KillProcessResult killProcess(const QJsonObject &serverConfig, int pid) = 0;
    virtual ServiceControlResult startService(const QJsonObject &serverConfig, const QJsonObject &projectConfig) = 0;
    virtual ServiceControlResult stopService(const QJsonObject &serverConfig, const QJsonObject &projectConfig) = 0;
    virtual ServiceControlResult restartService(const QJsonObject &serverConfig, const QJsonObject &projectConfig) = 0;
};

std::unique_ptr<RemoteMonitor> createRemoteMonitor(const RemoteConnectionContext &context);
