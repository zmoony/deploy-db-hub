#include "ui/ServiceNodeDialog.h"

#include "adapters/services/ServiceBroker.h"
#include "infra/ConfigStore.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "infra/ServiceNodeConnection.h"
#include "infra/ServiceDefaults.h"
#include "ui/PageLayout.h"
#include "ui/RemoteCredentialResolver.h"

#include <QJsonArray>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QFutureWatcher>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

ServiceNodeDialog::ServiceNodeDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("添加节点"));
    buildUi();
    PageLayout::applyModalDialog(this);
    setMinimumSize(640, 480);
    resize(680, 520);
}

void ServiceNodeDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    auto *formBox = new QGroupBox(QStringLiteral("节点配置"));
    formBox->setObjectName(QStringLiteral("dialogFormBox"));
    auto *form = new QFormLayout(formBox);
    PageLayout::applyInlineForm(form);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_parentLabel = new QLabel(QStringLiteral("-"));
    m_parentLabel->setObjectName(QStringLiteral("serviceParentTag"));
    m_parentLabel->setWordWrap(true);
    form->addRow(QStringLiteral("所属"), m_parentLabel);

    auto *serverRow = new QWidget;
    auto *serverLayout = new QHBoxLayout(serverRow);
    serverLayout->setContentsMargins(0, 0, 0, 0);
    serverLayout->setSpacing(PageLayout::Space8);
    m_server = new QComboBox;
    m_server->setPlaceholderText(QStringLiteral("请选择服务器"));
    PageLayout::configureFormInput(m_server);
    auto *addServerButton = new QPushButton(QStringLiteral("+"));
    addServerButton->setFixedWidth(36);
    addServerButton->setToolTip(QStringLiteral("请先在「部署工具 → 服务器管理」中登记服务器"));
    auto *refreshServerButton = new QPushButton(QStringLiteral("刷新"));
    refreshServerButton->setFixedWidth(56);
    serverLayout->addWidget(m_server, 1);
    serverLayout->addWidget(addServerButton);
    serverLayout->addWidget(refreshServerButton);
    form->addRow(QStringLiteral("服务器 *"), serverRow);

    m_info = new QLineEdit;
    m_info->setPlaceholderText(QStringLiteral("请输入"));
    PageLayout::configureFormInput(m_info);
    form->addRow(QStringLiteral("信息"), m_info);

    m_installPath = new QLineEdit;
    PageLayout::configureFormInput(m_installPath);
    form->addRow(QStringLiteral("安装路径"), m_installPath);

    m_storagePath = new QLineEdit;
    PageLayout::configureFormInput(m_storagePath);
    form->addRow(QStringLiteral("存储路径"), m_storagePath);

    layout->addWidget(formBox);

    auto *hintBox = new QGroupBox(QStringLiteral("填写说明"));
    hintBox->setObjectName(QStringLiteral("serviceHintBox"));
    auto *hintLayout = new QVBoxLayout(hintBox);
    hintLayout->setContentsMargins(PageLayout::Space16, PageLayout::Space16, PageLayout::Space16, PageLayout::Space16);
    m_instructions = new QLabel(hintBox);
    m_instructions->setObjectName(QStringLiteral("serviceHintText"));
    m_instructions->setWordWrap(true);
    hintLayout->addWidget(m_instructions);
    layout->addWidget(hintBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primaryButton"));
    m_testConnectionButton = new QPushButton(QStringLiteral("测试连接"));
    connect(m_testConnectionButton, &QPushButton::clicked, this, &ServiceNodeDialog::onTestConnection);
    connect(buttons, &QDialogButtonBox::accepted, this, &ServiceNodeDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_testConnectionButton);
    buttonRow->addStretch();
    buttonRow->addWidget(buttons);
    layout->addLayout(buttonRow);

    connect(refreshServerButton, &QPushButton::clicked, this, &ServiceNodeDialog::reloadServers);
    connect(addServerButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 QStringLiteral("添加服务器"),
                                 QStringLiteral("请切换到「部署工具 → 服务器管理」新建目标机，完成后点击刷新。"));
    });
}

void ServiceNodeDialog::setConfigStore(ConfigStore *store)
{
    m_store = store;
    reloadServers();
}

void ServiceNodeDialog::setCredentials(CredentialStore *credentials, CredentialSessionCache *sessionCache)
{
    m_credentials = credentials;
    m_sessionCache = sessionCache;
}

void ServiceNodeDialog::applyPathDefaults()
{
    const QString productKey = serviceProductKindKey(m_product);
    const QString installDefault = ServiceDefaults::installPath(productKey);
    const QString storageDefault = ServiceDefaults::storagePath(productKey);
    m_installPath->setPlaceholderText(installDefault.isEmpty()
                                        ? QStringLiteral("可留空或自定义")
                                        : QStringLiteral("默认 %1，可修改").arg(installDefault));
    m_storagePath->setPlaceholderText(storageDefault.isEmpty()
                                          ? QStringLiteral("可留空或自定义")
                                          : QStringLiteral("默认 %1，可修改").arg(storageDefault));
}

void ServiceNodeDialog::setContext(const QString &parentInstanceName, ServiceProductKind product)
{
    m_product = product;
    m_editMode = false;
    setWindowTitle(QStringLiteral("添加节点"));
    m_parentLabel->setText(QStringLiteral("%1 · %2")
                               .arg(serviceParentCategoryLabel(product), parentInstanceName));
    m_info->setPlaceholderText(serviceNodeInfoPlaceholder(product));
    syncInstructionText();
    applyPathDefaults();

    const QString productKey = serviceProductKindKey(product);
    m_info->setText(ServiceDefaults::defaultInfo(productKey));
    m_installPath->setText(ServiceDefaults::installPath(productKey));
    m_storagePath->setText(ServiceDefaults::storagePath(productKey));
}

void ServiceNodeDialog::setNode(const QJsonObject &node, bool editMode)
{
    m_editMode = editMode;
    setWindowTitle(editMode ? QStringLiteral("编辑节点") : QStringLiteral("添加节点"));

    const int index = m_server->findData(node.value(QStringLiteral("serverId")).toString());
    if (index >= 0) {
        m_server->setCurrentIndex(index);
    }
    m_info->setText(node.value(QStringLiteral("info")).toString());
    applyPathDefaults();

    const QString productKey = serviceProductKindKey(m_product);
    QString installPath = node.value(QStringLiteral("installPath")).toString();
    if (installPath.isEmpty()) {
        installPath = ServiceDefaults::installPath(productKey);
    }
    QString storagePath = node.value(QStringLiteral("storagePath")).toString();
    if (storagePath.isEmpty()) {
        storagePath = ServiceDefaults::storagePath(productKey);
    }
    m_installPath->setText(installPath);
    m_storagePath->setText(storagePath);
}

void ServiceNodeDialog::syncInstructionText()
{
    m_instructions->setText(serviceNodeFillInstructions(m_product));
}

QJsonObject ServiceNodeDialog::node() const
{
    return QJsonObject{
        {QStringLiteral("serverId"), m_server->currentData().toString()},
        {QStringLiteral("serverLabel"), m_server->currentText()},
        {QStringLiteral("info"), m_info->text().trimmed()},
        {QStringLiteral("installPath"), m_installPath->text().trimmed()},
        {QStringLiteral("storagePath"), m_storagePath->text().trimmed()}
    };
}

void ServiceNodeDialog::reloadServers()
{
    if (m_store == nullptr) {
        return;
    }

    const QString current = m_server->currentData().toString();
    m_server->clear();
    m_server->addItem(QStringLiteral("请选择服务器"), QString());

    QVector<StoredRecord> records;
    QString error;
    if (!m_store->listServers(&records, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    for (const StoredRecord &record : records) {
        const QJsonObject &server = record.config;
        const QString label = QStringLiteral("%1 (%2:%3)")
                                  .arg(server.value(QStringLiteral("name")).toString(),
                                       server.value(QStringLiteral("host")).toString(),
                                       QString::number(server.value(QStringLiteral("port")).toInt()));
        m_server->addItem(label, record.id);
    }

    const int index = m_server->findData(current);
    m_server->setCurrentIndex(index >= 0 ? index : 0);
}

void ServiceNodeDialog::onTestConnection()
{
    if (m_store == nullptr || m_server->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试连接"), QStringLiteral("请先选择服务器。"));
        return;
    }

    QJsonObject server;
    QString error;
    if (!m_store->getServer(m_server->currentData().toString(), &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("测试连接"), error);
        return;
    }

    QJsonObject instance{
        {QStringLiteral("nodes"),
         QJsonArray{QJsonObject{
             {QStringLiteral("serverId"), m_server->currentData().toString()},
             {QStringLiteral("info"), m_info->text().trimmed()},
             {QStringLiteral("installPath"), m_installPath->text().trimmed()},
             {QStringLiteral("storagePath"), m_storagePath->text().trimmed()}
         }}}
    };

    const RemoteConnectionContext remote =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache, this);
    m_testConnectionButton->setEnabled(false);

    auto *watcher = new QFutureWatcher<ServiceResult>(this);
    connect(watcher, &QFutureWatcher<ServiceResult>::finished, this, [this, watcher]() {
        m_testConnectionButton->setEnabled(true);
        const ServiceResult result = watcher->result();
        watcher->deleteLater();
        if (result.ok) {
            QMessageBox::information(this, QStringLiteral("测试连接"), result.message);
        } else {
            QMessageBox::warning(this, QStringLiteral("测试连接失败"), result.message);
        }
    });

    const QJsonObject instanceCopy = instance;
    const QJsonObject serverCopy = server;
    const RemoteConnectionContext remoteCopy = remote;
    const QString productKey = serviceProductKindKey(m_product);
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, remoteCopy, productKey]() {
        return ServiceBroker::testInstance(instanceCopy, serverCopy, productKey, remoteCopy);
    }));
}

void ServiceNodeDialog::onAccept()
{
    if (m_server->currentData().toString().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请选择服务器。"));
        return;
    }
    if (m_info->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写信息。"));
        return;
    }
    accept();
}
