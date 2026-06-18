#include "SshClientTest.h"

#include "adapters/ssh/SshClient.h"

#include <QJsonObject>
#include <QTest>

namespace {

RemoteConnectionContext linuxPasswordContext()
{
    RemoteConnectionContext context;
    context.password = QStringLiteral("secret");
    context.serverConfig = {
        {QStringLiteral("host"), QStringLiteral("172.18.12.208")},
        {QStringLiteral("port"), 22},
        {QStringLiteral("username"), QStringLiteral("root")},
        {QStringLiteral("auth"), QJsonObject{
            {QStringLiteral("mode"), QStringLiteral("manual")}
        }}
    };
    return context;
}

bool containsOptionPrefix(const QStringList &args, const QString &prefix)
{
    return std::any_of(args.cbegin(), args.cend(), [&](const QString &arg) {
        return arg.startsWith(prefix);
    });
}

}

void SshClientTest::windowsSshArgsDoNotEnableConnectionSharing()
{
    SshClient client(linuxPasswordContext());
    SshInteractiveInvocation invocation;
    QString error;

    QVERIFY2(client.buildInteractiveInvocation(&invocation, &error), qPrintable(error));

#ifdef Q_OS_WIN
    QVERIFY(!invocation.args.contains(QStringLiteral("ControlMaster=auto")));
    QVERIFY(!containsOptionPrefix(invocation.args, QStringLiteral("ControlPath=")));
    QVERIFY(!invocation.args.contains(QStringLiteral("ControlPersist=120")));
#else
    QVERIFY(invocation.args.contains(QStringLiteral("ControlMaster=auto")));
    QVERIFY(containsOptionPrefix(invocation.args, QStringLiteral("ControlPath=")));
    QVERIFY(invocation.args.contains(QStringLiteral("ControlPersist=120")));
#endif
}
