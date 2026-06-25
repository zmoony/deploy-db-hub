#include "adapters/services/JdbcSqlBridge.h"

#include "infra/AppSettingsStore.h"
#include "infra/DataPaths.h"
#include "infra/JdkProfileStore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

namespace {

QString driverClassForProduct(const QString &productKey)
{
    return productKey == QStringLiteral("oracle") ? QStringLiteral("oracle.jdbc.OracleDriver")
                                                  : QStringLiteral("org.postgresql.Driver");
}

QString jdbcUrlForEndpoint(const ServiceEndpoint &endpoint, const QString &productKey)
{
    if (productKey == QStringLiteral("oracle")) {
        return QStringLiteral("jdbc:oracle:thin:@%1:%2/%3")
            .arg(endpoint.host, QString::number(endpoint.port), endpoint.database);
    }
    return QStringLiteral("jdbc:postgresql://%1:%2/%3?connectTimeout=10&socketTimeout=30&sslmode=disable")
        .arg(endpoint.host, QString::number(endpoint.port), endpoint.database);
}

AppSettings loadSettings()
{
    AppSettings settings;
    AppSettingsStore(AppSettingsStore::defaultSettingsFile()).load(&settings, nullptr);
    return settings;
}

QString javaExecutableInHome(const QString &home)
{
    const QString normalized = QDir::fromNativeSeparators(home.trimmed());
    if (normalized.isEmpty()) {
        return {};
    }
    const QString candidate = QDir(normalized).filePath(QStringLiteral("bin/java"));
#ifdef Q_OS_WIN
    const QString winCandidate = candidate + QStringLiteral(".exe");
    if (QFileInfo::exists(winCandidate)) {
        return winCandidate;
    }
#endif
    if (QFileInfo::exists(candidate)) {
        return candidate;
    }
    return {};
}

QString resolveJavaExecutable(const AppSettings &)
{
    QVector<JdkProfile> profiles;
    JdkProfileStore store(DataPaths::jdkProfilesFile());
    store.load(&profiles, nullptr);
    for (const JdkProfile &profile : profiles) {
        const QString fromProfile = javaExecutableInHome(profile.path);
        if (!fromProfile.isEmpty()) {
            return fromProfile;
        }
    }

    const QByteArray javaHome = qgetenv("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        const QString fromEnv = javaExecutableInHome(QString::fromUtf8(javaHome));
        if (!fromEnv.isEmpty()) {
            return fromEnv;
        }
    }
    return QStandardPaths::findExecutable(QStringLiteral("java"));
}

QString runnerJarPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath(QStringLiteral("tools/jdbc-sql-runner.jar")),
        QDir(appDir).filePath(QStringLiteral("../tools/jdbc-sql-runner.jar")),
        QDir(appDir).filePath(QStringLiteral("../build-release/tools/jdbc-sql-runner.jar")),
        QDir(appDir).filePath(QStringLiteral("../../build-release/tools/jdbc-sql-runner.jar")),
        QDir(appDir).filePath(QStringLiteral("../tools/jdbc-sql-runner/jdbc-sql-runner.jar")),
    };
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }
    return candidates.first();
}

ServiceResult parseRunnerOutput(const QByteArray &output, const QString &fallbackError)
{
    QByteArray trimmed = output.trimmed();
    if (trimmed.isEmpty()) {
        return {false, fallbackError.isEmpty() ? QStringLiteral("JDBC 执行失败") : fallbackError, {}, {}};
    }

    const int jsonStart = trimmed.indexOf('{');
    if (jsonStart > 0) {
        trimmed = trimmed.mid(jsonStart);
    }

    const QJsonDocument document = QJsonDocument::fromJson(trimmed);
    if (!document.isObject()) {
        const QString snippet = QString::fromUtf8(trimmed.left(240));
        const QString message = fallbackError.isEmpty() ? QStringLiteral("JDBC 执行失败") : fallbackError;
        return {false, snippet.isEmpty() ? message : QStringLiteral("%1: %2").arg(message, snippet), {}, {}};
    }
    const QJsonObject object = document.object();
    ServiceResult result;
    result.ok = object.value(QStringLiteral("ok")).toBool();
    result.message = object.value(QStringLiteral("message")).toString();
    const QJsonArray rows = object.value(QStringLiteral("rows")).toArray();
    for (const QJsonValue &value : rows) {
        if (value.isObject()) {
            result.rows.append(value.toObject());
        }
    }
    if (!result.ok && result.message.isEmpty()) {
        result.message = fallbackError;
    }
    return result;
}

QString formatProcessFailure(const QByteArray &stdoutBytes,
                             const QByteArray &stderrBytes,
                             const QString &fallbackError)
{
    const ServiceResult parsed = parseRunnerOutput(stdoutBytes, QString());
    if (!parsed.message.isEmpty() && parsed.message != QStringLiteral("JDBC 执行失败")) {
        return parsed.message;
    }
    const QString stderrText = QString::fromUtf8(stderrBytes.trimmed());
    if (!stderrText.isEmpty()) {
        return stderrText;
    }
    return fallbackError.isEmpty() ? QStringLiteral("JDBC 查询失败") : fallbackError;
}

QString classpathFor(const QString &driverJar)
{
#ifdef Q_OS_WIN
    const QChar separator = QLatin1Char(';');
#else
    const QChar separator = QLatin1Char(':');
#endif
    const QString runnerJar = runnerJarPath();
    if (QFileInfo::exists(runnerJar)) {
        return runnerJar + separator + driverJar;
    }
    return driverJar;
}

}

QString JdbcSqlBridge::driverJarForProduct(const QString &productKey)
{
    const AppSettings settings = loadSettings();
    if (productKey == QStringLiteral("oracle")) {
        return settings.oracleDriverJar.trimmed();
    }
    if (productKey == QStringLiteral("postgresql")) {
        return settings.postgresDriverJar.trimmed();
    }
    return {};
}

bool JdbcSqlBridge::isConfigured(const QString &productKey)
{
    const QString jar = driverJarForProduct(productKey);
    return !jar.isEmpty() && QFileInfo::exists(jar);
}

bool JdbcSqlBridge::isJdbcReady(const QString &productKey)
{
    if (!isConfigured(productKey)) {
        return false;
    }
    return !resolveJavaExecutable(loadSettings()).isEmpty() && QFileInfo::exists(runnerJarPath());
}

ServiceResult JdbcSqlBridge::executeQuery(const ServiceEndpoint &endpoint,
                                          const QString &productKey,
                                          const QString &sql)
{
    const QString driverJar = driverJarForProduct(productKey);
    if (driverJar.isEmpty()) {
        return {false,
                QStringLiteral("未配置 %1 JDBC 驱动 JAR。请在「设置 → 数据库驱动」选择驱动文件。")
                    .arg(productKey == QStringLiteral("oracle") ? QStringLiteral("Oracle")
                                                                : QStringLiteral("PostgreSQL")),
                {},
                {}};
    }
    if (!QFileInfo::exists(driverJar)) {
        return {false, QStringLiteral("驱动 JAR 不存在: %1").arg(driverJar), {}, {}};
    }

    const AppSettings settings = loadSettings();
    const QString java = resolveJavaExecutable(settings);
    if (java.isEmpty()) {
        return {false, QStringLiteral("未找到 java 可执行文件。请在「部署工具 → 设置」或「一键部署」中配置 JDK，或设置 JAVA_HOME / PATH。"), {}, {}};
    }

    const QString runnerJar = runnerJarPath();
    if (!QFileInfo::exists(runnerJar)) {
        return {false,
                QStringLiteral("内置 JDBC 运行器缺失，请重新编译应用或从 build-release/tools 复制 jdbc-sql-runner.jar 到程序目录 tools/。"),
                {},
                {}};
    }

    QStringList arguments;
    arguments << QStringLiteral("-cp") << classpathFor(driverJar) << QStringLiteral("JdbcSqlRunner")
              << QStringLiteral("--driver-class") << driverClassForProduct(productKey) << QStringLiteral("--jdbc-url")
              << jdbcUrlForEndpoint(endpoint, productKey) << QStringLiteral("--username") << endpoint.username
              << QStringLiteral("--password") << endpoint.password << QStringLiteral("--sql") << sql;

    QProcess process;
    process.start(java, arguments);
    if (!process.waitForStarted(5000)) {
        return {false, QStringLiteral("无法启动 Java: %1").arg(process.errorString()), {}, {}};
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        return {false, QStringLiteral("JDBC 查询超时"), {}, {}};
    }

    const QByteArray stdoutBytes = process.readAllStandardOutput();
    const QByteArray stderrBytes = process.readAllStandardError();
    const ServiceResult parsed = parseRunnerOutput(stdoutBytes, QString());
    if (parsed.ok || !parsed.message.isEmpty()) {
        return parsed;
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {false, formatProcessFailure(stdoutBytes, stderrBytes, QStringLiteral("JDBC 查询失败")), {}, {}};
    }
    return parseRunnerOutput(stdoutBytes, QStringLiteral("JDBC 执行失败"));
}

SqlDriverProbeResult JdbcSqlBridge::probe()
{
    return probe(loadSettings());
}

SqlDriverProbeResult JdbcSqlBridge::probe(const AppSettings &settings)
{
    SqlDriverProbeResult result;
    const QString java = resolveJavaExecutable(settings);

    auto probeDriver = [&](const QString &label, const QString &jarPath) {
        if (jarPath.isEmpty()) {
            return QStringLiteral("%1: 未配置").arg(label);
        }
        if (!QFileInfo::exists(jarPath)) {
            return QStringLiteral("%1: 文件不存在").arg(label);
        }
        return QStringLiteral("%1: 已配置").arg(label);
    };

    QString javaText;
    QVector<JdkProfile> profiles;
    JdkProfileStore(DataPaths::jdkProfilesFile()).load(&profiles, nullptr);
    if (!profiles.isEmpty()) {
        int validCount = 0;
        for (const JdkProfile &profile : profiles) {
            if (!javaExecutableInHome(profile.path).isEmpty()) {
                ++validCount;
            }
        }
        javaText = validCount > 0 ? QStringLiteral("Java: 已配置 (%1 个 JDK)").arg(validCount)
                                  : QStringLiteral("Java: JDK 路径无效");
    } else if (!java.isEmpty()) {
        javaText = QStringLiteral("Java: 使用系统环境");
    } else {
        javaText = QStringLiteral("Java: 请在部署工具配置 JDK");
    }

    const QString postgresJar = settings.postgresDriverJar.trimmed();
    const QString oracleJar = settings.oracleDriverJar.trimmed();
    const QString postgresText = probeDriver(QStringLiteral("PostgreSQL"), postgresJar);
    const QString oracleText = probeDriver(QStringLiteral("Oracle"), oracleJar);
    result.postgresAvailable = !postgresJar.isEmpty() && QFileInfo::exists(postgresJar) && !java.isEmpty();
    result.oracleAvailable = !oracleJar.isEmpty() && QFileInfo::exists(oracleJar) && !java.isEmpty();
    result.message = QStringLiteral("%1 | %2 | %3").arg(javaText, postgresText, oracleText);
    return result;
}
