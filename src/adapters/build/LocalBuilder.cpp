#include "adapters/build/LocalBuilder.h"

#include "infra/ProcessOutputDecoder.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include <functional>

namespace {

struct ParsedCommand {
    QString program;
    QStringList arguments;
    bool useShell = false;
};

ParsedCommand parseCommand(const QString &command)
{
    ParsedCommand parsed;
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty()) {
        return parsed;
    }

#ifdef Q_OS_WIN
    if (trimmed.startsWith(QLatin1Char('"')) && trimmed.size() > 2) {
        const int closingQuote = trimmed.indexOf(QLatin1Char('"'), 1);
        if (closingQuote > 1) {
            parsed.program = QDir::toNativeSeparators(trimmed.mid(1, closingQuote - 1));
            const QString remainder = trimmed.mid(closingQuote + 1).trimmed();
            if (!remainder.isEmpty()) {
                parsed.arguments = QProcess::splitCommand(remainder);
            }
            if (QFileInfo::exists(parsed.program)) {
                return parsed;
            }
        }
    }
    parsed.program = QStringLiteral("cmd");
    parsed.arguments = {QStringLiteral("/c"), command};
    parsed.useShell = true;
    return parsed;
#else
    parsed.program = QStringLiteral("/bin/sh");
    parsed.arguments = {QStringLiteral("-c"), command};
    parsed.useShell = true;
    return parsed;
#endif
}

}

BuildResult LocalBuilder::run(const BuildRequest &request) const
{
    BuildResult result;
    if (request.command.trimmed().isEmpty()) {
        result.error = QStringLiteral("构建命令不能为空");
        return result;
    }

    const ParsedCommand parsed = parseCommand(request.command);
    if (parsed.program.isEmpty()) {
        result.error = QStringLiteral("构建命令不能为空");
        return result;
    }

    QProcess process;
    process.setWorkingDirectory(request.workingDirectory);

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    for (auto it = request.environment.cbegin(); it != request.environment.cend(); ++it) {
        environment.insert(it.key(), it.value());
    }
    process.setProcessEnvironment(environment);

    process.start(parsed.program, parsed.arguments);

    if (!process.waitForStarted()) {
        result.error = QStringLiteral("构建进程启动失败：%1").arg(process.errorString());
        return result;
    }

    if (request.shouldCancel) {
        const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + qint64(request.timeoutSec) * 1000;
        while (process.state() != QProcess::NotRunning) {
            if (request.shouldCancel()) {
                process.kill();
                process.waitForFinished(2000);
                result.error = QStringLiteral("构建已取消");
                return result;
            }
            const qint64 remaining = deadline - QDateTime::currentMSecsSinceEpoch();
            if (remaining <= 0) {
                process.kill();
                process.waitForFinished();
                result.error = QStringLiteral("构建超时");
                return result;
            }
            if (!process.waitForFinished(qMin<qint64>(500, remaining))) {
                if (process.state() == QProcess::NotRunning) {
                    break;
                }
            }
        }
    } else if (!process.waitForFinished(request.timeoutSec * 1000)) {
        process.kill();
        process.waitForFinished();
        result.error = QStringLiteral("构建超时");
        return result;
    }

    result.exitCode = process.exitCode();
    result.stdoutText = decodeProcessOutput(process.readAllStandardOutput());
    result.stderrText = decodeProcessOutput(process.readAllStandardError());
    result.ok = process.exitStatus() == QProcess::NormalExit && result.exitCode == 0;
    if (!result.ok && result.error.isEmpty()) {
        result.error = QStringLiteral("构建失败");
    }
    return result;
}
