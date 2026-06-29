#pragma once

#include "ui/tools/ToolPage.h"

class AiSettingsStore;
class CredentialStore;

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

private:
    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
};

} // namespace Tools
} // namespace Ui
