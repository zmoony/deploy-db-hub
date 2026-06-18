#include "ui/RemoteCredentialResolver.h"

#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"

#include <QInputDialog>
#include <QLineEdit>

QString RemoteCredentialResolver::credentialRefFor(const QJsonObject &serverConfig)
{
    const QString configured = serverConfig.value(QStringLiteral("auth")).toObject()
        .value(QStringLiteral("credentialRef")).toString();
    if (!configured.isEmpty()) {
        return configured;
    }
    const QString id = serverConfig.value(QStringLiteral("id")).toString();
    const QString username = serverConfig.value(QStringLiteral("username")).toString();
    const QString host = serverConfig.value(QStringLiteral("host")).toString();
    if (!id.isEmpty()) {
        return QStringLiteral("deploy-hub/session/") + id;
    }
    if (!username.isEmpty() || !host.isEmpty()) {
        return QStringLiteral("deploy-hub/session/") + username + QLatin1Char('@') + host;
    }
    return {};
}

RemoteConnectionContext RemoteCredentialResolver::resolve(const QJsonObject &serverConfig,
                                                           CredentialStore *store,
                                                           CredentialSessionCache *sessionCache,
                                                           QWidget *promptParent,
                                                           bool allowPrompt)
{
    RemoteConnectionContext context;
    context.serverConfig = serverConfig;

    const QJsonObject auth = serverConfig.value(QStringLiteral("auth")).toObject();
    const QString mode = auth.value(QStringLiteral("mode")).toString();

    if (mode == QStringLiteral("ssh-key")) {
        return context;
    }

    const QString ref = credentialRefFor(serverConfig);
    if (!ref.isEmpty()) {
        if (sessionCache != nullptr) {
            const QString cached = sessionCache->get(ref);
            if (!cached.isEmpty()) {
                context.password = cached;
                return context;
            }
        }
        if (store != nullptr) {
            const QString stored = store->load(ref);
            if (!stored.isEmpty()) {
                context.password = stored;
                if (sessionCache != nullptr) {
                    sessionCache->put(ref, stored);
                }
                return context;
            }
        }
    }

    if (!allowPrompt || promptParent == nullptr) {
        return context;
    }

    bool ok = false;
    const QString host = serverConfig.value(QStringLiteral("host")).toString();
    const QString username = serverConfig.value(QStringLiteral("username")).toString();
    const QString password = QInputDialog::getText(promptParent,
                                                   QStringLiteral("输入服务器密码"),
                                                   QStringLiteral("请输入 %1@%2 的登录密码：").arg(username, host),
                                                   QLineEdit::Password,
                                                   QString(),
                                                   &ok);
    if (ok && !password.isEmpty()) {
        context.password = password;
        if (sessionCache != nullptr && !ref.isEmpty()) {
            sessionCache->put(ref, password);
        }
    }

    return context;
}
