#pragma once

#include <QStringList>
#include <QWidget>

#include <functional>

#include "infra/AiSettingsStore.h"
class CredentialStore;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class CommonToolsWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit CommonToolsWidget(AiSettingsStore *aiSettings,
                               CredentialStore *credentials,
                               QWidget *parent = nullptr);
    QStringList toolLabels() const;
    QWidget *takeToolPage(int index);
    int toolPageCount() const;

    void wireAiAssist(QWidget *page,
                      QPushButton *assistButton,
                      QPushButton *stopButton,
                      QPlainTextEdit *output,
                      QLabel *message,
                      const std::function<QString()> &userContentProvider,
                      const QString &systemPrompt,
                      const std::function<void()> &onStart = {});

private:
    bool resolveAiCredentials(AiSettings *settings, QString *apiKey, QString *error) const;

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    QStackedWidget *m_stack = nullptr;
};
