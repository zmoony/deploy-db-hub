#include "ui/ServerDialog.h"

#include "adapters/remote/RemoteExecutor.h"
#include "adapters/ssh/SshClient.h"
#include "infra/ConfigValidator.h"
#include "infra/CredentialStore.h"
#include "ui/PageLayout.h"
#include "ui/RemoteCredentialResolver.h"
#include "ui/RemoteUiHelpers.h"
#include "ui/PathPickerWidget.h"
#include "ui/RemoteFileBrowserDialog.h"
#include "infra/AppBranding.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QSignalBlocker>
#include <QtConcurrent>

namespace {

QString defaultUsernameForOs(const QString &os)
{
    return os == QStringLiteral("windows")
        ? QStringLiteral("administrator")
        : QStringLiteral("root");
}

QString defaultRemoteBaseDirForOs(const QString &os)
{
    return os == QStringLiteral("windows")
        ? QStringLiteral("D:/psmp")
        : QStringLiteral("/home");
}

QJsonObject defaultRemoteFilesConfig(const QString &defaultBrowsePath)
{
    return QJsonObject{
        {QStringLiteral("enabled"), true},
        {QStringLiteral("defaultBrowsePath"), defaultBrowsePath},
        {QStringLiteral("uploadTempDir"), QStringLiteral("/tmp/deploy-hub-uploads")},
        {QStringLiteral("maxUploadSizeMb"), 500},
        {QStringLiteral("operations"), QJsonObject{
            {QStringLiteral("listDir"), true},
            {QStringLiteral("read"), true},
            {QStringLiteral("write"), true},
            {QStringLiteral("mkdir"), true},
            {QStringLiteral("upload"), true},
            {QStringLiteral("delete"), true},
            {QStringLiteral("rename"), true}
        }}
    };
}

void tuneFormBox(QGroupBox *box, QFormLayout *form)
{
    PageLayout::tuneDialogFormBox(box, form);
}

QJsonObject defaultMonitoringConfig()
{
    return QJsonObject{
        {QStringLiteral("enabled"), true},
        {QStringLiteral("processListLimit"), 20},
        {QStringLiteral("allowKillProcess"), true}
    };
}

RemoteCommandResult runConnectionTest(const RemoteConnectionContext &context,
                                      const HostKeyPromptHandler &hostKeyPrompt)
{
    const QString os = context.serverConfig.value(QStringLiteral("os")).toString();
    if (os == QStringLiteral("linux")) {
        SshClient client(context, hostKeyPrompt);
        return client.execute(QStringLiteral("echo deploy-hub-ping"), 10);
    }

    auto executor = createRemoteExecutor(context);
    if (!executor) {
        return RemoteCommandResult{false, -1, {}, {}, QStringLiteral("不支持的远程协议")};
    }
    return executor->testConnection();
}

}

ServerDialog::ServerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("服务器配置"));
    PageLayout::applyRemoteToolDialog(this,
                                      PageLayout::DialogMinWidth,
                                      640,
                                      PageLayout::DialogDefaultWidth,
                                      680);
    AppBranding::applyWindowIcon(this);
    buildUi();
}

void ServerDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *content = PageLayout::wrapScrollableBody(layout);
    auto *bodyLayout = new QVBoxLayout(content);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space16);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(PageLayout::Space24);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(PageLayout::Space16);
    auto *rightColumn = new QVBoxLayout;
    rightColumn->setSpacing(PageLayout::Space16);

    auto *basicBox = new QGroupBox(QStringLiteral("基本信息"));
    auto *basicForm = new QFormLayout(basicBox);
    tuneFormBox(basicBox, basicForm);
    m_id = new QLineEdit;
    m_id->setPlaceholderText(QStringLiteral("例如 prod-linux-1"));
    m_name = new QLineEdit;
    m_os = new QComboBox;
    m_os->addItem(QStringLiteral("Linux (SSH)"), QStringLiteral("linux"));
    m_os->addItem(QStringLiteral("Windows (WinRM)"), QStringLiteral("windows"));
    m_host = new QLineEdit;
    m_host->setPlaceholderText(QStringLiteral("192.168.1.10"));
    m_port = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(22);
    m_username = new QLineEdit;
    m_username->setPlaceholderText(QStringLiteral("root"));
    PageLayout::configureFormInput(m_id);
    PageLayout::configureFormInput(m_name);
    PageLayout::configureFormInput(m_os);
    PageLayout::configureFormInput(m_host);
    PageLayout::configureFormInput(m_port);
    PageLayout::configureFormInput(m_username);
    basicForm->addRow(QStringLiteral("服务器 ID"), m_id);
    basicForm->addRow(QStringLiteral("名称"), m_name);
    basicForm->addRow(QStringLiteral("系统"), m_os);
    basicForm->addRow(QStringLiteral("主机"), m_host);
    basicForm->addRow(QStringLiteral("端口"), m_port);
    basicForm->addRow(QStringLiteral("用户名"), m_username);
    leftColumn->addWidget(basicBox);

    auto *authBox = new QGroupBox(QStringLiteral("认证"));
    auto *authForm = new QFormLayout(authBox);
    tuneFormBox(authBox, authForm);
    m_authMode = new QComboBox;
    m_authMode->addItem(QStringLiteral("密码"), QStringLiteral("password"));
    m_authMode->addItem(QStringLiteral("SSH 私钥 (Linux)"), QStringLiteral("ssh-key"));
    m_authMode->addItem(QStringLiteral("每次部署时输入"), QStringLiteral("manual"));
    PageLayout::configureFormInput(m_authMode);
    authForm->addRow(QStringLiteral("模式"), m_authMode);

    auto *authStack = new QStackedWidget;
    auto *passwordPanel = new QWidget;
    auto *passwordForm = new QFormLayout(passwordPanel);
    PageLayout::applyDialogForm(passwordForm);
    passwordForm->setContentsMargins(0, 0, 0, 0);
    auto *passwordRow = new QWidget;
    PageLayout::configureHorizontalFormRow(passwordRow);
    auto *passwordRowLayout = new QHBoxLayout(passwordRow);
    passwordRowLayout->setContentsMargins(0, 0, 0, 0);
    passwordRowLayout->setSpacing(PageLayout::Space8);
    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(QStringLiteral("输入登录密码"));
    PageLayout::configureFormInput(m_password);
    m_passwordVisibilityButton = new QPushButton(QStringLiteral("显示"), passwordRow);
    m_passwordVisibilityButton->setCheckable(true);
    m_passwordVisibilityButton->setMinimumWidth(56);
    m_passwordVisibilityButton->setFixedHeight(PageLayout::DialogFieldHeight);
    m_passwordVisibilityButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_passwordVisibilityButton, &QPushButton::toggled, this, [this](bool visible) {
        m_password->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        m_passwordVisibilityButton->setText(visible ? QStringLiteral("隐藏") : QStringLiteral("显示"));
    });
    passwordRowLayout->addWidget(m_password, 1);
    passwordRowLayout->addWidget(m_passwordVisibilityButton);
    m_rememberPassword = new QCheckBox(QStringLiteral("记住密码"));
    m_rememberPassword->setChecked(true);
    passwordForm->addRow(QStringLiteral("密码"), passwordRow);
    auto *rememberRow = new QWidget;
    PageLayout::configureHorizontalFormRow(rememberRow);
    auto *rememberLayout = new QHBoxLayout(rememberRow);
    rememberLayout->setContentsMargins(0, 0, 0, 0);
    rememberLayout->addWidget(m_rememberPassword);
    rememberLayout->addStretch();
    m_testConnectionButton = new QPushButton(QStringLiteral("测试连接"));
    m_testConnectionButton->setObjectName(QStringLiteral("primaryButton"));
    rememberLayout->addWidget(m_testConnectionButton);
    passwordForm->addRow(QString(), rememberRow);
    connect(m_testConnectionButton, &QPushButton::clicked, this, &ServerDialog::onTestConnection);

    auto *manualPanel = new QLabel(QStringLiteral("部署时将提示输入密码，不会保存到本机。"));
    manualPanel->setWordWrap(true);

    authStack->addWidget(passwordPanel);
    authStack->addWidget(manualPanel);
    authStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    authForm->addRow(QStringLiteral("凭据"), authStack);
    m_authStack = authStack;
    connect(m_authMode, qOverload<int>(&QComboBox::currentIndexChanged), this, &ServerDialog::onAuthModeChanged);
    rightColumn->addWidget(authBox);

    auto *remoteBox = new QGroupBox(QStringLiteral("远程协议"));
    auto *remoteForm = new QFormLayout(remoteBox);
    tuneFormBox(remoteBox, remoteForm);
    m_osStack = new QStackedWidget;
    auto *linuxPanel = new QLabel(QStringLiteral("SSH Host Key 策略：strict-with-prompt（首次连接需确认指纹）"));
    linuxPanel->setWordWrap(true);
    auto *winPanel = new QWidget;
    auto *winForm = new QFormLayout(winPanel);
    PageLayout::applyInlineForm(winForm);
    winForm->setContentsMargins(0, 0, 0, 0);
    m_winrmScheme = new QComboBox;
    m_winrmScheme->addItem(QStringLiteral("HTTPS (5986)"), QStringLiteral("https"));
    m_winrmScheme->addItem(QStringLiteral("HTTP (5985)"), QStringLiteral("http"));
    PageLayout::configureFormInput(m_winrmScheme);
    winForm->addRow(QStringLiteral("WinRM 协议"), m_winrmScheme);
    m_osStack->addWidget(linuxPanel);
    m_osStack->addWidget(winPanel);
    remoteForm->addRow(QStringLiteral("协议选项"), m_osStack);
    connect(m_os, qOverload<int>(&QComboBox::currentIndexChanged), this, &ServerDialog::onOsChanged);
    rightColumn->addWidget(remoteBox);

    columns->addLayout(leftColumn, 1);
    columns->addLayout(rightColumn, 1);
    bodyLayout->addLayout(columns);

    auto *pathsBox = new QGroupBox(QStringLiteral("路径与目录"));
    auto *pathsForm = new QFormLayout(pathsBox);
    tuneFormBox(pathsBox, pathsForm);
    m_defaultRemoteBaseDir = new PathPickerWidget(PathPickerWidget::Mode::Directory);
    m_defaultRemoteBaseDir->setPlaceholderText(QStringLiteral("/home"));
    m_defaultRemoteBaseDir->setBrowseHandler([this]() { browseRemoteBaseDir(); });
    pathsForm->addRow(QStringLiteral("默认部署根目录"), m_defaultRemoteBaseDir);

    m_sshKeyRow = new QWidget;
    PageLayout::configureHorizontalFormRow(m_sshKeyRow);
    auto *sshKeyLayout = new QVBoxLayout(m_sshKeyRow);
    sshKeyLayout->setContentsMargins(0, 0, 0, 0);
    m_sshKeyPath = new PathPickerWidget(PathPickerWidget::Mode::File);
    m_sshKeyPath->setPlaceholderText(QStringLiteral("~/.ssh/id_ed25519"));
    sshKeyLayout->addWidget(m_sshKeyPath);
    auto *sshKeyLabel = new QLabel(QStringLiteral("SSH 私钥路径"));
    pathsForm->addRow(sshKeyLabel, m_sshKeyRow);
    m_sshKeyLabel = sshKeyLabel;
    bodyLayout->addWidget(pathsBox);

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);
    footerLayout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, footer);
    buttons->setCenterButtons(false);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("secondaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ServerDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);

    syncAuthFields();
}

void ServerDialog::setCredentialStore(CredentialStore *store)
{
    m_credentialStore = store;
}

void ServerDialog::setServer(const QJsonObject &server, bool editMode, bool hasStoredPassword)
{
    m_editMode = editMode;
    m_hasStoredPassword = hasStoredPassword;
    m_sourceServer = server;
    m_id->setText(server.value(QStringLiteral("id")).toString());
    m_id->setReadOnly(editMode);
    m_name->setText(server.value(QStringLiteral("name")).toString());

    const QString os = server.value(QStringLiteral("os")).toString();
    m_os->setCurrentIndex(os == QStringLiteral("windows") ? 1 : 0);
    m_host->setText(server.value(QStringLiteral("host")).toString());
    m_port->setValue(server.value(QStringLiteral("port")).toInt(os == QStringLiteral("windows") ? 5986 : 22));
    m_username->setText(server.value(QStringLiteral("username")).toString());
    m_defaultRemoteBaseDir->setPath(server.value(QStringLiteral("defaultRemoteBaseDir")).toString());

    const QJsonObject auth = server.value(QStringLiteral("auth")).toObject();
    const QString mode = auth.value(QStringLiteral("mode")).toString();
    if (mode == QStringLiteral("ssh-key")) {
        m_authMode->setCurrentIndex(1);
        m_sshKeyPath->setPath(auth.value(QStringLiteral("sshPrivateKeyPath")).toString());
    } else if (mode == QStringLiteral("manual")) {
        m_authMode->setCurrentIndex(2);
        m_rememberPassword->setChecked(false);
    } else {
        m_authMode->setCurrentIndex(0);
        m_rememberPassword->setChecked(mode == QStringLiteral("system-keychain") || hasStoredPassword);
    }

    m_password->clear();
    m_password->setEchoMode(QLineEdit::Password);
    if (m_passwordVisibilityButton != nullptr) {
        const QSignalBlocker blocker(m_passwordVisibilityButton);
        m_passwordVisibilityButton->setChecked(false);
        m_passwordVisibilityButton->setText(QStringLiteral("显示"));
    }
    if (hasStoredPassword) {
        m_password->setPlaceholderText(QStringLiteral("已保存密码，留空表示不修改"));
    } else {
        m_password->setPlaceholderText(QStringLiteral("输入登录密码"));
    }

    const QJsonObject winrm = server.value(QStringLiteral("winrm")).toObject();
    const QString scheme = winrm.value(QStringLiteral("scheme")).toString(QStringLiteral("https"));
    m_winrmScheme->setCurrentIndex(scheme == QStringLiteral("http") ? 1 : 0);

    syncOsFields();
    syncAuthFields();
}

QJsonObject ServerDialog::server() const
{
    const QString os = m_os->currentData().toString();
    const QString authUiMode = m_authMode->currentData().toString();
    const QString serverId = m_id->text().trimmed();
    const QString defaultRemoteBaseDir = m_defaultRemoteBaseDir->path();

    QString authMode;
    QJsonValue credentialRef = QJsonValue::Null;
    QJsonValue sshPrivateKeyPath = QJsonValue::Null;

    if (authUiMode == QStringLiteral("ssh-key")) {
        authMode = QStringLiteral("ssh-key");
        sshPrivateKeyPath = m_sshKeyPath->path().isEmpty()
            ? QStringLiteral("~/.ssh/id_ed25519")
            : m_sshKeyPath->path();
    } else if (authUiMode == QStringLiteral("manual") || !m_rememberPassword->isChecked()) {
        authMode = QStringLiteral("manual");
    } else {
        authMode = QStringLiteral("system-keychain");
        credentialRef = QStringLiteral("deploy-hub/") + serverId;
    }

    QJsonObject result{
        {QStringLiteral("schemaVersion"), 1},
        {QStringLiteral("id"), serverId},
        {QStringLiteral("name"), m_name->text().trimmed()},
        {QStringLiteral("os"), os},
        {QStringLiteral("host"), m_host->text().trimmed()},
        {QStringLiteral("port"), m_port->value()},
        {QStringLiteral("username"), m_username->text().trimmed()},
        {QStringLiteral("auth"), QJsonObject{
            {QStringLiteral("mode"), authMode},
            {QStringLiteral("credentialRef"), credentialRef},
            {QStringLiteral("sshPrivateKeyPath"), sshPrivateKeyPath}
        }},
        {QStringLiteral("defaultRemoteBaseDir"), defaultRemoteBaseDir}
    };

    if (os == QStringLiteral("linux")) {
        result[QStringLiteral("ssh")] = QJsonObject{{QStringLiteral("knownHostsPolicy"), QStringLiteral("strict-with-prompt")}};
        result[QStringLiteral("winrm")] = QJsonValue::Null;
        const QJsonObject existingRemoteFiles = m_sourceServer.value(QStringLiteral("remoteFiles")).toObject();
        result[QStringLiteral("remoteFiles")] = existingRemoteFiles.isEmpty()
            ? defaultRemoteFilesConfig(defaultRemoteBaseDir.isEmpty() ? QStringLiteral("/") : defaultRemoteBaseDir)
            : existingRemoteFiles;
    } else {
        result[QStringLiteral("ssh")] = QJsonValue::Null;
        result[QStringLiteral("winrm")] = QJsonObject{
            {QStringLiteral("scheme"), m_winrmScheme->currentData().toString()},
            {QStringLiteral("tlsVerify"), true},
            {QStringLiteral("trustedCertFingerprint"), QJsonValue::Null}
        };
        result[QStringLiteral("remoteFiles")] = QJsonValue::Null;
    }

    const QJsonObject existingMonitoring = m_sourceServer.value(QStringLiteral("monitoring")).toObject();
    result[QStringLiteral("monitoring")] = existingMonitoring.isEmpty()
        ? defaultMonitoringConfig()
        : existingMonitoring;

    return result;
}

QString ServerDialog::pendingPassword() const
{
    return m_password->text();
}

bool ServerDialog::shouldRememberPassword() const
{
    return m_authMode->currentData().toString() == QStringLiteral("password")
        && m_rememberPassword->isChecked();
}

bool ServerDialog::shouldClearStoredPassword() const
{
    return m_authMode->currentData().toString() != QStringLiteral("password")
        || !m_rememberPassword->isChecked();
}

QJsonObject ServerDialog::draftServerConfig() const
{
    QJsonObject draft = server();
    if (!m_sourceServer.contains(QStringLiteral("remoteFiles"))) {
        const QString baseDir = m_defaultRemoteBaseDir->path();
        draft[QStringLiteral("remoteFiles")] = defaultRemoteFilesConfig(
            baseDir.isEmpty() ? QStringLiteral("/") : baseDir);
    }
    return draft;
}

bool ServerDialog::buildConnectionContext(RemoteConnectionContext *context, QString *error) const
{
    if (context == nullptr) {
        return false;
    }

    if (m_host->text().trimmed().isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请先填写主机地址。");
        }
        return false;
    }
    if (m_username->text().trimmed().isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请先填写用户名。");
        }
        return false;
    }

    context->serverConfig = draftServerConfig();
    if (!m_password->text().isEmpty()) {
        context->password = m_password->text();
    } else {
        const RemoteConnectionContext resolved =
            RemoteCredentialResolver::resolve(context->serverConfig, m_credentialStore, nullptr, nullptr, false);
        context->password = resolved.password;
    }

    const QString authUiMode = m_authMode->currentData().toString();
    if (authUiMode == QStringLiteral("password") && context->password.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("请先输入服务器密码。");
        }
        return false;
    }

    return true;
}

void ServerDialog::browseRemoteBaseDir()
{
    if (m_os->currentData().toString() != QStringLiteral("linux")) {
        QMessageBox::information(this,
                               QStringLiteral("暂不支持"),
                               QStringLiteral("远程目录浏览当前仅支持 Linux 服务器。"));
        return;
    }

    if (m_host->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法浏览"), QStringLiteral("请先填写主机地址。"));
        return;
    }

    RemoteConnectionContext context;
    if (!buildConnectionContext(&context, nullptr)) {
        QMessageBox::warning(this, QStringLiteral("无法浏览"), QStringLiteral("请先填写主机、用户名和密码。"));
        return;
    }

    const QString authMode = context.serverConfig.value(QStringLiteral("auth")).toObject()
                                 .value(QStringLiteral("mode")).toString();
    if (authMode != QStringLiteral("ssh-key") && context.password.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法浏览"), QStringLiteral("请先输入或保存服务器密码。"));
        return;
    }

    RemoteFileBrowserDialog picker(context,
                                   this,
                                   RemoteFileBrowserDialog::Purpose::PickDirectory);
    const QString current = m_defaultRemoteBaseDir->path();
    if (!current.isEmpty()) {
        picker.setInitialPath(current);
    }
    if (picker.exec() == QDialog::Accepted) {
        m_defaultRemoteBaseDir->setPath(picker.selectedPath());
    }
}

void ServerDialog::onOsChanged(int)
{
    const QString previousOs = m_os->currentData().toString() == QStringLiteral("windows")
        ? QStringLiteral("linux")
        : QStringLiteral("windows");
    const QString currentOs = m_os->currentData().toString();
    const QString previousUsername = defaultUsernameForOs(previousOs);
    const QString previousBaseDir = defaultRemoteBaseDirForOs(previousOs);

    syncOsFields();
    if (currentOs == QStringLiteral("windows")) {
        m_port->setValue(5986);
        if (m_authMode->currentData().toString() == QStringLiteral("ssh-key")) {
            m_authMode->setCurrentIndex(0);
        }
    } else {
        m_port->setValue(22);
    }
    if (m_username->text().trimmed().isEmpty() || m_username->text().trimmed() == previousUsername) {
        m_username->setText(defaultUsernameForOs(currentOs));
    }
    if (m_defaultRemoteBaseDir->path().trimmed().isEmpty()
        || m_defaultRemoteBaseDir->path().trimmed() == previousBaseDir) {
        m_defaultRemoteBaseDir->setPath(defaultRemoteBaseDirForOs(currentOs));
    }
    m_username->setPlaceholderText(defaultUsernameForOs(currentOs));
    m_defaultRemoteBaseDir->setPlaceholderText(defaultRemoteBaseDirForOs(currentOs));
    syncAuthFields();
}

void ServerDialog::syncOsFields()
{
    m_osStack->setCurrentIndex(m_os->currentIndex());
}

void ServerDialog::onAuthModeChanged(int)
{
    syncAuthFields();
}

void ServerDialog::syncAuthFields()
{
    const QString authUiMode = m_authMode->currentData().toString();
    const bool linux = m_os->currentData().toString() == QStringLiteral("linux");

    m_authMode->setItemData(1, linux ? QVariant(1) : QVariant(0), Qt::UserRole - 1);

    if (authUiMode == QStringLiteral("manual")) {
        m_authStack->setCurrentIndex(1);
    } else {
        m_authStack->setCurrentIndex(0);
    }

    if (m_sshKeyRow != nullptr) {
        const bool showSshKey = authUiMode == QStringLiteral("ssh-key") && linux;
        m_sshKeyRow->setVisible(showSshKey);
        if (m_sshKeyLabel != nullptr) {
            m_sshKeyLabel->setVisible(showSshKey);
        }
    }
    if (m_testConnectionButton != nullptr) {
        m_testConnectionButton->setVisible(authUiMode == QStringLiteral("password"));
    }
}

void ServerDialog::onTestConnection()
{
    RemoteConnectionContext context;
    QString error;
    if (!buildConnectionContext(&context, &error)) {
        QMessageBox::warning(this, QStringLiteral("无法测试连接"), error);
        return;
    }

    const ValidationResult validation = ConfigValidator::validateServer(context.serverConfig);
    if (!validation.ok) {
        QMessageBox::warning(this, QStringLiteral("配置无效"), validation.errors.join(QStringLiteral("\n")));
        return;
    }

    const int generation = ++m_testConnectionGeneration;
    const QString originalLabel = m_testConnectionButton->text();
    m_testConnectionButton->setEnabled(false);
    m_testConnectionButton->setText(QStringLiteral("测试中..."));

    const HostKeyPromptHandler hostKeyPrompt = makeThreadSafeHostKeyPromptHandler(this);
    auto *watcher = new QFutureWatcher<RemoteCommandResult>(this);
    connect(watcher, &QFutureWatcher<RemoteCommandResult>::finished, this, [this, watcher, generation, originalLabel]() {
        watcher->deleteLater();
        if (generation != m_testConnectionGeneration) {
            return;
        }

        m_testConnectionButton->setEnabled(true);
        m_testConnectionButton->setText(originalLabel);

        const RemoteCommandResult result = watcher->result();
        if (result.ok) {
            const QString detail = result.stdoutText.trimmed();
            const QString message = detail.isEmpty()
                ? QStringLiteral("已成功连接到服务器。")
                : QStringLiteral("已成功连接到服务器。\n\n%1").arg(detail);
            QMessageBox::information(this, QStringLiteral("连接成功"), message);
            return;
        }

        QString failure = result.error.trimmed();
        if (failure.isEmpty()) {
            failure = result.stderrText.trimmed();
        }
        if (failure.isEmpty()) {
            failure = QStringLiteral("连接失败，请检查主机、端口、用户名和密码。");
        }
        QMessageBox::warning(this, QStringLiteral("连接失败"), failure);
    });

    const RemoteConnectionContext contextCopy = context;
    watcher->setFuture(QtConcurrent::run([contextCopy, hostKeyPrompt]() {
        return runConnectionTest(contextCopy, hostKeyPrompt);
    }));
}

void ServerDialog::onAccept()
{
    const QJsonObject config = server();
    const ValidationResult validation = ConfigValidator::validateServer(config);
    if (!validation.ok) {
        QMessageBox::warning(this, QStringLiteral("配置无效"), validation.errors.join(QStringLiteral("\n")));
        return;
    }

    if (m_authMode->currentData().toString() == QStringLiteral("password")
        && m_rememberPassword->isChecked()
        && m_password->text().isEmpty()
        && !m_hasStoredPassword) {
        QMessageBox::warning(this, QStringLiteral("配置无效"), QStringLiteral("勾选记住密码时请填写密码。"));
        return;
    }

    accept();
}
