#include "ui/tools/pages/AiConnectionDialog.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/CredentialStore.h"
#include "ui/PageLayout.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

AiConnectionDialog::AiConnectionDialog(CredentialStore *credentials,
                                       Mode mode,
                                       AiConnection seed,
                                       const QVector<AiConnection> &existing,
                                       QWidget *parent)
    : QDialog(parent)
    , m_credentials(credentials)
    , m_mode(mode)
    , m_draft(std::move(seed))
    , m_existing(existing)
{
    if (m_draft.id.isEmpty()) {
        m_draft.id = AiSettings::generateConnectionId();
    }
    if (m_draft.name.isEmpty()) {
        m_draft.name = QStringLiteral("新连接");
    }
    if (m_draft.credentialRef.isEmpty()) {
        m_draft.credentialRef = AiSettingsStore::buildCredentialRef(m_draft.id);
    }
    if (m_draft.model.isEmpty()) {
        m_draft.model = QStringLiteral("gpt-4o-mini");
    }

    setWindowTitle(m_mode == Mode::Create ? QStringLiteral("添加 AI 连接") : QStringLiteral("编辑 AI 连接"));
    PageLayout::applyFormModalDialog(this,
                                     PageLayout::FormDialogDefaultWidth,
                                     PageLayout::FormDialogDefaultHeight);
    buildUi();
    applyToWidgets();

    m_testClient = new OpenAiChatClient(this);
}

void AiConnectionDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *body = PageLayout::wrapScrollableBody(layout);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space16);

    QFormLayout *form = nullptr;
    auto *infoCard = PageLayout::wrapDialogFormSection(
        QStringLiteral("连接信息"), body, &form);
    form->setContentsMargins(0, 0, 0, 0);

    m_nameEdit = new QLineEdit(infoCard);
    m_nameEdit->setPlaceholderText(QStringLiteral("例如 OpenAI 主账号"));
    PageLayout::configureFormInput(m_nameEdit);
    form->addRow(QStringLiteral("名称"), m_nameEdit);

    m_baseUrlEdit = new QLineEdit(infoCard);
    m_baseUrlEdit->setPlaceholderText(QStringLiteral("https://api.openai.com/v1"));
    PageLayout::configureFormInput(m_baseUrlEdit);
    form->addRow(QStringLiteral("API Base URL"), m_baseUrlEdit);

    m_modelCombo = new QComboBox(infoCard);
    m_modelCombo->setEditable(true);
    m_modelCombo->setInsertPolicy(QComboBox::NoInsert);
    m_modelCombo->addItems(AiSettingsStore::defaultModelPresets());
    m_modelCombo->setPlaceholderText(QStringLiteral("gpt-4o-mini"));
    m_modelCombo->setProperty("manualEdit", true);
    PageLayout::configureFormInput(m_modelCombo);
    if (QLineEdit *le = m_modelCombo->lineEdit()) {
        le->setReadOnly(false);
        le->setPlaceholderText(QStringLiteral("gpt-4o-mini"));
    }
    form->addRow(QStringLiteral("模型"), m_modelCombo);

    auto *secretCard = PageLayout::wrapDialogFormSection(
        QStringLiteral("凭据"), body, &form);
    form->setContentsMargins(0, 0, 0, 0);

    auto *keyRow = new QWidget(secretCard);
    auto *keyLayout = new QHBoxLayout(keyRow);
    keyLayout->setContentsMargins(0, 0, 0, 0);
    keyLayout->setSpacing(PageLayout::Space8);
    m_apiKeyEdit = new QLineEdit(keyRow);
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setPlaceholderText(QStringLiteral("留空表示不修改已保存的 Key"));
    PageLayout::configureFormInput(m_apiKeyEdit);
    m_visibilityButton = new QPushButton(QStringLiteral("显示"), keyRow);
    m_visibilityButton->setObjectName(QStringLiteral("aiVisibilityButton"));
    m_visibilityButton->setCheckable(true);
    m_visibilityButton->setFixedSize(56, PageLayout::DialogFieldHeight);
    connect(m_visibilityButton, &QPushButton::toggled, this, [this](bool visible) {
        m_apiKeyEdit->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password);
        m_visibilityButton->setText(visible ? QStringLiteral("隐藏") : QStringLiteral("显示"));
    });
    keyLayout->addWidget(m_apiKeyEdit, 1);
    keyLayout->addWidget(m_visibilityButton);
    form->addRow(QStringLiteral("API Key"), keyRow);

    m_rememberKeyCheck = new QCheckBox(QStringLiteral("记住 Key"), secretCard);
    m_rememberKeyCheck->setChecked(true);
    form->addRow(QString(), m_rememberKeyCheck);

    bodyLayout->addWidget(infoCard);
    bodyLayout->addWidget(secretCard);

    auto *actionsCard = new QFrame(body);
    auto *actionsLayout = new QHBoxLayout(actionsCard);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(PageLayout::Space12);

    m_message = new QLabel(actionsCard);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    m_message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    actionsLayout->addWidget(m_message, 1);

    m_testButton = new QPushButton(QStringLiteral("测试连接"), actionsCard);
    m_testButton->setObjectName(QStringLiteral("secondaryButton"));
    actionsLayout->addWidget(m_testButton, 0, Qt::AlignRight);
    bodyLayout->addWidget(actionsCard);

    connect(m_testButton, &QPushButton::clicked, this, &AiConnectionDialog::onTestConnection);

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);
    footerLayout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, footer);
    buttons->setCenterButtons(false);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("secondaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &AiConnectionDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);
}

void AiConnectionDialog::applyToWidgets()
{
    m_nameEdit->setText(m_draft.name);
    m_baseUrlEdit->setText(m_draft.apiBaseUrl);
    {
        const int index = m_modelCombo->findText(m_draft.model);
        if (index >= 0) {
            m_modelCombo->setCurrentIndex(index);
        } else {
            m_modelCombo->setEditText(m_draft.model);
        }
    }
    if (m_credentials != nullptr && m_credentials->has(m_draft.credentialRef)) {
        m_apiKeyEdit->setPlaceholderText(QStringLiteral("已保存 Key，留空表示不修改"));
        m_rememberKeyCheck->setChecked(true);
    } else {
        m_apiKeyEdit->setPlaceholderText(QStringLiteral("输入 API Key"));
        m_rememberKeyCheck->setChecked(false);
    }
}

void AiConnectionDialog::syncFromWidgets()
{
    m_draft.name = m_nameEdit->text().trimmed();
    m_draft.apiBaseUrl = m_baseUrlEdit->text().trimmed();
    m_draft.model = m_modelCombo->currentText().trimmed();
    m_secret = m_apiKeyEdit->text();
    m_hasSecret = !m_secret.isEmpty();
    m_clearStoredSecret = m_mode == Mode::Edit && !m_rememberKeyCheck->isChecked();
}

void AiConnectionDialog::onTestConnection()
{
    syncFromWidgets();
    if (m_draft.apiBaseUrl.isEmpty() || m_draft.model.isEmpty()) {
        m_message->setText(QStringLiteral("请先填写 API Base URL 和模型"));
        return;
    }
    QString key = m_secret;
    if (key.isEmpty() && m_credentials != nullptr && m_credentials->has(m_draft.credentialRef)) {
        key = m_credentials->load(m_draft.credentialRef);
    }
    if (key.isEmpty()) {
        m_message->setText(QStringLiteral("请先填写 API Key"));
        return;
    }

    m_testClient->abort();
    const int generation = ++m_testGeneration;
    m_testButton->setEnabled(false);
    m_message->setText(QStringLiteral("正在测试连接..."));

    connect(m_testClient, &OpenAiChatClient::finished, this, [this, generation]() {
        if (generation != m_testGeneration) {
            return;
        }
        m_message->setText(QStringLiteral("连接成功"));
        m_testButton->setEnabled(true);
    }, Qt::SingleShotConnection);
    connect(m_testClient, &OpenAiChatClient::failed, this, [this, generation](const QString &error) {
        if (generation != m_testGeneration) {
            return;
        }
        m_message->setText(QStringLiteral("连接失败：%1").arg(error));
        m_testButton->setEnabled(true);
    }, Qt::SingleShotConnection);

    m_testClient->testConnection(m_draft.apiBaseUrl, key, m_draft.model);
}

void AiConnectionDialog::onAccept()
{
    syncFromWidgets();
    if (m_draft.name.isEmpty()) {
        m_message->setText(QStringLiteral("请填写连接名称"));
        return;
    }
    if (m_draft.apiBaseUrl.isEmpty()) {
        m_message->setText(QStringLiteral("请填写 API Base URL"));
        return;
    }
    if (m_draft.model.isEmpty()) {
        m_message->setText(QStringLiteral("请填写模型名称"));
        return;
    }
    for (const AiConnection &existing : m_existing) {
        if (existing.id == m_draft.id) {
            continue;
        }
        if (!existing.name.isEmpty() && existing.name == m_draft.name) {
            m_message->setText(QStringLiteral("已存在同名连接：%1").arg(existing.name));
            return;
        }
    }
    if (m_mode == Mode::Edit && !m_rememberKeyCheck->isChecked() && !m_hasSecret) {
        m_clearStoredSecret = true;
    }
    if (m_rememberKeyCheck->isChecked() && !m_hasSecret
        && m_credentials != nullptr && !m_credentials->has(m_draft.credentialRef)) {
        m_message->setText(QStringLiteral("勾选记住 Key 时请填写 API Key"));
        return;
    }
    accept();
}
