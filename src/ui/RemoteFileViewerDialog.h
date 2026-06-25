#pragma once

#include "adapters/remote/RemoteFileBrowser.h"

#include <memory>

#include <QDialog>
#include <QFutureWatcher>

class AiSettingsStore;
class CredentialStore;
class LogAiAnalysisWidget;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

class RemoteFileViewerDialog final : public QDialog
{
    Q_OBJECT

public:
    RemoteFileViewerDialog(RemoteFileBrowser *browser,
                           const QString &remotePath,
                           AiSettingsStore *aiSettings,
                           CredentialStore *credentials,
                           QWidget *parent = nullptr);
    RemoteFileViewerDialog(std::unique_ptr<RemoteFileBrowser> browser,
                           const QString &remotePath,
                           AiSettingsStore *aiSettings,
                           CredentialStore *credentials,
                           QWidget *parent = nullptr);
    ~RemoteFileViewerDialog() override;

private slots:
    void loadTail();

private:
    void setLoading(bool loading);

    std::unique_ptr<RemoteFileBrowser> m_ownedBrowser;
    RemoteFileBrowser *m_browser = nullptr;
    AiSettingsStore *m_aiSettings = nullptr;
    CredentialStore *m_credentials = nullptr;
    QString m_remotePath;
    int m_generation = 0;
    QFutureWatcher<RemoteFileReadResult> *m_tailWatcher = nullptr;

    QLabel *m_statusLabel = nullptr;
    QSpinBox *m_lineCountSpin = nullptr;
    QPushButton *m_loadButton = nullptr;
    QPlainTextEdit *m_viewer = nullptr;
    LogAiAnalysisWidget *m_aiPanel = nullptr;
};
