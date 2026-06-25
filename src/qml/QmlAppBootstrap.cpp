#include "qml/QmlAppBootstrap.h"

#include "infra/AppBranding.h"
#include "qml/AppShellController.h"
#include "qml/WidgetHostItem.h"
#include "ui/PageLayout.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlError>
#include <QString>
#include <QQuickWindow>
#include <QScreen>
#include <QUrl>

namespace QmlAppBootstrap {

namespace {

QQmlApplicationEngine *engineInstance()
{
    static QQmlApplicationEngine engine;
    return &engine;
}

void applyWindowGeometry(QQuickWindow *window)
{
    if (window == nullptr) {
        return;
    }
    window->setMinimumSize(QSize(PageLayout::MainWindowMinWidth, PageLayout::MainWindowMinHeight));

    QScreen *screen = window->screen();
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    const QRect available = screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
    const int maxW = qMax(640, static_cast<int>(available.width() * 0.88));
    const int maxH = qMax(460, static_cast<int>(available.height() * 0.88));
    const int width = qBound(PageLayout::MainWindowMinWidth, PageLayout::MainWindowDefaultWidth, maxW);
    const int height = qBound(PageLayout::MainWindowMinHeight, PageLayout::MainWindowDefaultHeight, maxH);
    window->resize(width, height);
    window->setX(available.x() + (available.width() - width) / 2);
    window->setY(available.y() + (available.height() - height) / 2);
}

QString formatQmlErrors(const QList<QQmlError> &errors)
{
    QStringList messages;
    messages.reserve(errors.size());
    for (const QQmlError &error : errors) {
        messages.append(error.toString());
    }
    return messages.isEmpty() ? QStringLiteral("Unknown QML error") : messages.join(QStringLiteral("\n\n"));
}

QUrl applicationQmlUrl()
{
#ifdef DEPLOY_HUB_USE_FLUENTUI
    return QUrl(QStringLiteral("qrc:/DeployHub/AppFluent.qml"));
#else
    return QUrl(QStringLiteral("qrc:/DeployHub/App.qml"));
#endif
}

} // namespace

bool loadApplicationWindow(AppShellController *shell, QString *errorOut)
{
    if (shell == nullptr) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("App shell is not initialized");
        }
        return false;
    }

    qmlRegisterType<WidgetHostItem>("DeployHub", 1, 0, "WidgetHost");

    QQmlApplicationEngine *engine = engineInstance();
    engine->addImportPath(QStringLiteral("qrc:/"));
#ifdef DEPLOY_HUB_USE_FLUENTUI
    const QString appDir = QCoreApplication::applicationDirPath();
    engine->addImportPath(appDir + QStringLiteral("/qml"));
    engine->addImportPath(appDir);
#endif
    engine->rootContext()->setContextProperty(QStringLiteral("AppShell"), shell);

    QQmlComponent component(engine, applicationQmlUrl());
    if (component.isError()) {
        if (errorOut != nullptr) {
            *errorOut = formatQmlErrors(component.errors());
        }
        return false;
    }

    QObject *rootObject = component.create();
    if (rootObject == nullptr) {
        if (errorOut != nullptr) {
            *errorOut = formatQmlErrors(component.errors());
        }
        return false;
    }

    auto *window = qobject_cast<QQuickWindow *>(rootObject);
    if (window == nullptr) {
        if (errorOut != nullptr) {
            *errorOut = QStringLiteral("Root QML object is not a window");
        }
        rootObject->deleteLater();
        return false;
    }

    applyWindowGeometry(window);
    window->setTitle(QStringLiteral("Deploy Hub - 可视化部署工具"));
    window->setIcon(AppBranding::applicationIcon());
    window->show();
    return true;
}

} // namespace QmlAppBootstrap
