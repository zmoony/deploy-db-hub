#pragma once

#include "infra/CredentialSessionCache.h"

#include <QWidget>

#include <memory>

class ConfigStore;
class CredentialStore;
class QLabel;
class QTableWidget;
class ServerDialog;

class ServerManagerWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ServerManagerWidget(ConfigStore *store, QWidget *parent = nullptr, bool showPageHeader = true);

    int serverCount() const;
    QStringList serverIds() const;

signals:
    void serversChanged();

private slots:
    void reload();
    void addServer();
    void editServer();
    void deleteServer();
    void browseRemoteFiles();
    void openServerMonitor();

private:
    QString selectedServerId() const;
    void setupTable();
    void persistCredentials(const QJsonObject &server, const ServerDialog &dialog);
    QString credentialRefFor(const QJsonObject &server) const;

    ConfigStore *m_store = nullptr;
    CredentialStore *m_credentials = nullptr;
    std::unique_ptr<CredentialSessionCache> m_sessionCache;
    QTableWidget *m_table = nullptr;
    QLabel *m_emptyState = nullptr;
};
