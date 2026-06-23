#pragma once

#include <QObject>

class MainWindowStartupTest final : public QObject
{
    Q_OBJECT

private slots:
    void startupRefreshesDoNotPromptForRemoteAccess();
};
