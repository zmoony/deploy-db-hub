cmake_minimum_required(VERSION 3.21)

project(deploy_hub VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 QuickEffects Qml QuickWidgets Sql Test Svg Concurrent Network PrintSupport)

qt_standard_project_setup()

option(DEPLOY_HUB_USE_FLUENTUI "Build with QFluentUI from third_party/FluentUI" OFF)
if(DEPLOY_HUB_USE_FLUENTUI)
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/FluentUI/CMakeLists.txt")
        message(FATAL_ERROR "DEPLOY_HUB_USE_FLUENTUI=ON but third_party/FluentUI is missing. See third_party/FluentUI/README.md")
    endif()
    set(FLUENTUI_BUILD_STATIC_LIB OFF CACHE BOOL "" FORCE)
    set(FLUENTUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(FLUENTUI_QML_PLUGIN_DIRECTORY "${CMAKE_BINARY_DIR}/FluentUI" CACHE PATH "" FORCE)
    add_subdirectory(third_party/FluentUI)
endif()

add_library(deploy_hub_core
    src/adapters/build/LocalBuilder.cpp
    src/adapters/git/GitSourceProvider.cpp
    src/adapters/remote/RemoteExecutor.cpp
    src/adapters/remote/RemoteFileBrowser.cpp
    src/adapters/remote/RemoteMonitor.cpp
    src/adapters/remote/RemoteMonitorCommon.cpp
    src/adapters/ssh/SftpRemoteFileBrowser.cpp
    src/adapters/ssh/SshClient.cpp
    src/adapters/ssh/SshRemoteExecutor.cpp
    src/adapters/ssh/SshRemoteMonitor.cpp
    src/adapters/winrm/WinRmClient.cpp
    src/adapters/winrm/WinRmLogFileBrowser.cpp
    src/adapters/winrm/WinRmRemoteExecutor.cpp
    src/adapters/winrm/WinRmRemoteMonitor.cpp
    src/core/DeploymentState.cpp
    src/core/DeploymentRecord.cpp
    src/core/DeployJob.cpp
    src/core/DeployOrchestrator.cpp
    src/core/Version.cpp
    src/infra/ArtifactMatcher.cpp
    src/infra/AppSettingsStore.cpp
    src/infra/AiSettingsStore.cpp
    src/infra/ConfigValidator.cpp
    src/infra/ConfigStore.cpp
    src/infra/CredentialSessionCache.cpp
    src/infra/CredentialStore.cpp
    src/infra/DataPaths.cpp
    src/infra/JdkProfileStore.cpp
    src/infra/LocalLogCatalog.cpp
    src/infra/LogPath.cpp
    src/infra/LogSanitizer.cpp
    src/infra/LogWriter.cpp
    src/infra/ProcessOutputDecoder.cpp
    src/infra/ProjectServiceConfig.cpp
    src/infra/RemoteLogPath.cpp
    src/infra/RemoteOutputCleaner.cpp
    src/infra/ServiceInstanceStore.cpp
    src/infra/ServiceNodeConnection.cpp
    src/infra/ServiceDefaults.cpp
    src/infra/SqlDriverLoader.cpp
    src/adapters/services/JdbcSqlBridge.cpp
    src/tools/CommonTools.cpp
    src/tools/CommonTools.h
    src/adapters/services/RedisServiceClient.cpp
    src/adapters/services/ElasticsearchServiceClient.cpp
    src/adapters/services/SqlServiceClient.cpp
    src/adapters/services/KafkaServiceClient.cpp
    src/adapters/services/KafkaAdminBridge.cpp
    src/adapters/services/ServiceBroker.cpp
    src/adapters/ai/OpenAiChatClient.cpp
    src/tools/OpenAiStreamParser.cpp
)

target_include_directories(deploy_hub_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(deploy_hub_core
    PUBLIC
        Qt6::Core
        Qt6::Sql
        Qt6::Network
)

qt_add_executable(deploy-hub WIN32
    src/app/main.cpp
    src/infra/AppBranding.cpp
    src/ui/AppStyle.cpp
    src/ui/AppStyle.h
    src/ui/PageLayout.cpp
    src/ui/PageLayout.h
    src/ui/Theme.h
    src/ui/DeployWorker.cpp
    src/ui/DeployWorker.h
    src/ui/CommonToolsWidget.cpp
    src/ui/CommonToolsWidget.h
    src/ui/tools/ToolPage.cpp
    src/ui/tools/ToolPage.h
    src/ui/AiAssistHelper.cpp
    src/ui/AiAssistHelper.h
    src/ui/AiChatWidget.cpp
    src/ui/AiChatWidget.h
    src/ui/AiStreamBuffer.cpp
    src/ui/AiStreamBuffer.h
    src/ui/LogAiAnalysisWidget.cpp
    src/ui/LogAiAnalysisWidget.h
    src/ui/HttpRequestWidget.cpp
    src/ui/HttpRequestWidget.h
    src/ui/MiniWebSocket.cpp
    src/ui/MiniWebSocket.h
    src/ui/WebSocketToolWidget.cpp
    src/ui/WebSocketToolWidget.h
    src/ui/DeploymentLogDialog.cpp
    src/ui/DeploymentLogDialog.h
    src/ui/DeploymentLogOpener.cpp
    src/ui/DeploymentLogOpener.h
    src/ui/JdkProfileDialog.cpp
    src/ui/JdkProfileDialog.h
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/qml/AppShellController.cpp
    src/qml/AppShellController.h
    src/qml/LineTabBarController.cpp
    src/qml/LineTabBarController.h
    src/qml/QmlEngineProvider.cpp
    src/qml/QmlEngineProvider.h
    src/qml/QmlPerfConfig.cpp
    src/qml/QmlPerfConfig.h
    src/qml/QmlAppBootstrap.cpp
    src/qml/QmlAppBootstrap.h
    src/qml/UuidToolController.cpp
    src/qml/UuidToolController.h
    src/qml/WidgetHostItem.cpp
    src/qml/WidgetHostItem.h
    src/ui/ProjectDialog.cpp
    src/ui/ProjectDialog.h
    src/ui/ServerDialog.cpp
    src/ui/ServerDialog.h
    src/ui/ProjectManagerWidget.cpp
    src/ui/ProjectManagerWidget.h
    src/ui/ServerManagerWidget.cpp
    src/ui/ServerManagerWidget.h
    src/ui/ServiceHubWidget.cpp
    src/ui/ServiceHubWidget.h
    src/ui/BigDataManagerWidget.cpp
    src/ui/BigDataManagerWidget.h
    src/ui/DatabaseManagerWidget.cpp
    src/ui/DatabaseManagerWidget.h
    src/ui/ServiceNodeTypes.cpp
    src/ui/ServiceNodeTypes.h
    src/ui/ServiceNodeDialog.cpp
    src/ui/ServiceNodeDialog.h
    src/ui/ServiceInstanceDialog.cpp
    src/ui/ServiceInstanceDialog.h
    src/ui/ServiceProductPanel.cpp
    src/ui/ServiceProductPanel.h
    src/ui/ServiceSqlDialog.cpp
    src/ui/ServiceSqlDialog.h
    src/ui/ServiceSqlWorkbenchWidget.cpp
    src/ui/ServiceSqlWorkbenchWidget.h
    src/ui/ServiceContentDialog.cpp
    src/ui/ServiceContentDialog.h
    src/ui/ServiceDatabaseConnectionDialog.cpp
    src/ui/ServiceDatabaseConnectionDialog.h
    src/ui/ServiceRedisConnectionDialog.cpp
    src/ui/ServiceRedisConnectionDialog.h
    src/ui/ServiceElasticsearchQueryDialog.cpp
    src/ui/ServiceElasticsearchQueryDialog.h
    src/ui/ServerMonitorDialog.cpp
    src/ui/ServerMonitorDialog.h
    src/ui/PathPickerWidget.cpp
    src/ui/PathPickerWidget.h
    src/ui/RemoteFileBrowserDialog.cpp
    src/ui/RemoteFileBrowserDialog.h
    src/ui/RemoteFileEditorDialog.cpp
    src/ui/RemoteFileEditorDialog.h
    src/ui/RemoteFileViewerDialog.cpp
    src/ui/RemoteFileViewerDialog.h
    src/ui/RemoteTerminalWidget.cpp
    src/ui/RemoteTerminalWidget.h
    src/ui/TerminalStream.cpp
    src/ui/TerminalStream.h
    src/ui/RemoteCredentialResolver.cpp
    src/ui/RemoteCredentialResolver.h
    src/ui/RemoteUiHelpers.cpp
    src/ui/RemoteUiHelpers.h
)

qt_add_resources(deploy-hub "deploy_hub_app"
    PREFIX "/"
    FILES
        images/icon.svg
        images/icon.png
        images/dashboard-hero.png
        images/dashboard-hero-visual.png
        images/ai/quick.svg
        images/ai/image.svg
        images/ai/presentation.svg
        images/ai/code.svg
        images/ai/video.svg
        images/ai/translate.svg
        images/ai/more.svg
        images/ui/check.svg
        src/ui/style.qss
  )

qt_add_resources(deploy-hub "deploy_hub_qml"
    PREFIX "/"
    BASE src/qml
    FILES
        src/qml/DeployHub/qmldir
        src/qml/DeployHub/Theme.qml
        src/qml/DeployHub/Visual.qml
        src/qml/DeployHub/DhSidebar.qml
        src/qml/DeployHub/DhModuleTabBar.qml
        src/qml/DeployHub/DhLineTabBar.qml
        src/qml/DeployHub/DhCard.qml
        src/qml/DeployHub/DhImage.qml
        src/qml/DeployHub/DhFastBlur.qml
        src/qml/DeployHub/DhButton.qml
        src/qml/DeployHub/DhPageHeader.qml
        src/qml/DeployHub/DhTextArea.qml
        src/qml/DeployHub/DhCheckBox.qml
        src/qml/DeployHub/DhSpinBox.qml
        src/qml/DeployHub/UuidToolPage.qml
        src/qml/DeployHub/MainShell.qml
        src/qml/DeployHub/App.qml
        src/qml/DeployHub/SidebarRoot.qml
        src/qml/DeployHub/ModuleTabRoot.qml
        src/qml/DeployHub/LineTabRoot.qml
)

target_link_libraries(deploy-hub
    PRIVATE
        deploy_hub_core
        Qt6::Widgets
        Qt6::Gui
        Qt6::Svg
        Qt6::Concurrent
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickEffects
        Qt6::Qml
        Qt6::QuickWidgets
)

if(DEPLOY_HUB_USE_FLUENTUI)
    target_link_libraries(deploy-hub PRIVATE
        fluentuiplugin
        Qt6::PrintSupport
    )
    target_compile_definitions(deploy-hub PRIVATE DEPLOY_HUB_USE_FLUENTUI=1)
    qt_add_resources(deploy-hub "deploy_hub_qml_fluent"
        PREFIX "/"
        BASE src/qml
        FILES
            src/qml/DeployHub/AppFluent.qml
    )
    add_custom_command(TARGET deploy-hub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:deploy-hub>/qml/FluentUI"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/FluentUI"
            "$<TARGET_FILE_DIR:deploy-hub>/qml/FluentUI"
        COMMENT "Copy FluentUI QML plugin to output directory"
    )
endif()

find_package(Java QUIET COMPONENTS Runtime Development)
set(JDBC_RUNNER_SRC ${CMAKE_SOURCE_DIR}/tools/jdbc-sql-runner/JdbcSqlRunner.java)
set(JDBC_RUNNER_BUILD_DIR ${CMAKE_BINARY_DIR}/tools/jdbc-sql-runner-classes)
set(JDBC_RUNNER_JAR ${CMAKE_BINARY_DIR}/tools/jdbc-sql-runner.jar)

if(Java_JAVAC_EXECUTABLE AND Java_JAR_EXECUTABLE)
    add_custom_command(
        OUTPUT ${JDBC_RUNNER_JAR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${JDBC_RUNNER_BUILD_DIR}
        COMMAND ${Java_JAVAC_EXECUTABLE} -encoding UTF-8 -d ${JDBC_RUNNER_BUILD_DIR} ${JDBC_RUNNER_SRC}
        COMMAND ${Java_JAR_EXECUTABLE} cfe ${JDBC_RUNNER_JAR} JdbcSqlRunner -C ${JDBC_RUNNER_BUILD_DIR} .
        DEPENDS ${JDBC_RUNNER_SRC}
        COMMENT "Building jdbc-sql-runner.jar"
    )
    add_custom_target(jdbc_sql_runner DEPENDS ${JDBC_RUNNER_JAR})
    add_dependencies(deploy-hub jdbc_sql_runner)
    add_custom_command(TARGET deploy-hub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:deploy-hub>/tools
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${JDBC_RUNNER_JAR} $<TARGET_FILE_DIR:deploy-hub>/tools/jdbc-sql-runner.jar
    )
else()
    message(STATUS "Java JDK not found; build tools/jdbc-sql-runner/jdbc-sql-runner.jar manually")
endif()

set(KAFKA_ADMIN_RUNNER_SRC ${CMAKE_SOURCE_DIR}/tools/kafka-admin-runner/KafkaAdminRunner.java)
set(KAFKA_ADMIN_RUNNER_BUILD_DIR ${CMAKE_BINARY_DIR}/tools/kafka-admin-runner-classes)
set(KAFKA_ADMIN_RUNNER_JAR ${CMAKE_BINARY_DIR}/tools/kafka-admin-runner.jar)

if(Java_JAVAC_EXECUTABLE AND Java_JAR_EXECUTABLE)
    if(DEFINED ENV{KAFKA_HOME})
        file(GLOB KAFKA_ADMIN_CP_JARS "$ENV{KAFKA_HOME}/libs/*.jar")
    endif()
    if(KAFKA_ADMIN_CP_JARS)
        list(JOIN KAFKA_ADMIN_CP_JARS ";" KAFKA_ADMIN_CLASSPATH)
        add_custom_command(
            OUTPUT ${KAFKA_ADMIN_RUNNER_JAR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${KAFKA_ADMIN_RUNNER_BUILD_DIR}
            COMMAND ${Java_JAVAC_EXECUTABLE} -encoding UTF-8 -classpath "${KAFKA_ADMIN_CLASSPATH}" -d ${KAFKA_ADMIN_RUNNER_BUILD_DIR} ${KAFKA_ADMIN_RUNNER_SRC}
            COMMAND ${Java_JAR_EXECUTABLE} cfe ${KAFKA_ADMIN_RUNNER_JAR} KafkaAdminRunner -C ${KAFKA_ADMIN_RUNNER_BUILD_DIR} .
            DEPENDS ${KAFKA_ADMIN_RUNNER_SRC}
            COMMENT "Building kafka-admin-runner.jar"
        )
        add_custom_target(kafka_admin_runner DEPENDS ${KAFKA_ADMIN_RUNNER_JAR})
        add_dependencies(deploy-hub kafka_admin_runner)
        add_custom_command(TARGET deploy-hub POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:deploy-hub>/tools
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${KAFKA_ADMIN_RUNNER_JAR} $<TARGET_FILE_DIR:deploy-hub>/tools/kafka-admin-runner.jar
        )
    else()
        message(STATUS "Set KAFKA_HOME to build kafka-admin-runner.jar; Kafka shell fallback remains available")
    endif()
else()
    message(STATUS "Java JDK not found; build tools/kafka-admin-runner/kafka-admin-runner.jar manually")
endif()

enable_testing()

qt_add_executable(deploy_hub_tests
    tests/unit/main.cpp
    tests/unit/AiAssistHelperTest.cpp
    tests/unit/AiAssistHelperTest.h
    tests/unit/AiSettingsStoreTest.cpp
    tests/unit/AiSettingsStoreTest.h
    tests/unit/AppSettingsStoreTest.cpp
    tests/unit/AppSettingsStoreTest.h
    tests/unit/AppStyleTest.cpp
    tests/unit/AppStyleTest.h
    tests/unit/ArtifactMatcherTest.cpp
    tests/unit/ArtifactMatcherTest.h
    tests/unit/ConfigStoreTest.cpp
    tests/unit/ConfigStoreTest.h
    tests/unit/ConfigValidatorTest.cpp
    tests/unit/ConfigValidatorTest.h
    tests/unit/CommonToolsTest.cpp
    tests/unit/CommonToolsTest.h
    tests/unit/DeployJobTest.cpp
    tests/unit/DeployJobTest.h
    tests/unit/DeployLogPathOptionsTest.cpp
    tests/unit/DeployLogPathOptionsTest.h
    tests/unit/DeployOrchestratorTest.cpp
    tests/unit/DeployOrchestratorTest.h
    tests/unit/DeploymentRecordTest.cpp
    tests/unit/DeploymentRecordTest.h
    tests/unit/ElasticsearchIndexOrganizeTest.cpp
    tests/unit/ElasticsearchIndexOrganizeTest.h
    tests/unit/GitSourceProviderTest.cpp
    tests/unit/GitSourceProviderTest.h
    tests/unit/JdkProfileStoreTest.cpp
    tests/unit/JdkProfileStoreTest.h
    tests/unit/KafkaTopicParseTest.cpp
    tests/unit/KafkaTopicParseTest.h
    tests/unit/LocalLogCatalogTest.cpp
    tests/unit/LocalLogCatalogTest.h
    tests/unit/LogPathTest.cpp
    tests/unit/LogPathTest.h
    tests/unit/LogSanitizerTest.cpp
    tests/unit/LogSanitizerTest.h
    tests/unit/MainWindowStartupTest.cpp
    tests/unit/MainWindowStartupTest.h
    tests/unit/OpenAiStreamParserTest.cpp
    tests/unit/OpenAiStreamParserTest.h
    tests/unit/PageLayoutTest.cpp
    tests/unit/PageLayoutTest.h
    tests/unit/ProcessOutputDecoderTest.cpp
    tests/unit/ProcessOutputDecoderTest.h
    tests/unit/ProjectManagerWidgetTest.cpp
    tests/unit/ProjectManagerWidgetTest.h
    tests/unit/ServiceNodeConnectionTest.cpp
    tests/unit/ServiceNodeConnectionTest.h
    tests/unit/ProjectServiceConfigTest.cpp
    tests/unit/ProjectServiceConfigTest.h
    tests/unit/RemoteOutputCleanerTest.cpp
    tests/unit/RemoteOutputCleanerTest.h
    tests/unit/RemoteFileBrowserTest.cpp
    tests/unit/RemoteFileBrowserTest.h
    tests/unit/RemoteTerminalWidgetTest.cpp
    tests/unit/RemoteTerminalWidgetTest.h
    tests/unit/SshClientTest.cpp
    tests/unit/SshClientTest.h
    tests/unit/VersionTest.cpp
    tests/unit/VersionTest.h
    src/ui/RemoteTerminalWidget.cpp
    src/ui/RemoteTerminalWidget.h
    src/ui/AppStyle.cpp
    src/ui/AppStyle.h
    src/ui/TerminalStream.cpp
    src/ui/TerminalStream.h
    src/tools/CommonTools.h
    src/infra/AppBranding.cpp
    src/infra/AppBranding.h
    src/ui/PageLayout.cpp
    src/ui/PageLayout.h
    src/ui/Theme.h
    src/ui/PathPickerWidget.cpp
    src/ui/PathPickerWidget.h
    src/ui/ProjectDialog.cpp
    src/ui/ProjectDialog.h
    src/ui/ProjectManagerWidget.cpp
    src/ui/ProjectManagerWidget.h
    src/ui/DeploymentLogDialog.cpp
    src/ui/DeploymentLogDialog.h
    src/ui/DeploymentLogOpener.cpp
    src/ui/DeploymentLogOpener.h
    src/ui/AiAssistHelper.cpp
    src/ui/AiAssistHelper.h
    src/ui/AiStreamBuffer.cpp
    src/ui/AiStreamBuffer.h
    src/ui/LogAiAnalysisWidget.cpp
    src/ui/LogAiAnalysisWidget.h
    src/qml/LineTabBarController.cpp
    src/qml/LineTabBarController.h
    src/qml/QmlEngineProvider.cpp
    src/qml/QmlEngineProvider.h
    src/ui/RemoteFileViewerDialog.cpp
    src/ui/RemoteFileViewerDialog.h
    src/ui/RemoteCredentialResolver.cpp
    src/ui/RemoteCredentialResolver.h
    src/ui/RemoteUiHelpers.cpp
    src/ui/RemoteUiHelpers.h
    src/adapters/remote/StubRemoteVfs.cpp
    src/adapters/remote/StubRemoteVfs.h
)

qt_add_resources(deploy_hub_tests "deploy_hub_tests_qml"
    PREFIX "/"
    BASE src/qml
    FILES
        src/qml/DeployHub/qmldir
        src/qml/DeployHub/Theme.qml
        src/qml/DeployHub/DhLineTabBar.qml
        src/qml/DeployHub/LineTabRoot.qml
)

target_link_libraries(deploy_hub_tests
    PRIVATE
        deploy_hub_core
        Qt6::Test
        Qt6::Widgets
        Qt6::Concurrent
        Qt6::Svg
        Qt6::Quick
        Qt6::QuickWidgets
        Qt6::Qml
)

add_test(NAME deploy_hub_tests COMMAND deploy_hub_tests)
