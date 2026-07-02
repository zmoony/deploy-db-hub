#include "ui/tools/pages/AiConnectionManagerDialog.h"

#include "adapters/ai/OpenAiChatClient.h"
#include "infra/AiSettingsStore.h"
#include "infra/CredentialStore.h"
#include "ui/PageLayout.h"
#include "ui/tools/pages/AiConnectionDialog.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

AiConnectionManagerDialog::AiConnectionManagerDialog(CredentialStore *credentials, QWidget *parent)
    : QDialog(parent)
    , m_credentials(credentials)
{
    setWindowTitle(QStringLiteral("AI 连接管理"));
    PageLayout::applyRemoteToolDialog(this,
                                      720,
                                      460,
                                      880,
                                      560);

    buildUi();

    {
        AiSettings settings;
        QString error;
        AiSettingsStore store(AiSettingsStore::defaultSettingsFile());
        store.load(&settings, &error);
        m_workingConnections = settings.connections;
        m_workingActiveId = settings.activeConnectionId;
    }

    reloadList();
}

void AiConnectionManagerDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *body = PageLayout::wrapScrollableBody(layout);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(PageLayout::Space16);

    auto *columns = new QHBoxLayout;
    columns->setSpacing(PageLayout::Space16);

    auto *listCard = new QFrame(body);
    listCard->setObjectName(QStringLiteral("aiConfigSection"));
    listCard->setAttribute(Qt::WA_StyledBackground, true);
    listCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    PageLayout::applyLighterCardShadow(listCard);
    auto *listLayout = new QVBoxLayout(listCard);
    listLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    listLayout->setSpacing(PageLayout::Space12);
    listLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("已保存的连接"), listCard));

    m_list = new QListWidget(listCard);
    m_list->setObjectName(QStringLiteral("toolListNav"));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setUniformItemSizes(true);
    listLayout->addWidget(m_list, 1);

    auto *detailCard = new QFrame(body);
    detailCard->setObjectName(QStringLiteral("aiConfigSection"));
    detailCard->setAttribute(Qt::WA_StyledBackground, true);
    detailCard->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    PageLayout::applyLighterCardShadow(detailCard);
    auto *detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    detailLayout->setSpacing(PageLayout::Space12);
    detailLayout->addWidget(PageLayout::makeSectionLabel(QStringLiteral("连接详情"), detailCard));

    m_detail = new QLabel(detailCard);
    m_detail->setObjectName(QStringLiteral("mutedText"));
    m_detail->setWordWrap(true);
    m_detail->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_detail->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    detailLayout->addWidget(m_detail, 1);

    m_message = new QLabel(detailCard);
    m_message->setObjectName(QStringLiteral("toolMessage"));
    m_message->setTextInteractionFlags(Qt::TextSelectableByMouse);
    detailLayout->addWidget(m_message);

    columns->addWidget(listCard, 1);
    columns->addWidget(detailCard, 1);
    bodyLayout->addLayout(columns, 1);

    auto *actionRow = new QWidget(body);
    auto *actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(0, 0, 0, 0);
    actionLayout->setSpacing(PageLayout::Space8);
    m_addButton = new QPushButton(QStringLiteral("添加"), actionRow);
    m_addButton->setObjectName(QStringLiteral("primaryButton"));
    m_editButton = new QPushButton(QStringLiteral("编辑"), actionRow);
    m_editButton->setObjectName(QStringLiteral("secondaryButton"));
    m_deleteButton = new QPushButton(QStringLiteral("删除"), actionRow);
    m_deleteButton->setObjectName(QStringLiteral("secondaryButton"));
    m_testButton = new QPushButton(QStringLiteral("测试连接"), actionRow);
    m_testButton->setObjectName(QStringLiteral("secondaryButton"));
    m_setActiveButton = new QPushButton(QStringLiteral("设为默认"), actionRow);
    m_setActiveButton->setObjectName(QStringLiteral("secondaryButton"));
    actionLayout->addWidget(m_addButton);
    actionLayout->addWidget(m_editButton);
    actionLayout->addWidget(m_deleteButton);
    actionLayout->addWidget(m_testButton);
    actionLayout->addStretch();
    actionLayout->addWidget(m_setActiveButton);
    bodyLayout->addWidget(actionRow);

    connect(m_addButton, &QPushButton::clicked, this, &AiConnectionManagerDialog::onAdd);
    connect(m_editButton, &QPushButton::clicked, this, &AiConnectionManagerDialog::onEdit);
    connect(m_deleteButton, &QPushButton::clicked, this, &AiConnectionManagerDialog::onDelete);
    connect(m_testButton, &QPushButton::clicked, this, &AiConnectionManagerDialog::onTest);
    connect(m_setActiveButton, &QPushButton::clicked, this, &AiConnectionManagerDialog::onSetActive);
    connect(m_list, &QListWidget::currentRowChanged, this, &AiConnectionManagerDialog::onCurrentChanged);

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);
    footerLayout->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, footer);
    buttons->setCenterButtons(false);
    buttons->button(QDialogButtonBox::Save)->setObjectName(QStringLiteral("primaryButton"));
    buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("secondaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);

    onCurrentChanged(-1);
}

void AiConnectionManagerDialog::reloadList()
{
    m_list->clear();
    for (int i = 0; i < m_workingConnections.size(); ++i) {
        const AiConnection &connection = m_workingConnections.at(i);
        const QString marker = connection.id == m_workingActiveId
            ? QStringLiteral(" ★")
            : QString();
        auto *item = new QListWidgetItem(connection.name + marker, m_list);
        item->setData(Qt::UserRole, connection.id);
        item->setToolTip(connection.apiBaseUrl);
        if (connection.id == m_workingActiveId) {
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
    }
    if (m_list->count() > 0) {
        int activeRow = 0;
        for (int i = 0; i < m_workingConnections.size(); ++i) {
            if (m_workingConnections.at(i).id == m_workingActiveId) {
                activeRow = i;
                break;
            }
        }
        m_list->setCurrentRow(activeRow);
    } else {
        onCurrentChanged(-1);
    }
}

void AiConnectionManagerDialog::onCurrentChanged(int row)
{
    const bool hasItem = row >= 0 && row < m_workingConnections.size();
    m_editButton->setEnabled(hasItem);
    m_deleteButton->setEnabled(hasItem);
    m_testButton->setEnabled(hasItem);
    m_setActiveButton->setEnabled(hasItem && m_workingConnections.at(row).id != m_workingActiveId);

    if (!hasItem) {
        m_detail->setText(QStringLiteral("请选择左侧连接查看详情。"));
        return;
    }
    const AiConnection &connection = m_workingConnections.at(row);
    bool hasSecret = m_credentials != nullptr && m_credentials->has(connection.credentialRef);
    QString text;
    text += QStringLiteral("名称：%1\n").arg(connection.name);
    text += QStringLiteral("Base URL：%1\n").arg(connection.apiBaseUrl.isEmpty() ? QStringLiteral("（未填写）") : connection.apiBaseUrl);
    text += QStringLiteral("模型：%1\n").arg(connection.model);
    text += QStringLiteral("API Key：%1").arg(hasSecret ? QStringLiteral("已保存") : QStringLiteral("未保存"));
    if (connection.id == m_workingActiveId) {
        text += QStringLiteral("\n默认连接");
    }
    m_detail->setText(text);
}

void AiConnectionManagerDialog::onAdd()
{
    AiConnection seed;
    AiConnectionDialog dialog(m_credentials,
                              AiConnectionDialog::Mode::Create,
                              seed,
                              m_workingConnections,
                              this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    AiConnection created = dialog.connection();
    QString secret = dialog.secret();
    bool hasSecret = dialog.hasSecret();
    if (created.id.isEmpty()) {
        created.id = AiSettings::generateConnectionId();
    }
    if (created.credentialRef.isEmpty()) {
        created.credentialRef = AiSettingsStore::buildCredentialRef(created.id);
    }
    m_workingConnections.append(created);
    if (m_workingActiveId.isEmpty()) {
        m_workingActiveId = created.id;
    }
    if (hasSecret && m_credentials != nullptr) {
        QString err;
        if (!m_credentials->save(created.credentialRef, secret, &err)) {
            m_message->setText(QStringLiteral("保存 Key 失败：%1").arg(err));
        }
    }
    reloadList();
    m_message->setText(QStringLiteral("已添加连接：%1").arg(created.name));
}

void AiConnectionManagerDialog::onEdit()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_workingConnections.size()) {
        return;
    }
    AiConnection editing = m_workingConnections.at(row);
    AiConnectionDialog dialog(m_credentials,
                              AiConnectionDialog::Mode::Edit,
                              editing,
                              m_workingConnections,
                              this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    AiConnection updated = dialog.connection();
    QString secret = dialog.secret();
    bool hasSecret = dialog.hasSecret();
    bool clearStored = dialog.clearStoredSecret();
    updated.credentialRef = editing.credentialRef;
    if (updated.credentialRef.isEmpty()) {
        updated.credentialRef = AiSettingsStore::buildCredentialRef(updated.id);
    }
    m_workingConnections[row] = updated;
    if (clearStored && m_credentials != nullptr) {
        QString err;
        m_credentials->remove(updated.credentialRef, &err);
    } else if (hasSecret && m_credentials != nullptr) {
        QString err;
        if (!m_credentials->save(updated.credentialRef, secret, &err)) {
            m_message->setText(QStringLiteral("保存 Key 失败：%1").arg(err));
        }
    }
    reloadList();
    m_message->setText(QStringLiteral("已更新连接：%1").arg(updated.name));
}

void AiConnectionManagerDialog::onDelete()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_workingConnections.size()) {
        return;
    }
    const AiConnection &connection = m_workingConnections.at(row);
    const QString id = connection.id;
    const QString name = connection.name;
    const QString ref = connection.credentialRef;

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this,
        QStringLiteral("确认删除"),
        QStringLiteral("确定删除连接「%1」？此操作不会删除已保存的 API Key。").arg(name));
    if (answer != QMessageBox::Yes) {
        return;
    }

    m_workingConnections.remove(row);
    if (m_workingActiveId == id) {
        m_workingActiveId = m_workingConnections.isEmpty()
            ? QString()
            : m_workingConnections.first().id;
    }
    if (m_credentials != nullptr) {
        QString err;
        m_credentials->remove(ref, &err);
    }
    m_removedCredentialRefs.removeAll(ref);
    reloadList();
    m_message->setText(QStringLiteral("已删除连接：%1").arg(name));
}

void AiConnectionManagerDialog::onSetActive()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_workingConnections.size()) {
        return;
    }
    const QString id = m_workingConnections.at(row).id;
    if (id == m_workingActiveId) {
        return;
    }
    m_workingActiveId = id;
    reloadList();
    m_message->setText(QStringLiteral("已将「%1」设为默认连接").arg(m_workingConnections.at(row).name));
}

void AiConnectionManagerDialog::onTest()
{
    int row = m_list->currentRow();
    if (row < 0 || row >= m_workingConnections.size()) {
        return;
    }
    const AiConnection &connection = m_workingConnections.at(row);
    if (connection.apiBaseUrl.isEmpty() || connection.model.isEmpty()) {
        m_message->setText(QStringLiteral("请先填写 Base URL 和模型"));
        return;
    }
    QString key;
    if (m_credentials != nullptr && m_credentials->has(connection.credentialRef)) {
        key = m_credentials->load(connection.credentialRef);
    }
    if (key.isEmpty()) {
        m_message->setText(QStringLiteral("未找到已保存的 Key，请先在编辑中填写"));
        return;
    }

    auto *testClient = new OpenAiChatClient(this);
    m_testButton->setEnabled(false);
    m_message->setText(QStringLiteral("正在测试连接..."));

    connect(testClient, &OpenAiChatClient::finished, this, [this, testClient]() {
        m_message->setText(QStringLiteral("连接成功"));
        m_testButton->setEnabled(true);
        testClient->deleteLater();
    }, Qt::SingleShotConnection);
    connect(testClient, &OpenAiChatClient::failed, this, [this, testClient](const QString &error) {
        m_message->setText(QStringLiteral("连接失败：%1").arg(error));
        m_testButton->setEnabled(true);
        testClient->deleteLater();
    }, Qt::SingleShotConnection);

    testClient->testConnection(connection.apiBaseUrl, key, connection.model);
}

void AiConnectionManagerDialog::applyAiSettings(AiSettings *settings) const
{
    settings->connections = m_workingConnections;
    settings->activeConnectionId = m_workingActiveId;
}

void AiConnectionManagerDialog::persist()
{
    AiSettingsStore store(AiSettingsStore::defaultSettingsFile());
    AiSettings settings;
    QString error;
    store.load(&settings, &error);
    applyAiSettings(&settings);
    QString saveError;
    if (!store.save(settings, &saveError)) {
        m_message->setText(QStringLiteral("保存失败：%1").arg(saveError));
    }
}

void AiConnectionManagerDialog::accept()
{
    persist();
    QDialog::accept();
}
