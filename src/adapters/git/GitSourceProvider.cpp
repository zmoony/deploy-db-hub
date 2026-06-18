#include "adapters/git/GitSourceProvider.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

namespace {
constexpr int GitTimeoutMs = 120000;
constexpr int UnshallowTimeoutMs = 300000;

const QRegularExpression CommitShaPattern(QStringLiteral("^[0-9a-fA-F]{7,40}$"));

bool runGit(QProcess &process, const QString &workingDir, const QStringList &args, int timeoutMs, QString *error)
{
    process.setWorkingDirectory(workingDir);
    process.start(QStringLiteral("git"), args);
    if (!process.waitForFinished(timeoutMs)) {
        if (error) {
            *error = QStringLiteral("Git 拉取超时，请检查网络后重试");
        }
        return false;
    }
    if (process.exitCode() != 0) {
        if (error) {
            const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
            *error = stderrText.isEmpty() ? QStringLiteral("Git 拉取或切换失败") : stderrText;
        }
        return false;
    }
    return true;
}

bool checkoutSha(QProcess &process, const QString &workingDir, const QString &sha, QString *error)
{
    if (runGit(process, workingDir, {QStringLiteral("fetch"), QStringLiteral("--depth"), QStringLiteral("1"),
                                    QStringLiteral("origin"), sha},
               GitTimeoutMs, nullptr)) {
        return runGit(process, workingDir, {QStringLiteral("checkout"), QStringLiteral("-f"), sha}, GitTimeoutMs, error);
    }

    if (!runGit(process, workingDir, {QStringLiteral("fetch"), QStringLiteral("--unshallow")}, UnshallowTimeoutMs, nullptr)) {
        if (error) {
            *error = QStringLiteral("分支/tag/commit 无效，请检查 ref");
        }
        return false;
    }

    if (!runGit(process, workingDir, {QStringLiteral("fetch"), QStringLiteral("origin"), sha}, GitTimeoutMs, nullptr)) {
        if (error) {
            *error = QStringLiteral("分支/tag/commit 无效，请检查 ref");
        }
        return false;
    }

    return runGit(process, workingDir, {QStringLiteral("checkout"), QStringLiteral("-f"), sha}, GitTimeoutMs, error);
}

bool checkoutBranchOrTag(QProcess &process, const QString &workingDir, const QString &ref, QString *error)
{
    if (!runGit(process, workingDir, {QStringLiteral("fetch"), QStringLiteral("origin")}, GitTimeoutMs, error)) {
        return false;
    }
    if (!runGit(process, workingDir, {QStringLiteral("checkout"), QStringLiteral("-f"), ref}, GitTimeoutMs, error)) {
        return false;
    }

    QProcess symbolicRef;
    symbolicRef.setWorkingDirectory(workingDir);
    symbolicRef.start(QStringLiteral("git"), {QStringLiteral("symbolic-ref"), QStringLiteral("-q"), QStringLiteral("HEAD")});
    if (symbolicRef.waitForFinished(10000) && symbolicRef.exitCode() == 0) {
        return runGit(process, workingDir, {QStringLiteral("pull"), QStringLiteral("--ff-only")}, GitTimeoutMs, error);
    }
    return true;
}
}

bool GitSourceProvider::isCommitSha(const QString &ref)
{
    return CommitShaPattern.match(ref).hasMatch();
}

SourceResult GitSourceProvider::prepare(const QString &workspaceRoot, const QString &projectId, const QString &repoUrl, const QString &ref) const
{
    SourceResult result;
    if (workspaceRoot.isEmpty() || projectId.isEmpty() || repoUrl.isEmpty() || ref.isEmpty()) {
        result.error = QStringLiteral("GitHub 来源配置不完整");
        return result;
    }

    const QString targetDir = QDir(workspaceRoot).filePath(QStringLiteral("repos/") + projectId);
    QDir().mkpath(QFileInfo(targetDir).absolutePath());

    QProcess git;
    const bool existingRepo = QDir(targetDir).exists(QStringLiteral(".git"));
    const bool isSha = isCommitSha(ref);

    if (existingRepo) {
        git.setWorkingDirectory(targetDir);
        if (isSha) {
            if (!checkoutSha(git, targetDir, ref, &result.error)) {
                return result;
            }
        } else if (!checkoutBranchOrTag(git, targetDir, ref, &result.error)) {
            return result;
        }
    } else if (isSha) {
        if (!runGit(git, workspaceRoot,
                    {QStringLiteral("clone"), QStringLiteral("--depth"), QStringLiteral("1"), repoUrl, targetDir},
                    GitTimeoutMs, &result.error)) {
            return result;
        }
        if (!checkoutSha(git, targetDir, ref, &result.error)) {
            return result;
        }
    } else {
        if (!runGit(git, workspaceRoot,
                    {QStringLiteral("clone"), QStringLiteral("--depth"), QStringLiteral("1"),
                     QStringLiteral("--branch"), ref, repoUrl, targetDir},
                    GitTimeoutMs, &result.error)) {
            return result;
        }
    }

    result.ok = true;
    result.workingTree = targetDir;
    return result;
}
