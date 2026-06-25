#include "ui/DeploymentLogDialog.h"

#include "infra/AppBranding.h"
#include "infra/LocalLogCatalog.h"
#include "ui/LogAiAnalysisWidget.h"
#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

bool DeploymentLogDialog::canOpen(const QString &relativePath)
{
    return LocalLogCatalog::canOpen(relativePath);
}

QString DeploymentLogDialog::loadContent(const QString &relativePath, QString *error)
{
    return LocalLogCatalog::loadContent(relativePath, error);
}

DeploymentLogDialog::DeploymentLogDialog(const QString &relativePath,
                                         AiSettingsStore *aiSettings,
                                         CredentialStore *credentials,
                                         QWidget *parent)
    : QDialog(parent)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
{
    setWindowTitle(QStringLiteral("部署日志"));
    setModal(true);
    PageLayout::applyModalDialog(this);
    AppBranding::applyWindowIcon(this);

    QString error;
    const QString content = loadContent(relativePath, &error);
    buildUi(relativePath, content.isEmpty() ? error : content);
}

void DeploymentLogDialog::buildUi(const QString &relativePath, const QString &content)
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    auto *editor = new QPlainTextEdit(splitter);
    editor->setReadOnly(true);
    editor->setPlainText(content);
    editor->setMinimumHeight(220);
    splitter->addWidget(editor);

    if (m_aiSettings != nullptr && m_credentials != nullptr) {
        auto *aiPanel = new LogAiAnalysisWidget(m_aiSettings, m_credentials, splitter);
        aiPanel->setLogContent(content);
        splitter->addWidget(aiPanel);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);
    }

    layout->addWidget(splitter, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(buttons);

    setWindowTitle(QStringLiteral("部署日志 - %1").arg(relativePath));
}
