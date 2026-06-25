#include "infra/ConfigValidator.h"

#include "infra/LogPath.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

namespace {
const QRegularExpression VersionPattern(QStringLiteral("^[0-9]{8}-[0-9]{6}$"));
const QRegularExpression Sha1FingerprintPattern(QStringLiteral("^[0-9A-Fa-f]{40}$"));

void requireString(const QJsonObject &object, const QString &field, QStringList *errors)
{
    if (!object.contains(field) || !object.value(field).isString() || object.value(field).toString().isEmpty()) {
        errors->append(field + QStringLiteral(" is required"));
    }
}

void requireObject(const QJsonObject &object, const QString &field, QStringList *errors)
{
    if (!object.contains(field) || !object.value(field).isObject()) {
        errors->append(field + QStringLiteral(" is required"));
    }
}

void requireNonEmptyStringArray(const QJsonObject &object, const QString &field, QStringList *errors)
{
    if (!object.contains(field) || !object.value(field).isArray()) {
        errors->append(field + QStringLiteral(" is required"));
        return;
    }
    const QJsonArray array = object.value(field).toArray();
    if (array.isEmpty()) {
        errors->append(field + QStringLiteral(" must not be empty"));
        return;
    }
    for (const QJsonValue &value : array) {
        if (!value.isString() || value.toString().isEmpty()) {
            errors->append(field + QStringLiteral(" must contain non-empty strings"));
            break;
        }
    }
}

void requireFinishedAt(const QJsonObject &record, QStringList *errors)
{
    if (!record.contains(QStringLiteral("finishedAt"))
        || record.value(QStringLiteral("finishedAt")).isNull()
        || !record.value(QStringLiteral("finishedAt")).isString()
        || record.value(QStringLiteral("finishedAt")).toString().isEmpty()) {
        errors->append(QStringLiteral("finishedAt is required"));
    }
}
}

ValidationResult ConfigValidator::validateProject(const QJsonObject &project)
{
    ValidationResult result;

    requireString(project, QStringLiteral("id"), &result.errors);
    requireString(project, QStringLiteral("name"), &result.errors);
    requireString(project, QStringLiteral("type"), &result.errors);
    requireObject(project, QStringLiteral("source"), &result.errors);
    requireObject(project, QStringLiteral("build"), &result.errors);
    requireObject(project, QStringLiteral("deploy"), &result.errors);

    const QString type = project.value(QStringLiteral("type")).toString();
    const bool isFrontend = type == QStringLiteral("frontend-static");
    const bool isJava = type == QStringLiteral("java-maven");

    const QJsonObject source = project.value(QStringLiteral("source")).toObject();
    const QString kind = source.value(QStringLiteral("kind")).toString();
    if (kind == QStringLiteral("local")) {
        if (!source.value(QStringLiteral("localPath")).isString() || source.value(QStringLiteral("localPath")).toString().isEmpty()) {
            result.errors.append(QStringLiteral("source.localPath is required for local source"));
        }
    } else if (kind == QStringLiteral("github")) {
        if (!source.value(QStringLiteral("repoUrl")).isString() || source.value(QStringLiteral("repoUrl")).toString().isEmpty()) {
            result.errors.append(QStringLiteral("source.repoUrl is required for github source"));
        }
        if (!source.value(QStringLiteral("ref")).isString() || source.value(QStringLiteral("ref")).toString().isEmpty()) {
            result.errors.append(QStringLiteral("source.ref is required for github source"));
        }
    } else {
        result.errors.append(QStringLiteral("source.kind must be local or github"));
    }

    const QJsonObject build = project.value(QStringLiteral("build")).toObject();
    if (build.value(QStringLiteral("location")).toString() != QStringLiteral("local")) {
        result.errors.append(QStringLiteral("build.location must be local"));
    }
    const QString buildMode = build.value(QStringLiteral("mode")).toString(QStringLiteral("build"));
    if (isJava) {
        if (buildMode != QStringLiteral("build") && buildMode != QStringLiteral("prebuilt-jar")) {
            result.errors.append(QStringLiteral("build.mode must be build or prebuilt-jar for java-maven projects"));
        }
    } else {
        if (buildMode != QStringLiteral("build")) {
            result.errors.append(QStringLiteral("build.mode must be build for non-java projects"));
        }
    }
    const bool prebuiltJar = isJava && buildMode == QStringLiteral("prebuilt-jar");
    if (!prebuiltJar) {
        requireString(build, QStringLiteral("command"), &result.errors);
    }
    requireString(build, QStringLiteral("artifactPath"), &result.errors);
    const QJsonValue artifactRenameValue = build.value(QStringLiteral("artifactRename"));
    if (artifactRenameValue.isString() && artifactRenameValue.toString().trimmed().isEmpty()) {
        result.errors.append(QStringLiteral("build.artifactRename must not be empty when provided"));
    }

    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    requireString(deploy, QStringLiteral("serverId"), &result.errors);
    requireString(deploy, QStringLiteral("remoteBaseDir"), &result.errors);
    const QString failureStrategy = deploy.value(QStringLiteral("failureStrategy")).toString();
    if (failureStrategy != QStringLiteral("keep") && failureStrategy != QStringLiteral("rollback")) {
        result.errors.append(QStringLiteral("deploy.failureStrategy must be keep or rollback"));
    }
    const QJsonValue logDirValue = deploy.value(QStringLiteral("logDir"));
    if (logDirValue.isString() && logDirValue.toString().trimmed().isEmpty()) {
        result.errors.append(QStringLiteral("deploy.logDir must not be empty when provided"));
    }

    if (!isFrontend) {
        const QString backupPolicy = deploy.value(QStringLiteral("backupPolicy")).toString(QStringLiteral("backup"));
        if (backupPolicy != QStringLiteral("backup") && backupPolicy != QStringLiteral("replace")) {
            result.errors.append(QStringLiteral("deploy.backupPolicy must be backup or replace"));
        }
        const QString restartMode = deploy.value(QStringLiteral("restartMode")).toString();
        if (!restartMode.isEmpty()
            && restartMode != QStringLiteral("restart-script")
            && restartMode != QStringLiteral("service-command")) {
            result.errors.append(QStringLiteral("deploy.restartMode must be restart-script or service-command"));
        }
    }

    const QStringList optionalStringFields = {
        QStringLiteral("serviceMatch"),
        QStringLiteral("startCommand"),
        QStringLiteral("stopCommand"),
        QStringLiteral("restartCommand"),
        QStringLiteral("statusCommand"),
        QStringLiteral("targetJarPath"),
        QStringLiteral("backupDir")
    };
    for (const QString &field : optionalStringFields) {
        const QJsonValue value = deploy.value(field);
        if (value.isString() && value.toString().trimmed().isEmpty()) {
            result.errors.append(QStringLiteral("deploy.%1 must not be empty when provided").arg(field));
        }
    }

    result.ok = result.errors.isEmpty();
    return result;
}

ValidationResult ConfigValidator::validateServer(const QJsonObject &server)
{
    ValidationResult result;

    requireString(server, QStringLiteral("id"), &result.errors);
    requireString(server, QStringLiteral("name"), &result.errors);
    requireString(server, QStringLiteral("os"), &result.errors);
    requireString(server, QStringLiteral("host"), &result.errors);
    requireString(server, QStringLiteral("username"), &result.errors);
    requireObject(server, QStringLiteral("auth"), &result.errors);
    requireString(server, QStringLiteral("defaultRemoteBaseDir"), &result.errors);

    const QString os = server.value(QStringLiteral("os")).toString();
    const QJsonObject auth = server.value(QStringLiteral("auth")).toObject();
    const QString authMode = auth.value(QStringLiteral("mode")).toString();
    if (authMode != QStringLiteral("system-keychain") && authMode != QStringLiteral("manual") && authMode != QStringLiteral("ssh-key")) {
        result.errors.append(QStringLiteral("auth.mode is invalid"));
    }

    if (os == QStringLiteral("windows")) {
        if (authMode == QStringLiteral("ssh-key")) {
            result.errors.append(QStringLiteral("auth.mode ssh-key is only supported for linux servers"));
        }
        if (!server.value(QStringLiteral("winrm")).isObject()) {
            result.errors.append(QStringLiteral("winrm is required for windows servers"));
        } else {
            const QJsonObject winrm = server.value(QStringLiteral("winrm")).toObject();
            const QString scheme = winrm.value(QStringLiteral("scheme")).toString();
            if (scheme != QStringLiteral("https") && scheme != QStringLiteral("http")) {
                result.errors.append(QStringLiteral("winrm.scheme must be https or http"));
            }
            const QJsonValue fingerprint = winrm.value(QStringLiteral("trustedCertFingerprint"));
            if (!fingerprint.isNull()) {
                if (!fingerprint.isString() || !Sha1FingerprintPattern.match(fingerprint.toString()).hasMatch()) {
                    result.errors.append(QStringLiteral("winrm.trustedCertFingerprint must be 40 hex digits"));
                }
            }
        }
        if (!server.value(QStringLiteral("ssh")).isNull() && server.contains(QStringLiteral("ssh"))) {
            result.errors.append(QStringLiteral("ssh must be null for windows servers"));
        }
        if (server.contains(QStringLiteral("remoteFiles")) && !server.value(QStringLiteral("remoteFiles")).isNull()) {
            result.errors.append(QStringLiteral("remoteFiles must be null for windows servers"));
        }
    } else if (os == QStringLiteral("linux")) {
        if (!server.value(QStringLiteral("ssh")).isObject()) {
            result.errors.append(QStringLiteral("ssh is required for linux servers"));
        }
        if (!server.value(QStringLiteral("winrm")).isNull() && server.contains(QStringLiteral("winrm"))) {
            result.errors.append(QStringLiteral("winrm must be null for linux servers"));
        }
        if (server.contains(QStringLiteral("remoteFiles")) && !server.value(QStringLiteral("remoteFiles")).isNull()) {
            const QJsonObject remoteFiles = server.value(QStringLiteral("remoteFiles")).toObject();
            if (!remoteFiles.value(QStringLiteral("defaultBrowsePath")).toString().isEmpty()
                && remoteFiles.value(QStringLiteral("defaultBrowsePath")).toString().at(0) != QLatin1Char('/')) {
                result.errors.append(QStringLiteral("remoteFiles.defaultBrowsePath must be absolute on linux"));
            }
            if (remoteFiles.value(QStringLiteral("maxUploadSizeMb")).toInt(0) <= 0) {
                result.errors.append(QStringLiteral("remoteFiles.maxUploadSizeMb must be positive"));
            }
        }
    } else {
        result.errors.append(QStringLiteral("os must be linux or windows"));
    }

    result.ok = result.errors.isEmpty();
    return result;
}

ValidationResult ConfigValidator::validateDeployment(const QJsonObject &record)
{
    ValidationResult result;

    requireString(record, QStringLiteral("id"), &result.errors);
    requireString(record, QStringLiteral("projectId"), &result.errors);
    requireString(record, QStringLiteral("serverId"), &result.errors);
    requireString(record, QStringLiteral("startedAt"), &result.errors);

    const QString version = record.value(QStringLiteral("version")).toString();
    if (version.isEmpty() || !VersionPattern.match(version).hasMatch()) {
        result.errors.append(QStringLiteral("version must match YYYYMMDD-HHmmss"));
    }

    const QString logPath = record.value(QStringLiteral("logPath")).toString();
    if (!LogPath::isValidRelativePath(logPath)) {
        result.errors.append(QStringLiteral("logPath must be relative, e.g. logs/<deploy-id>.log"));
    }

    const QString status = record.value(QStringLiteral("status")).toString();
    if (status.isEmpty()) {
        result.errors.append(QStringLiteral("status is required"));
        result.ok = false;
        return result;
    }

    const QStringList knownStatuses = {
        QStringLiteral("pending"),
        QStringLiteral("building"),
        QStringLiteral("uploading"),
        QStringLiteral("restarting"),
        QStringLiteral("success"),
        QStringLiteral("failed"),
        QStringLiteral("rollbacking"),
        QStringLiteral("rolled_back"),
        QStringLiteral("rollback_failed"),
        QStringLiteral("canceled")
    };
    if (!knownStatuses.contains(status)) {
        result.errors.append(QStringLiteral("status is invalid"));
    }

    if (status == QStringLiteral("success")) {
        requireString(record, QStringLiteral("remoteVersionDir"), &result.errors);
        requireNonEmptyStringArray(record, QStringLiteral("artifactNames"), &result.errors);
        requireFinishedAt(record, &result.errors);
    } else if (status == QStringLiteral("failed")) {
        requireString(record, QStringLiteral("failureStep"), &result.errors);
        requireString(record, QStringLiteral("failureReason"), &result.errors);
        requireFinishedAt(record, &result.errors);
    } else if (status == QStringLiteral("rolled_back")) {
        requireString(record, QStringLiteral("remoteVersionDir"), &result.errors);
        requireNonEmptyStringArray(record, QStringLiteral("artifactNames"), &result.errors);
        requireString(record, QStringLiteral("failureStep"), &result.errors);
        requireString(record, QStringLiteral("failureReason"), &result.errors);
        requireString(record, QStringLiteral("rolledBackFrom"), &result.errors);
        requireFinishedAt(record, &result.errors);
        const QString rolledBackFrom = record.value(QStringLiteral("rolledBackFrom")).toString();
        if (!rolledBackFrom.isEmpty() && !VersionPattern.match(rolledBackFrom).hasMatch()) {
            result.errors.append(QStringLiteral("rolledBackFrom must match YYYYMMDD-HHmmss"));
        }
    } else if (status == QStringLiteral("rollback_failed")) {
        requireString(record, QStringLiteral("failureStep"), &result.errors);
        requireString(record, QStringLiteral("failureReason"), &result.errors);
        requireString(record, QStringLiteral("rolledBackFrom"), &result.errors);
        requireFinishedAt(record, &result.errors);
        const QString rolledBackFrom = record.value(QStringLiteral("rolledBackFrom")).toString();
        if (!rolledBackFrom.isEmpty() && !VersionPattern.match(rolledBackFrom).hasMatch()) {
            result.errors.append(QStringLiteral("rolledBackFrom must match YYYYMMDD-HHmmss"));
        }
    } else if (status == QStringLiteral("canceled")) {
        requireFinishedAt(record, &result.errors);
    }

    const QString failureStep = record.value(QStringLiteral("failureStep")).toString();
    if (!failureStep.isEmpty()) {
        const QStringList knownSteps = {
            QStringLiteral("validate"),
            QStringLiteral("building"),
            QStringLiteral("uploading"),
            QStringLiteral("current_switch"),
            QStringLiteral("restarting"),
            QStringLiteral("rollback")
        };
        if (!knownSteps.contains(failureStep)) {
            result.errors.append(QStringLiteral("failureStep is invalid"));
        }
    }

    if (record.contains(QStringLiteral("failureReason")) && !record.value(QStringLiteral("failureReason")).isNull()
        && !record.value(QStringLiteral("failureReason")).isString()) {
        result.errors.append(QStringLiteral("failureReason must be string or null"));
    }
    if (record.contains(QStringLiteral("rolledBackFrom")) && !record.value(QStringLiteral("rolledBackFrom")).isNull()
        && !record.value(QStringLiteral("rolledBackFrom")).isString()) {
        result.errors.append(QStringLiteral("rolledBackFrom must be string or null"));
    }
    if (record.contains(QStringLiteral("finishedAt")) && !record.value(QStringLiteral("finishedAt")).isNull()
        && !record.value(QStringLiteral("finishedAt")).isString()) {
        result.errors.append(QStringLiteral("finishedAt must be string or null"));
    }

    result.ok = result.errors.isEmpty();
    return result;
}
