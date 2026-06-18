#include "AppStyleTest.h"
#include "AppSettingsStoreTest.h"
#include "ArtifactMatcherTest.h"
#include "ConfigStoreTest.h"
#include "ConfigValidatorTest.h"
#include "DeployJobTest.h"
#include "DeployLogPathOptionsTest.h"
#include "DeployOrchestratorTest.h"
#include "DeploymentRecordTest.h"
#include "GitSourceProviderTest.h"
#include "JdkProfileStoreTest.h"
#include "LocalLogCatalogTest.h"
#include "LogPathTest.h"
#include "LogSanitizerTest.h"
#include "ProcessOutputDecoderTest.h"
#include "ProjectServiceConfigTest.h"
#include "RemoteFileBrowserTest.h"
#include "RemoteTerminalWidgetTest.h"
#include "SshClientTest.h"
#include "VersionTest.h"

#include <QApplication>
#include <QDir>
#include <QTest>

namespace {
template <typename Test>
int runTest(const char *name, int argc, char **argv)
{
    Test test;
    if (argc > 1) {
        return QTest::qExec(&test, argc, argv);
    }

    char appName[] = "deploy_hub_tests";
    char outputFlag[] = "-o";
    QByteArray outputPath = QDir::current().filePath(QStringLiteral("test-") + QString::fromLatin1(name) + QStringLiteral(".txt")).toLocal8Bit() + ",txt";
    char *args[] = {appName, outputFlag, outputPath.data()};
    return QTest::qExec(&test, 3, args);
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    int status = 0;
    status |= runTest<AppSettingsStoreTest>("AppSettingsStoreTest", argc, argv);
    status |= runTest<AppStyleTest>("AppStyleTest", argc, argv);
    status |= runTest<ArtifactMatcherTest>("ArtifactMatcherTest", argc, argv);
    status |= runTest<ConfigStoreTest>("ConfigStoreTest", argc, argv);
    status |= runTest<ConfigValidatorTest>("ConfigValidatorTest", argc, argv);
    status |= runTest<DeployLogPathOptionsTest>("DeployLogPathOptionsTest", argc, argv);
    status |= runTest<DeploymentRecordTest>("DeploymentRecordTest", argc, argv);
    status |= runTest<DeployOrchestratorTest>("DeployOrchestratorTest", argc, argv);
    status |= runTest<DeployJobTest>("DeployJobTest", argc, argv);
    status |= runTest<GitSourceProviderTest>("GitSourceProviderTest", argc, argv);
    status |= runTest<JdkProfileStoreTest>("JdkProfileStoreTest", argc, argv);
    status |= runTest<LocalLogCatalogTest>("LocalLogCatalogTest", argc, argv);
    status |= runTest<LogPathTest>("LogPathTest", argc, argv);
    status |= runTest<LogSanitizerTest>("LogSanitizerTest", argc, argv);
    status |= runTest<ProcessOutputDecoderTest>("ProcessOutputDecoderTest", argc, argv);
    status |= runTest<ProjectServiceConfigTest>("ProjectServiceConfigTest", argc, argv);
    status |= runTest<RemoteFileBrowserTest>("RemoteFileBrowserTest", argc, argv);
    status |= runTest<RemoteTerminalWidgetTest>("RemoteTerminalWidgetTest", argc, argv);
    status |= runTest<SshClientTest>("SshClientTest", argc, argv);
    status |= runTest<VersionTest>("VersionTest", argc, argv);

    return status;
}
