#include "ui/MainWindow.h"
#include "ui/AppStyle.h"
#include "ui/Theme.h"

#include "infra/AppBranding.h"
#include "infra/AppSettingsStore.h"
#include "infra/DataPaths.h"

#include <QApplication>

static ThemeMode loadThemeMode()
{
    AppSettingsStore store(AppSettingsStore::defaultSettingsFile());
    AppSettings settings;
    QString error;
    if (store.load(&settings, &error)) {
        const QString &tm = settings.themeMode;
        if (tm == QStringLiteral("dark")) {
            return ThemeMode::Dark;
        }
        if (tm == QStringLiteral("light")) {
            return ThemeMode::Light;
        }
    }
    return ThemeMode::System;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DataPaths::initialize();
    AppStyle::apply(app, loadThemeMode());
    app.setWindowIcon(AppBranding::applicationIcon());

    MainWindow window;
    AppBranding::applyWindowIcon(&window);
    window.show();

    return app.exec();
}
