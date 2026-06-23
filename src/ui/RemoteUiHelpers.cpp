#include "ui/RemoteUiHelpers.h"

#include <QApplication>
#include <QMessageBox>
#include <QPointer>
#include <QThread>

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

HostKeyPromptHandler makeThreadSafeHostKeyPromptHandler(QWidget *parent)
{
    QPointer<QWidget> parentGuard(parent);
    return [parentGuard](const QString &fingerprintSha256, QString *error) {
        if (QThread::currentThread() == qApp->thread()) {
            return makeHostKeyPromptHandler(parentGuard.data())(fingerprintSha256, error);
        }

        struct PromptState {
            bool accepted = false;
            QString error;
        } state;

        QMetaObject::invokeMethod(qApp, [&]() {
            QWidget *dialogParent = parentGuard.data();
            if (dialogParent == nullptr) {
                state.error = QStringLiteral("窗口已关闭");
                return;
            }
            state.accepted = makeHostKeyPromptHandler(dialogParent)(fingerprintSha256, &state.error);
        }, Qt::BlockingQueuedConnection);

        if (!state.accepted && error != nullptr && error->isEmpty()) {
            *error = state.error;
        }
        return state.accepted;
    };
}
