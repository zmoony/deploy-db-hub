#pragma once

#include "ui/ServiceNodeTypes.h"

#include <QDialog>
#include <QJsonObject>

class ConfigStore;
class CredentialSessionCache;
class CredentialStore;
class QComboBox;
class QLineEdit;
class QLabel;
class QPushButton;

class ServiceNodeDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceNodeDialog(QWidget *parent = nullptr);

    void setConfigStore(ConfigStore *store);
    void setCredentials(CredentialStore *credentials, CredentialSessionCache *sessionCache);
    void setContext(const QString &parentInstanceName, ServiceProductKind product);
    void setNode(const QJsonObject &node, bool editMode);
    QJsonObject node() const;

private slots:
    void reloadServers();
    void onTestConnection();
    void onAccept();

private:
    void buildUi();
    void syncInstructionText();
    void applyPathDefaults();

    ConfigStore *m_store = nullptr;
    CredentialStore *m_credentials = nullptr;
    CredentialSessionCache *m_sessionCache = nullptr;
    ServiceProductKind m_product = ServiceProductKind::Kafka;
    bool m_editMode = false;

    QLabel *m_parentLabel = nullptr;
    QComboBox *m_server = nullptr;
    QLineEdit *m_info = nullptr;
    QLineEdit *m_installPath = nullptr;
    QLineEdit *m_storagePath = nullptr;
    QLabel *m_instructions = nullptr;
    QPushButton *m_testConnectionButton = nullptr;
};
