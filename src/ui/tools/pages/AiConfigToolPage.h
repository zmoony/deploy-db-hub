#pragma once

#include "ui/tools/ToolPage.h"

#include <QString>

class AiSettingsStore;
class CredentialStore;
class QComboBox;
class QLineEdit;
class QLabel;
class QCheckBox;

namespace Ui {
namespace Tools {

class AiConfigToolPage : public ToolPage {
    Q_OBJECT
public:
    explicit AiConfigToolPage(AiSettingsStore *aiSettings,
                              CredentialStore *credentials,
                              QWidget *parent = nullptr);

    QString title() const override;
    QString subtitle() const override;

private slots:
    void openConnectionManager();

private:
    void buildUi();
    void loadForm();
    int findConnectionIndex(const QString &id) const;

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;

    QComboBox *m_connectionCombo = nullptr;
    QLineEdit *m_apiBaseUrlInput = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QLineEdit *m_apiKeyInput = nullptr;
    QCheckBox *m_rememberKeyCheck = nullptr;
    QLabel *m_message = nullptr;
};

} // namespace Tools
} // namespace Ui
