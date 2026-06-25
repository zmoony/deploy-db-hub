#pragma once

#include <QDialog>

class AiSettingsStore;
class CredentialStore;

class DeploymentLogDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit DeploymentLogDialog(const QString &relativePath,
                                 AiSettingsStore *aiSettings,
                                 CredentialStore *credentials,
                                 QWidget *parent = nullptr);

    static bool canOpen(const QString &relativePath);
    static QString loadContent(const QString &relativePath, QString *error);

private:
    void buildUi(const QString &relativePath, const QString &content);

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
};
