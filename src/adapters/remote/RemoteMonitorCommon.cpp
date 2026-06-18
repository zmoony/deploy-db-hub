#include "adapters/remote/RemoteMonitor.h"

#include <QFileInfo>

namespace {

bool monitoringEnabled(const QJsonObject &serverConfig)
{
    const QJsonObject monitoring = serverConfig.value(QStringLiteral("monitoring")).toObject();
    if (monitoring.isEmpty()) {
        return true;
    }
    return monitoring.value(QStringLiteral("enabled")).toBool(true);
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
