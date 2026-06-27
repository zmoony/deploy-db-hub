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

private:
    QWidget *buildTextToolPage(const QString &title,
                               const QString &subtitle,
                               const QString &primaryAction,
                               const QString &secondaryAction,
                               const QString &placeholder,
                               bool enableAiAssist = false,
                               const QString &aiSystemPrompt = QString());
    QWidget *buildHttpRequestPage();
    QWidget *buildJsonViewerPage();
    QWidget *buildRegexPage();
    QWidget *buildTimestampPage();
    QWidget *buildHtmlEntityPage();
    QWidget *buildHttpStatusPage();
    QWidget *buildDiffPage();
    QWidget *buildCronPage();
    QWidget *buildImageBase64Page();
    QWidget *buildWebSocketPage();
    QWidget *buildUuidPage();
    QWidget *buildHashPage();
    QWidget *buildUrlCodecPage();
    QWidget *buildBase64TextPage();
    QWidget *buildJwtPage();
    QWidget *buildNumberBasePage();
    QWidget *buildCasePage();
    void setOutput(QPlainTextEdit *output, QLabel *message, const QString &text, const QString &error);
    bool resolveAiCredentials(AiSettings *settings, QString *apiKey, QString *error) const;
    void wireAiAssist(QWidget *page,
                      QPushButton *assistButton,
                      QPushButton *stopButton,
                      QPlainTextEdit *output,
                      QLabel *message,
                      const std::function<QString()> &userContentProvider,
                      const QString &systemPrompt,
                      const std::function<void()> &onStart = {});

    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    QStackedWidget *m_stack = nullptr;
};
