#include "ui/RemoteUiHelpers.h"

#include <QMessageBox>

HostKeyPromptHandler makeHostKeyPromptHandler(QWidget *parent)
{
    return [parent](const QString &fingerprintSha256, QString *error) {
        const auto answer = QMessageBox::question(parent,
                                                  QStringLiteral("信任服务器 host key"),
                                                  QStringLiteral("首次连接该服务器，是否信任以下 SHA256 指纹？\n\n%1")
                                                      .arg(fingerprintSha256));
        if (answer != QMessageBox::Yes) {
            if (error != nullptr) {
                *error = QStringLiteral("用户拒绝信任服务器 host key");
            }
            return false;
        }
        return true;
    };
}
