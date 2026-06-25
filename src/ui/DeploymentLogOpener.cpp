#include "ui/DeploymentLogOpener.h"

#include "adapters/remote/RemoteFileBrowser.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "infra/LocalLogCatalog.h"
#include "infra/RemoteLogPath.h"
#include "ui/DeploymentLogDialog.h"
#include "ui/RemoteCredentialResolver.h"
#include "ui/RemoteFileViewerDialog.h"
#include "ui/RemoteUiHelpers.h"

#include <QDir>
#include <QInputDialog>
#include <QMessageBox>

namespace DeploymentLogOpener {

QString resolveLogPathForProject(const QJsonObject &project, QWidget *parent, bool *ok)
{
    if (ok != nullptr) {
        *ok = false;
    }

    QString logPath = defaultRemoteLogPath(project);
    if (!logPath.isEmpty()) {
        if (ok != nullptr) {
            *ok = true;
        }
        return logPath;
    }

    const QStringList options = deployLogPathOptionsForProject(project);
    if (options.isEmpty()) {
        QMessageBox::information(parent,
                                 QStringLiteral("查看日志"),
                                 QStringLiteral("请先在项目配置中填写「日志目录」。"));
        return {};
    }
    if (options.size() == 1) {
        if (ok != nullptr) {
            *ok = true;
        }
        return options.first();
    }

    bool accepted = false;
    const QString picked = QInputDialog::getItem(parent,
                                                 QStringLiteral("选择日志路径"),
                                                 QStringLiteral("该项目存在多个候选日志路径，请选择："),
                                                 options,
                                                 0,
                                                 false,
                                                 &accepted);
    if (!accepted || picked.isEmpty()) {
        return {};
    }
    if (ok != nullptr) {
        *ok = true;
    }
    return picked;
}

void open(QWidget *parent,
          CredentialStore *credentials,
          CredentialSessionCache *sessionCache,
          const QJsonObject &server,
          const QString &logPath,
          AiSettingsStore *aiSettings)
{
    AiSettingsStore localAiSettings(AiSettingsStore::defaultSettingsFile());
    AiSettingsStore *effectiveAiSettings = aiSettings != nullptr ? aiSettings : &localAiSettings;
    const QString trimmed = logPath.trimmed();
    if (trimmed.isEmpty()) {
        QMessageBox::information(parent,
                                 QStringLiteral("查看日志"),
                                 QStringLiteral("请选择或输入日志路径，例如 /home/app/logs/*.log 或 /home/app/logs/*.txt"));
        return;
    }

    if (isRemoteDeployLogPath(trimmed)) {
        if (server.isEmpty()) {
            QMessageBox::information(parent,
                                   QStringLiteral("查看日志"),
                                   QStringLiteral("无法解析目标服务器，请检查项目部署配置。"));
            return;
        }

        RemoteConnectionContext context =
            RemoteCredentialResolver::resolve(server, credentials, sessionCache, parent, true);
        const QJsonObject auth = server.value(QStringLiteral("auth")).toObject();
        if (auth.value(QStringLiteral("mode")).toString() != QStringLiteral("ssh-key")
            && context.password.isEmpty()) {
            QMessageBox::warning(parent,
                                 QStringLiteral("无法打开远程日志"),
                                 QStringLiteral("未获取到服务器密码，请先在「部署工具 → 服务器管理」中保存密码。"));
            return;
        }

        auto browser = createRemoteLogFileBrowser(context, makeHostKeyPromptHandler(parent));
        if (!browser) {
            QMessageBox::warning(parent,
                                 QStringLiteral("无法打开远程日志"),
                                 QStringLiteral("远程日志查看当前仅支持 Linux SSH 或 Windows WinRM 服务器。"));
            return;
        }

        RemoteFileViewerDialog dialog(std::move(browser),
                                      QDir::fromNativeSeparators(trimmed),
                                      effectiveAiSettings,
                                      credentials,
                                      parent);
        dialog.exec();
        return;
    }

    if (!LocalLogCatalog::isValidRelativePath(trimmed)) {
        QMessageBox::warning(parent,
                             QStringLiteral("路径无效"),
                             QStringLiteral("日志路径格式应为 logs/<filename>.log 或远程绝对路径。"));
        return;
    }

    QString loadError;
    if (!DeploymentLogDialog::canOpen(trimmed)) {
        DeploymentLogDialog::loadContent(trimmed, &loadError);
        QMessageBox::warning(parent, QStringLiteral("无法打开日志"), loadError);
        return;
    }

    DeploymentLogDialog dialog(trimmed, effectiveAiSettings, credentials, parent);
    dialog.exec();
}

}
