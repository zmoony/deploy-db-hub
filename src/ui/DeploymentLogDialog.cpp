#include "ui/DeploymentLogDialog.h"

#include "infra/AppBranding.h"
#include "infra/LocalLogCatalog.h"
#include "ui/LogAiAnalysisWidget.h"
#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
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
    resize(1040, 720);
    setMinimumSize(900, 620);
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

    auto *logPanel = new QFrame(splitter);
    logPanel->setObjectName(QStringLiteral("contentPanel"));
    auto *logPanelLayout = new QVBoxLayout(logPanel);
    logPanelLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    logPanelLayout->setSpacing(PageLayout::Space8);

    auto *logHeader = new QHBoxLayout;
    logHeader->setContentsMargins(0, 0, 0, 0);
    auto *logTitle = new QLabel(QStringLiteral("日志内容"), logPanel);
    logTitle->setObjectName(QStringLiteral("sectionLabel"));
    logHeader->addWidget(logTitle);
    logHeader->addStretch();
    logPanelLayout->addLayout(logHeader);

    auto *editor = new QPlainTextEdit(logPanel);
    editor->setReadOnly(true);
    editor->setPlainText(content);
    editor->setMinimumHeight(180);
    editor->setFrameShape(QFrame::NoFrame);
    logPanelLayout->addWidget(editor, 1);
    splitter->addWidget(logPanel);

    if (m_aiSettings != nullptr && m_credentials != nullptr) {
        auto *aiPanel = new QFrame(splitter);
        aiPanel->setObjectName(QStringLiteral("contentPanel"));
        auto *aiPanelLayout = new QVBoxLayout(aiPanel);
        aiPanelLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
        aiPanelLayout->setSpacing(PageLayout::Space8);

        auto *aiWidget = new LogAiAnalysisWidget(m_aiSettings, m_credentials, aiPanel);
        aiWidget->setLogContent(content);
        aiWidget->setMinimumHeight(260);
        aiPanelLayout->addWidget(aiWidget, 1);
        aiPanel->setMinimumHeight(300);
        splitter->addWidget(aiPanel);
        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);
        splitter->setSizes({320, 320});
    }

    layout->addWidget(splitter, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(buttons);

    setWindowTitle(QStringLiteral("部署日志 - %1").arg(relativePath));
}
