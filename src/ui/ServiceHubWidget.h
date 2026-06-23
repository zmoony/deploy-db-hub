#pragma once

#include <QWidget>

class BigDataManagerWidget;
class ConfigStore;
class CredentialSessionCache;
class CredentialStore;
class DatabaseManagerWidget;
class ServerManagerWidget;

class ServiceHubWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceHubWidget(ConfigStore *store,
                              CredentialStore *credentials,
                              CredentialSessionCache *sessionCache,
                              QWidget *parent = nullptr);

    ServerManagerWidget *serverManager() const;
    int serverCount() const;
    QStringList serverIds() const;

signals:
    void serversChanged();

private:
    ServerManagerWidget *m_serverManager = nullptr;
    BigDataManagerWidget *m_bigDataManager = nullptr;
    DatabaseManagerWidget *m_databaseManager = nullptr;
};
