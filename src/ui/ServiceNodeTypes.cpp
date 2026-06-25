#include "ui/ServiceNodeTypes.h"

#include <QStringList>

QString serviceProductKindKey(ServiceProductKind kind)
{
    switch (kind) {
    case ServiceProductKind::Kafka:
        return QStringLiteral("kafka");
    case ServiceProductKind::Redis:
        return QStringLiteral("redis");
    case ServiceProductKind::Elasticsearch:
        return QStringLiteral("elasticsearch");
    case ServiceProductKind::Oracle:
        return QStringLiteral("oracle");
    case ServiceProductKind::PostgreSQL:
        return QStringLiteral("postgresql");
    }
    return QStringLiteral("unknown");
}

QString serviceProductKindLabel(ServiceProductKind kind)
{
    switch (kind) {
    case ServiceProductKind::Kafka:
        return QStringLiteral("Kafka");
    case ServiceProductKind::Redis:
        return QStringLiteral("Redis");
    case ServiceProductKind::Elasticsearch:
        return QStringLiteral("Elasticsearch");
    case ServiceProductKind::Oracle:
        return QStringLiteral("Oracle");
    case ServiceProductKind::PostgreSQL:
        return QStringLiteral("PostgreSQL");
    }
    return QStringLiteral("-");
}

QString serviceParentCategoryLabel(ServiceProductKind kind)
{
    switch (kind) {
    case ServiceProductKind::Oracle:
    case ServiceProductKind::PostgreSQL:
        return QStringLiteral("所属数据库");
    default:
        return QStringLiteral("所属大数据");
    }
}

QString serviceNodeInfoPlaceholder(ServiceProductKind kind)
{
    switch (kind) {
    case ServiceProductKind::Elasticsearch:
        return QStringLiteral("port:username:password:auth_fingerprint");
    case ServiceProductKind::Kafka:
        return QStringLiteral("18103");
    case ServiceProductKind::Redis:
        return QStringLiteral("6379 或 port:password");
    case ServiceProductKind::Oracle:
    case ServiceProductKind::PostgreSQL:
        return QStringLiteral("db_name:port:username:password");
    }
    return QStringLiteral("请输入");
}

QString serviceNodeFillInstructions(ServiceProductKind kind)
{
    if (serviceProductUsesDirectConnect(kind)) {
        switch (kind) {
        case ServiceProductKind::Redis:
            return QStringLiteral(
                "直连模式：填写服务地址与端口即可连接，无需先在「部署工具 → 服务器管理」登记服务器。"
                "\nRedis 6+ ACL 可在用户名栏填写 ACL 用户名。");
        case ServiceProductKind::Elasticsearch:
            return QStringLiteral(
                "直连模式：填写 Elasticsearch HTTP 地址与端口即可，无需登记部署服务器。"
                "\nKibana 端口用于工具栏「Kibana」按钮，默认 5601。");
        case ServiceProductKind::Oracle:
        case ServiceProductKind::PostgreSQL:
            return QStringLiteral(
                "直连模式：填写数据库地址、端口、库名与账号即可，无需登记部署服务器。");
        default:
            break;
        }
    }

    const QString serverHint = QStringLiteral(
        "服务器：可从下拉列表选择已登记服务器，也可直接输入 IP/主机名。");

    QString infoHint;
    switch (kind) {
    case ServiceProductKind::Elasticsearch:
        infoHint = QStringLiteral(
            "信息：Elasticsearch 格式 port:username:password:auth_fingerprint，无用户名时只填 port。"
            "\nKibana 端口：点击 Kibana 按钮时打开的端口，默认 5601，可按实际部署修改。");
        break;
    case ServiceProductKind::Kafka:
        infoHint = QStringLiteral("信息：Kafka 只填 port，默认 18103。");
        break;
    case ServiceProductKind::Redis:
        infoHint = QStringLiteral("信息：Redis 填 port，或 port:password，或 port:password:username（ACL）。");
        break;
    case ServiceProductKind::Oracle:
    case ServiceProductKind::PostgreSQL:
        infoHint = QStringLiteral("信息：Postgresql/Oracle 格式 db_name:port:username:password。");
        break;
    default:
        infoHint = QStringLiteral("信息：按组件类型填写连接参数。");
        break;
    }

    const QString installHint = QStringLiteral(
        "安装路径：组件属于本平台时必填；留空则使用各组件默认安装目录，也可自行填写覆盖。");
    const QString storageHint = QStringLiteral(
        "存储路径：用于监控，多个路径用英文逗号分隔（如 df -h 中的挂载路径）。");

    QStringList parts;
    parts << serverHint << infoHint << installHint << storageHint;
    return parts.join(QStringLiteral("\n"));
}

bool serviceProductUsesDirectConnect(ServiceProductKind kind)
{
    switch (kind) {
    case ServiceProductKind::Redis:
    case ServiceProductKind::Elasticsearch:
    case ServiceProductKind::Oracle:
    case ServiceProductKind::PostgreSQL:
        return true;
    case ServiceProductKind::Kafka:
        return false;
    }
    return false;
}
