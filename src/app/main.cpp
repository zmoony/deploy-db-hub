#include "ui/MainWindow.h"
#include "ui/AppStyle.h"

#include "infra/AppBranding.h"
#include "infra/DataPaths.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    DataPaths::initialize();
    AppStyle::apply(app);
    app.setWindowIcon(AppBranding::applicationIcon());

    MainWindow window;
    AppBranding::applyWindowIcon(&window);
    window.show();

    return app.exec();
}
