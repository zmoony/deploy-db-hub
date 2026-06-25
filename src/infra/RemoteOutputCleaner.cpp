#include "infra/RemoteOutputCleaner.h"

#include <QRegularExpression>

namespace RemoteOutputCleaner {

namespace {

bool isBannerLine(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }
    static const QStringList patterns = {
        QStringLiteral("Authorized users only"),
        QStringLiteral("All activities may be monitored"),
        QStringLiteral("All connections are monitored"),
        QStringLiteral("This system is for the use of authorized users only"),
    };
    for (const QString &pattern : patterns) {
        if (trimmed.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString joinLines(const QStringList &lines)
{
    return lines.join(QLatin1Char('\n')).trimmed();
}

}

QString stripSshBanner(const QString &text)
{
    QStringList kept;
    for (const QString &line : text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
        if (!isBannerLine(line)) {
            kept.append(line.trimmed());
        }
    }
    return joinLines(kept);
}

QString normalizeRemoteError(const QString &text)
{
    const QString cleaned = stripSshBanner(text);
    if (cleaned.contains(QStringLiteral("Permission denied"), Qt::CaseInsensitive)) {
        if (cleaned.contains(QStringLiteral("kafka"), Qt::CaseInsensitive)
            || cleaned.contains(QStringLiteral(".sh"), Qt::CaseInsensitive)) {
            return QStringLiteral("Kafka 脚本无执行权限，请在服务器上执行 chmod +x <安装目录>/bin/*.sh");
        }
        return QStringLiteral("远程命令无执行权限");
    }
    if (cleaned.contains(QStringLiteral("No such file or directory"), Qt::CaseInsensitive)) {
        return QStringLiteral("远程路径不存在，请检查 Kafka 安装路径配置");
    }
    if (cleaned.contains(QStringLiteral("Connection refused"), Qt::CaseInsensitive)) {
        return QStringLiteral("无法连接 Kafka 端口，请检查 broker 是否运行");
    }
    return cleaned;
}

}
