#include "ui/RemoteFileViewerDialog.h"

#include "infra/AppBranding.h"
#include "ui/LogAiAnalysisWidget.h"
#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QtConcurrent>

RemoteFileViewerDialog::RemoteFileViewerDialog(RemoteFileBrowser *browser,
                                               const QString &remotePath,
                                               AiSettingsStore *aiSettings,
                                               CredentialStore *credentials,
                                               QWidget *parent)
    : QDialog(parent)
    , m_browser(browser)
    , m_aiSettings(aiSettings)
    , m_credentials(credentials)
    , m_remotePath(remotePath)
{
    setWindowTitle(QStringLiteral("查看远程文件 - %1").arg(remotePath));
    setModal(true);
    PageLayout::applyRemoteToolDialog(this, 760, 460, 1040, 720);
    AppBranding::applyWindowIcon(this);

    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *pathRow = new QVBoxLayout;
    pathRow->setContentsMargins(0, 0, 0, 0);
    pathRow->setSpacing(2);
    auto *pathLabel = new QLabel(remotePath);
    pathLabel->setWordWrap(true);
    pathRow->addWidget(pathLabel);
    layout->addLayout(pathRow);

    auto *controlRow = new QHBoxLayout;
    controlRow->setSpacing(PageLayout::Space12);
    controlRow->addWidget(new QLabel(QStringLiteral("显示最后")));
    m_lineCountSpin = new QSpinBox;
    m_lineCountSpin->setRange(1, 20000);
    m_lineCountSpin->setValue(100);
    m_lineCountSpin->setSuffix(QStringLiteral(" 行"));
    PageLayout::configureFormInput(m_lineCountSpin);
    m_loadButton = new QPushButton(QStringLiteral("加载"));
    m_loadButton->setObjectName(QStringLiteral("primaryButton"));
    m_statusLabel = new QLabel(QStringLiteral("默认显示最后 100 行"));
    m_statusLabel->setObjectName(QStringLiteral("remoteStatusLabel"));
    connect(m_loadButton, &QPushButton::clicked, this, &RemoteFileViewerDialog::loadTail);
    controlRow->addWidget(m_lineCountSpin);
    controlRow->addWidget(m_loadButton);
    controlRow->addWidget(m_statusLabel, 1);
    layout->addLayout(controlRow);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_viewer = new QPlainTextEdit(splitter);
    m_viewer->setObjectName(QStringLiteral("remoteFileViewer"));
    m_viewer->setReadOnly(true);
    m_viewer->setPlainText(QStringLiteral("思考中...."));
    splitter->addWidget(m_viewer);

    if (m_aiSettings != nullptr && m_credentials != nullptr) {
        m_aiPanel = new LogAiAnalysisWidget(m_aiSettings, m_credentials, splitter);
        splitter->addWidget(m_aiPanel);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);
    }

    layout->addWidget(splitter, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    loadTail();
}

RemoteFileViewerDialog::RemoteFileViewerDialog(std::unique_ptr<RemoteFileBrowser> browser,
                                               const QString &remotePath,
                                               AiSettingsStore *aiSettings,
                                               CredentialStore *credentials,
                                               QWidget *parent)
    : RemoteFileViewerDialog(browser.get(), remotePath, aiSettings, credentials, parent)
{
    m_ownedBrowser = std::move(browser);
}

RemoteFileViewerDialog::~RemoteFileViewerDialog()
{
    ++m_generation;
    if (m_tailWatcher != nullptr && m_tailWatcher->isRunning()) {
        m_tailWatcher->waitForFinished();
    }
}

void RemoteFileViewerDialog::loadTail()
{
    if (m_browser == nullptr) {
        return;
    }
    if (m_tailWatcher != nullptr && m_tailWatcher->isRunning()) {
        return;
    }

    const int generation = ++m_generation;
    const QString path = m_remotePath;
    const int lineCount = m_lineCountSpin->value();
    RemoteFileBrowser *browser = m_browser;
    setLoading(true);

    auto *watcher = new QFutureWatcher<RemoteFileReadResult>(this);
    m_tailWatcher = watcher;
    connect(watcher, &QFutureWatcher<RemoteFileReadResult>::finished, this, [this, watcher, generation, lineCount]() {
        if (m_tailWatcher == watcher) {
            m_tailWatcher = nullptr;
        }
        if (generation != m_generation) {
            watcher->deleteLater();
            return;
        }

        const RemoteFileReadResult result = watcher->result();
        if (!result.ok) {
            m_viewer->setPlainText(result.error);
            m_statusLabel->setText(QStringLiteral("加载失败"));
            if (m_aiPanel != nullptr) {
                m_aiPanel->setLogContent(result.error);
            }
        } else {
            m_viewer->setPlainText(result.content);
            m_statusLabel->setText(QStringLiteral("已显示最后 %1 行").arg(lineCount));
            if (m_aiPanel != nullptr) {
                m_aiPanel->setLogContent(result.content);
            }
        }
        setLoading(false);
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([browser, path, lineCount]() {
        return browser->readFileTail(path, lineCount);
    }));
}

void RemoteFileViewerDialog::setLoading(bool loading)
{
    m_loadButton->setEnabled(!loading);
    m_lineCountSpin->setEnabled(!loading);
    if (loading) {
        m_statusLabel->setText(QStringLiteral("思考中...."));
        m_viewer->setPlainText(QStringLiteral("思考中...."));
    }
}
