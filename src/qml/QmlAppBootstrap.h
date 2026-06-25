#pragma once

class AppShellController;
class QString;

namespace QmlAppBootstrap {

bool loadApplicationWindow(AppShellController *shell, QString *errorOut = nullptr);

}
