#pragma once

#include "infra/AiSettingsStore.h"

#include <QDialog>
#include <QString>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class OpenAiChatClient;
class CredentialStore;

class AiConnectionDialog final : public QDialog
{
    Q_OBJECT

public:
    enum class Mode {
        Create,
        Edit
    };

    explicit AiConnectionDialog(CredentialStore *credentials,
                                Mode mode,
                                AiConnection seed,
                                const QVector<AiConnection> &existing,
                                QWidget *parent = nullptr);

    AiConnection connection() const { return m_draft; }
    QString secret() const { return m_secret; }
    bool hasSecret() const { return m_hasSecret; }
    bool clearStoredSecret() const { return m_clearStoredSecret; }

private slots:
    void onTestConnection();
    void onAccept();

private:
    void buildUi();
    void applyToWidgets();
    void syncFromWidgets();

    CredentialStore *m_credentials = nullptr;
    Mode m_mode;
    AiConnection m_draft;
    QVector<AiConnection> m_existing;
    QString m_secret;
    bool m_hasSecret = false;
    bool m_clearStoredSecret = false;

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_baseUrlEdit = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QLineEdit *m_apiKeyEdit = nullptr;
    QPushButton *m_visibilityButton = nullptr;
    QCheckBox *m_rememberKeyCheck = nullptr;
    QLabel *m_message = nullptr;
    QPushButton *m_testButton = nullptr;
    OpenAiChatClient *m_testClient = nullptr;
    int m_testGeneration = 0;
};
