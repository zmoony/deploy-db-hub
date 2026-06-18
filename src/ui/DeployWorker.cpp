#include "ui/DeployWorker.h"

#include "adapters/build/LocalBuilder.h"
#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteExecutor.h"
#include "adapters/remote/RemoteMonitor.h"
#include "core/DeployOrchestrator.h"
#include "core/DeploymentRecord.h"
#include "core/DeploymentState.h"
#include "infra/AppSettingsStore.h"
#include "infra/ArtifactMatcher.h"
#include "infra/ConfigStore.h"
#include "infra/ConfigValidator.h"
#include "infra/DataPaths.h"
#include "infra/JdkProfileStore.h"
#include "infra/ProjectServiceConfig.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include <algorithm>

namespace {

QString deploymentLogPath(const DeployOrchestrator::ActiveDeployment *deployment)
{
    if (deployment == nullptr) {
        return {};
    }
    return deployment->record.value(QStringLiteral("logPath")).toString();
}

QString shellQuote(QString value)
{
    value.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QStringLiteral("'%1'").arg(value);
}

QString remoteDirectoryOf(const QString &path)
{
    const QString normalized = QDir::fromNativeSeparators(path);
    const int slash = normalized.lastIndexOf(QLatin1Char('/'));
    if (slash <= 0) {
        return QStringLiteral(".");
    }
    return normalized.left(slash);
}

QString ensureRemoteDirCommand(const QString &os, const QString &path)
{
    const QString normalized = QDir::fromNativeSeparators(path);
    if (os == QStringLiteral("windows")) {
        const QString native = QDir::toNativeSeparators(normalized);
        QString escaped = native;
        escaped.replace(QLatin1Char('\''), QStringLiteral("''"));
        return QStringLiteral("powershell -NoProfile -Command \"New-Item -ItemType Directory -Force -Path '%1' | Out-Null\"")
            .arg(escaped);
    }
    return QStringLiteral("mkdir -p %1").arg(shellQuote(normalized));
}

QString backupRemoteFileCommand(const QString &os, const QString &sourcePath, const QString &backupPath)
{
    const QString source = QDir::fromNativeSeparators(sourcePath);
    const QString backup = QDir::fromNativeSeparators(backupPath);
    if (os == QStringLiteral("windows")) {
        QString sourceNative = QDir::toNativeSeparators(source);
        QString backupNative = QDir::toNativeSeparators(backup);
        sourceNative.replace(QLatin1Char('\''), QStringLiteral("''"));
        backupNative.replace(QLatin1Char('\''), QStringLiteral("''"));
        return QStringLiteral("powershell -NoProfile -Command \""
                              "if (Test-Path '%1') { "
                              "New-Item -ItemType Directory -Force -Path (Split-Path '%2' -Parent) | Out-Null; "
                              "Move-Item -Force '%1' '%2' }\"")
            .arg(sourceNative, backupNative);
    }
    return QStringLiteral("if [ -f %1 ]; then mkdir -p %2 && mv -f %1 %3; fi")
        .arg(shellQuote(source), shellQuote(remoteDirectoryOf(backup)), shellQuote(backup));
}

QString replaceRemoteFileCommand(const QString &os, const QString &path)
{
    const QString normalized = QDir::fromNativeSeparators(path);
    if (os == QStringLiteral("windows")) {
        QString native = QDir::toNativeSeparators(normalized);
        native.replace(QLatin1Char('\''), QStringLiteral("''"));
        return QStringLiteral("powershell -NoProfile -Command \"Remove-Item -Force '%1' -ErrorAction SilentlyContinue\"")
            .arg(native);
    }
    return QStringLiteral("rm -f %1").arg(shellQuote(normalized));
}

QString tailLines(const QString &text, int maxLines)
{
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (lines.size() <= maxLines) {
        return text.trimmed();
    }
    return lines.mid(lines.size() - maxLines).join(QLatin1Char('\n'));
}

void prependPathEntry(QProcessEnvironment systemEnv, QMap<QString, QString> *environment, const QString &entry)
{
    if (environment == nullptr || entry.isEmpty()) {
        return;
    }
#ifdef Q_OS_WIN
    const QChar separator = QLatin1Char(';');
    const QStringList keys = {QStringLiteral("Path"), QStringLiteral("PATH")};
#else
    const QChar separator = QLatin1Char(':');
    const QStringList keys = {QStringLiteral("PATH")};
#endif
    for (const QString &key : keys) {
        const QString existing = environment->contains(key) ? environment->value(key) : systemEnv.value(key);
        environment->insert(key, existing.isEmpty() ? entry : entry + separator + existing);
    }
}

QString resolveMavenExecutable(const AppSettings &settings)
{
    if (settings.mavenHome.isEmpty()) {
        return {};
    }
#ifdef Q_OS_WIN
    const QString executable = QStringLiteral("mvn.cmd");
#else
    const QString executable = QStringLiteral("mvn");
#endif
    const QString candidate = QDir(settings.mavenHome).filePath(QStringLiteral("bin/%1").arg(executable));
    return QFileInfo::exists(candidate) ? QDir::fromNativeSeparators(candidate) : QString();
}

QString applyMavenToCommand(QString command, const QString &mavenExecutable)
{
    const QString trimmed = command.trimmed();
    if (mavenExecutable.isEmpty() || !trimmed.startsWith(QStringLiteral("mvn"))) {
        return command;
    }
    const QString nativeExecutable = QDir::toNativeSeparators(mavenExecutable);
#ifdef Q_OS_WIN
    const QString quoted = QStringLiteral("\"%1\"").arg(nativeExecutable);
#else
    const QString quoted = QStringLiteral("'%1'").arg(nativeExecutable);
#endif
    if (trimmed == QStringLiteral("mvn")) {
        return quoted;
    }
    if (trimmed.startsWith(QStringLiteral("mvn "))) {
        return quoted + trimmed.mid(3);
    }
    return command;
}

QString appendMavenRepositoryArg(QString command, const QString &repository)
{
    if (repository.trimmed().isEmpty() || !command.contains(QStringLiteral("mvn"))) {
        return command;
    }
    const QString normalized = QDir::fromNativeSeparators(repository.trimmed());
    if (normalized.contains(QLatin1Char(' '))) {
        return command + QStringLiteral(" -Dmaven.repo.local=\"%1\"").arg(normalized);
    }
    return command + QStringLiteral(" -Dmaven.repo.local=%1").arg(normalized);
}

QString friendlyBuildFailureMessage(const QString &output, bool usedResolvedMaven)
{
    const QString lower = output.toLower();
    const bool javaMissing = lower.contains(QStringLiteral("java"))
        && (lower.contains(QStringLiteral("not recognized"))
            || output.contains(QStringLiteral("不是内部或外部命令"))
            || output.contains(QStringLiteral("JAVA_HOME")));
    if (javaMissing) {
        return QStringLiteral(
            "未找到 Java 运行环境。请在一键部署页选择已配置的 JDK（不要选「系统默认环境」），"
            "或确认 JDK 的 bin 目录已加入系统 PATH。");
    }

    if (usedResolvedMaven) {
        return output;
    }

    static const QRegularExpression bareMvnPattern(
        QStringLiteral(R"((^|[\r\n'"])mvn(['"\s]|$))"));
    if (bareMvnPattern.match(output).hasMatch()
        && (lower.contains(QStringLiteral("not recognized"))
            || output.contains(QStringLiteral("不是内部或外部命令"))
            || output.contains(QStringLiteral("No such file or directory")))) {
        return QStringLiteral(
            "未找到 mvn 命令。请在「设置」页配置 Maven 目录（例如 D:/install/apache-maven），"
            "或在项目构建命令中写 mvn 的完整路径。");
    }
    return output;
}

FailureStrategy parseFailureStrategy(const QJsonObject &project)
{
    const QString strategy = project.value(QStringLiteral("deploy")).toObject()
                                 .value(QStringLiteral("failureStrategy")).toString();
    return strategy == QStringLiteral("keep") ? FailureStrategy::Keep : FailureStrategy::Rollback;
}

ArtifactMatchPolicy parseArtifactPolicy(const QJsonObject &project)
{
    const QString policy = project.value(QStringLiteral("build")).toObject()
                               .value(QStringLiteral("artifactMatchPolicy")).toString();
    return policy == QStringLiteral("newest") ? ArtifactMatchPolicy::Newest : ArtifactMatchPolicy::FailIfMultiple;
}

void writeDeploymentLog(DeployOrchestrator::ActiveDeployment *deployment, const QString &line, QString *error)
{
    if (deployment == nullptr || deployment->logWriter == nullptr) {
        return;
    }
    deployment->logWriter->writeLine(line, error);
}

}

DeployWorker::DeployWorker(QString databasePath,
                           QString selectedJdkId,
                           RemoteConnectionContext connectionContext,
                           QObject *parent)
    : QObject(parent)
    , m_databasePath(std::move(databasePath))
    , m_selectedJdkId(std::move(selectedJdkId))
    , m_connectionContext(std::move(connectionContext))
{
}

void DeployWorker::run(const QString &projectId, const QString &serverId)
{
    ConfigStore store(m_databasePath);
    QString error;
    if (!store.open(&error)) {
        emit finished(false, error, {});
        return;
    }

    auto fail = [&](DeployOrchestrator::ActiveDeployment *deployment,
                    FailureStep step,
                    const QString &reason,
                    int progressValue) {
        QString persistError;
        if (deployment != nullptr) {
            deployment->job.fail(step, reason, false);
            writeDeploymentLog(deployment,
                               QStringLiteral("[%1] %2").arg(toString(step), reason),
                               &persistError);
            store.upsertDeployment(
                DeploymentRecord::applyJobState(deployment->record, deployment->job),
                &persistError);
        }
        emit progress(progressValue);
        emit finished(false, reason, deploymentLogPath(deployment));
    };

    emit progress(5);
    emit logLine(QStringLiteral("VALIDATE"), QStringLiteral("加载项目与服务器配置…"));

    QJsonObject project;
    QJsonObject server;
    if (!store.getProject(projectId, &project, &error)) {
        emit finished(false, error, {});
        return;
    }
    if (!store.getServer(serverId, &server, &error)) {
        emit finished(false, error, {});
        return;
    }

    const ValidationResult projectValidation = ConfigValidator::validateProject(project);
    if (!projectValidation.ok) {
        emit finished(false, projectValidation.errors.join(QStringLiteral("\n")), {});
        return;
    }
    const ValidationResult serverValidation = ConfigValidator::validateServer(server);
    if (!serverValidation.ok) {
        emit finished(false, serverValidation.errors.join(QStringLiteral("\n")), {});
        return;
    }

    const QJsonObject source = project.value(QStringLiteral("source")).toObject();
    if (source.value(QStringLiteral("kind")).toString() != QStringLiteral("local")) {
        emit finished(false, QStringLiteral("当前部署流程仅支持 Local 本地项目来源"), {});
        return;
    }

    const QString projectRoot = QDir::fromNativeSeparators(source.value(QStringLiteral("localPath")).toString());
    if (!QDir(projectRoot).exists()) {
        emit finished(false, QStringLiteral("本地项目目录不存在：%1").arg(projectRoot), {});
        return;
    }

    DeployOrchestrator orchestrator(store, DataPaths::configDir());
    DeployOrchestrator::ActiveDeployment deployment(parseFailureStrategy(project));
    if (!orchestrator.begin(projectId, serverId, parseFailureStrategy(project), &deployment, &error)) {
        emit finished(false, error, {});
        return;
    }
    emit deploymentLogReady(deploymentLogPath(&deployment));

    emit logLine(QStringLiteral("VALIDATE"),
                 QStringLiteral("项目 %1、服务器 %2 配置校验通过").arg(projectId, serverId));
    writeDeploymentLog(&deployment, QStringLiteral("[VALIDATE] configuration ok"), &error);

    emit progress(15);
    emit logLine(QStringLiteral("SOURCE"), QStringLiteral("使用本地目录：%1").arg(projectRoot));
    writeDeploymentLog(&deployment, QStringLiteral("[SOURCE] local path: ") + projectRoot, &error);

    const QJsonObject build = project.value(QStringLiteral("build")).toObject();
    const QString workingDirRelative = build.value(QStringLiteral("workingDir")).toString(QStringLiteral("."));
    const QString workingDirectory = QDir(projectRoot).filePath(workingDirRelative);
    const QString command = build.value(QStringLiteral("command")).toString();
    const QString artifactGlob = build.value(QStringLiteral("artifactPath")).toString();
    const bool uploadPrebuilt = build.value(QStringLiteral("mode")).toString() == QStringLiteral("prebuilt-jar");

    if (!uploadPrebuilt && !QDir(workingDirectory).exists()) {
        fail(&deployment,
             FailureStep::Validate,
             QStringLiteral("构建工作目录不存在：%1").arg(workingDirectory),
             15);
        return;
    }

    QString artifactPath;
    if (uploadPrebuilt) {
        artifactPath = QDir::fromNativeSeparators(artifactGlob);
        if (!QFileInfo::exists(artifactPath) || !QFileInfo(artifactPath).isFile()) {
            fail(&deployment,
                 FailureStep::Validate,
                 QStringLiteral("本地 JAR 不存在：%1").arg(artifactPath),
                 25);
            return;
        }
        emit progress(55);
        emit logLine(QStringLiteral("BUILD"), QStringLiteral("跳过构建，使用本地 JAR：%1").arg(artifactPath));
        writeDeploymentLog(&deployment, QStringLiteral("[BUILD] prebuilt jar: ") + artifactPath, &error);
    } else {
        deployment.job.transitionTo(DeploymentStatus::Building);
        orchestrator.persistJobState(&deployment, &error);

        emit progress(25);
        emit logLine(QStringLiteral("BUILD"),
                     QStringLiteral("执行：%1（目录：%2）").arg(command, workingDirectory));
        writeDeploymentLog(&deployment, QStringLiteral("[BUILD] command: ") + command, &error);

        BuildRequest request;
        request.workingDirectory = workingDirectory;
        request.command = command;
        request.timeoutSec = build.value(QStringLiteral("timeoutSec")).toInt(600);
        const QJsonObject envObject = build.value(QStringLiteral("env")).toObject();
        for (auto it = envObject.begin(); it != envObject.end(); ++it) {
            request.environment.insert(it.key(), it.value().toString());
        }

        AppSettings settings;
        QString settingsError;
        AppSettingsStore(AppSettingsStore::defaultSettingsFile()).load(&settings, &settingsError);
        const QProcessEnvironment systemEnv = QProcessEnvironment::systemEnvironment();
        const QString mavenExecutable = resolveMavenExecutable(settings);
        bool usedResolvedMaven = false;
        if (!settings.mavenHome.isEmpty()) {
            request.environment.insert(QStringLiteral("MAVEN_HOME"), settings.mavenHome);
            request.environment.insert(QStringLiteral("M2_HOME"), settings.mavenHome);
            prependPathEntry(systemEnv, &request.environment, settings.mavenHome + QStringLiteral("/bin"));
            emit logLine(QStringLiteral("BUILD"), QStringLiteral("使用 Maven：%1").arg(settings.mavenHome));
            if (mavenExecutable.isEmpty()) {
                fail(&deployment,
                     FailureStep::Validate,
                     QStringLiteral("Maven 目录无效：未找到 %1/bin/mvn.cmd，请填写 Maven 安装根目录（包含 bin 子目录）")
                         .arg(settings.mavenHome),
                     25);
                return;
            }
        } else if (command.contains(QStringLiteral("mvn"))) {
            emit logLine(QStringLiteral("BUILD"),
                         QStringLiteral("提示：未配置 Maven 目录，将依赖系统 PATH 查找 mvn"));
        }
        if (!mavenExecutable.isEmpty()) {
            request.command = applyMavenToCommand(request.command, mavenExecutable);
            usedResolvedMaven = true;
        }
        if (!settings.mavenRepository.isEmpty()) {
            request.command = appendMavenRepositoryArg(request.command, settings.mavenRepository);
            emit logLine(QStringLiteral("BUILD"), QStringLiteral("使用 Maven 仓库：%1").arg(settings.mavenRepository));
        }

        if (!m_selectedJdkId.isEmpty() && m_selectedJdkId != QStringLiteral("system")) {
            QVector<JdkProfile> profiles;
            QString jdkError;
            JdkProfileStore jdkStore(DataPaths::jdkProfilesFile());
            if (!jdkStore.load(&profiles, &jdkError)) {
                fail(&deployment, FailureStep::Validate, jdkError, 25);
                return;
            }
            auto it = std::find_if(profiles.cbegin(), profiles.cend(), [this](const JdkProfile &profile) {
                return profile.id == m_selectedJdkId;
            });
            if (it == profiles.cend()) {
                fail(&deployment,
                     FailureStep::Validate,
                     QStringLiteral("未找到 JDK 配置：%1").arg(m_selectedJdkId),
                     25);
                return;
            }
            request.environment.insert(QStringLiteral("JAVA_HOME"), QDir::fromNativeSeparators(it->path));
            prependPathEntry(systemEnv, &request.environment, QDir(it->path).filePath(QStringLiteral("bin")));
            emit logLine(QStringLiteral("BUILD"),
                         QStringLiteral("使用 JDK：%1 (%2)").arg(it->version, it->path));
            writeDeploymentLog(&deployment,
                               QStringLiteral("[BUILD] jdk: %1 %2").arg(it->version, it->path),
                               &error);
        } else if (command.contains(QStringLiteral("mvn"))) {
            emit logLine(QStringLiteral("BUILD"),
                         QStringLiteral("提示：当前使用系统默认 JDK，若构建失败请在一键部署页选择具体 JDK"));
        }

        emit logLine(QStringLiteral("BUILD"), QStringLiteral("实际命令：%1").arg(request.command));
        writeDeploymentLog(&deployment, QStringLiteral("[BUILD] resolved command: ") + request.command, &error);

        const BuildResult buildResult = LocalBuilder().run(request);
        if (!buildResult.ok) {
            const QString rawOutput = buildResult.stderrText.trimmed().isEmpty()
                ? buildResult.stdoutText.trimmed()
                : buildResult.stderrText.trimmed();
            const QString output = friendlyBuildFailureMessage(rawOutput, usedResolvedMaven);
            if (!buildResult.stdoutText.trimmed().isEmpty()) {
                writeDeploymentLog(&deployment, QStringLiteral("[BUILD:stdout]\n") + buildResult.stdoutText.trimmed(), &error);
            }
            if (!buildResult.stderrText.trimmed().isEmpty()) {
                writeDeploymentLog(&deployment, QStringLiteral("[BUILD:stderr]\n") + buildResult.stderrText.trimmed(), &error);
            }
            const QString detail = output.isEmpty()
                ? buildResult.error
                : QStringLiteral("%1\n%2").arg(buildResult.error, tailLines(output, 8));
            emit logLine(QStringLiteral("BUILD"), detail);
            fail(&deployment, FailureStep::Building, detail, 40);
            return;
        }

        emit progress(55);
        emit logLine(QStringLiteral("BUILD"), QStringLiteral("构建成功，退出码 0"));
        writeDeploymentLog(&deployment, QStringLiteral("[BUILD] exit code 0"), &error);

        const ArtifactMatchResult artifactResult = ArtifactMatcher::match(
            projectRoot,
            artifactGlob,
            parseArtifactPolicy(project));

        if (!artifactResult.ok) {
            const QString artifactError = QStringLiteral("产物匹配失败（规则：%1）：%2")
                                              .arg(artifactGlob, artifactResult.error);
            emit logLine(QStringLiteral("BUILD"), artifactError);
            fail(&deployment, FailureStep::Building, artifactError, 60);
            return;
        }

        artifactPath = artifactResult.paths.first();
    }
    emit progress(70);
    emit logLine(QStringLiteral("BUILD"), QStringLiteral("产物：%1").arg(artifactPath));
    writeDeploymentLog(&deployment, QStringLiteral("[BUILD] artifact: ") + artifactPath, &error);

    deployment.job.transitionTo(DeploymentStatus::Uploading);
    orchestrator.persistJobState(&deployment, &error);

    const QString os = server.value(QStringLiteral("os")).toString();
    const QJsonObject deploy = project.value(QStringLiteral("deploy")).toObject();
    const QString remoteBaseDir = deploy.value(QStringLiteral("remoteBaseDir")).toString();
    const QString version = deployment.version;
    const QString artifactFileName = QFileInfo(artifactPath).fileName();
    const ProjectServiceConfig serviceConfig = projectServiceConfig(project);
    const QString remoteArtifact = remoteProjectJarPath(project, artifactFileName);
    const QString backupPath = remoteProjectBackupPath(project, artifactFileName, version);

    emit progress(80);
    emit logLine(QStringLiteral("UPLOAD"), QStringLiteral("准备上传到：%1").arg(remoteArtifact));

    auto executor = createRemoteExecutor(m_connectionContext);
    if (!executor) {
        fail(&deployment, FailureStep::Uploading, QStringLiteral("不支持的服务器类型：%1").arg(os), 80);
        return;
    }

    const RemoteCommandResult ensureDir = executor->execute(ensureRemoteDirCommand(os, remoteDirectoryOf(remoteArtifact)), 30);
    if (!ensureDir.ok) {
        const QString dirError = ensureDir.error.isEmpty() ? ensureDir.stderrText.trimmed() : ensureDir.error;
        emit logLine(QStringLiteral("UPLOAD"), dirError);
        fail(&deployment, FailureStep::Uploading, dirError, 82);
        return;
    }
    writeDeploymentLog(&deployment, QStringLiteral("[UPLOAD] target dir ready: ") + remoteDirectoryOf(remoteArtifact), &error);

    if (serviceConfig.backupPolicy == QStringLiteral("backup")) {
        emit logLine(QStringLiteral("UPLOAD"), QStringLiteral("备份旧 JAR 到：%1").arg(backupPath));
        writeDeploymentLog(&deployment, QStringLiteral("[UPLOAD] backup old jar to: ") + backupPath, &error);
        const RemoteCommandResult backup = executor->execute(backupRemoteFileCommand(os, remoteArtifact, backupPath), 45);
        if (!backup.ok) {
            const QString backupError = backup.error.isEmpty() ? backup.stderrText.trimmed() : backup.error;
            emit logLine(QStringLiteral("UPLOAD"), backupError);
            fail(&deployment, FailureStep::Uploading, backupError, 83);
            return;
        }
    } else {
        writeDeploymentLog(&deployment, QStringLiteral("[UPLOAD] replace old jar: ") + remoteArtifact, &error);
        const RemoteCommandResult removeOld = executor->execute(replaceRemoteFileCommand(os, remoteArtifact), 30);
        if (!removeOld.ok) {
            const QString replaceError = removeOld.error.isEmpty() ? removeOld.stderrText.trimmed() : removeOld.error;
            emit logLine(QStringLiteral("UPLOAD"), replaceError);
            fail(&deployment, FailureStep::Uploading, replaceError, 83);
            return;
        }
    }

    const UploadResult uploadResult = executor->uploadFile(artifactPath, remoteArtifact);
    if (!uploadResult.ok) {
        const QString uploadError = QStringLiteral("%1（本地构建与产物校验已完成，产物：%2）")
                                        .arg(uploadResult.error, artifactPath);
        emit logLine(QStringLiteral("UPLOAD"), uploadError);
        fail(&deployment, FailureStep::Uploading, uploadError, 85);
        return;
    }

    deployment.job.transitionTo(DeploymentStatus::Restarting);
    orchestrator.persistJobState(&deployment, &error);

    emit progress(92);
    emit logLine(QStringLiteral("UPLOAD"), QStringLiteral("上传完成：%1").arg(remoteArtifact));
    writeDeploymentLog(&deployment, QStringLiteral("[UPLOAD] completed"), &error);

    auto monitor = createRemoteMonitor(m_connectionContext);
    const ServiceControlResult restart = monitor->restartService(server, project);
    if (!restart.ok) {
        const QString restartError = restart.error.isEmpty() ? QStringLiteral("服务重启失败") : restart.error;
        emit logLine(QStringLiteral("RESTART"), restartError);
        fail(&deployment, FailureStep::Restarting, restartError, 95);
        return;
    }

    emit progress(100);
    emit logLine(QStringLiteral("RESTART"), restart.output.isEmpty() ? QStringLiteral("服务重启命令已执行") : restart.output);
    writeDeploymentLog(&deployment,
                       restart.output.isEmpty()
                           ? QStringLiteral("[RESTART] command executed")
                           : QStringLiteral("[RESTART] ") + restart.output,
                       &error);

    const QStringList artifactNames{ artifactFileName };
    if (!orchestrator.completeSuccess(&deployment,
                                      remoteDirectoryOf(remoteArtifact),
                                      artifactNames,
                                      &error)) {
        emit finished(false, error, deploymentLogPath(&deployment));
        return;
    }

    emit logLine(QStringLiteral("DONE"), QStringLiteral("部署完成"));
    emit finished(true,
                 QStringLiteral("部署成功，产物：%1").arg(artifactPath),
                 deploymentLogPath(&deployment));
}
