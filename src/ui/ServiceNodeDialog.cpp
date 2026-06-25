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
#include <QFont>
#include <QFormLayout>
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
    PageLayout::applyFormModalDialog(this);
}

void ServiceNodeDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    QWidget *body = PageLayout::wrapScrollableBody(layout);
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(0, 0, PageLayout::Space4, 0);
    bodyLayout->setSpacing(PageLayout::Space16);

    QFormLayout *form = nullptr;
    bodyLayout->addWidget(PageLayout::wrapDialogFormSection(QStringLiteral("节点配置"), body, &form));

    m_parentLabel = new QLabel(QStringLiteral("-"));
    m_parentLabel->setObjectName(QStringLiteral("serviceParentTag"));
    m_parentLabel->setWordWrap(true);
    form->addRow(QStringLiteral("所属"), m_parentLabel);

    m_directSection = new QWidget(body);
    auto *directForm = new QFormLayout(m_directSection);
    directForm->setContentsMargins(0, 0, 0, 0);
    directForm->setHorizontalSpacing(PageLayout::Space12);
    directForm->setVerticalSpacing(PageLayout::Space12);

    m_host = new QLineEdit(m_directSection);
    m_host->setPlaceholderText(QStringLiteral("IP 或域名"));
    PageLayout::configureFormInput(m_host);
    directForm->addRow(PageLayout::makeRequiredFormLabel(QStringLiteral("地址"), m_directSection), m_host);

    m_port = new QLineEdit(m_directSection);
    PageLayout::configureFormInput(m_port);
    directForm->addRow(PageLayout::makeRequiredFormLabel(QStringLiteral("端口"), m_directSection), m_port);

    auto *passwordRow = new QWidget(m_directSection);
    PageLayout::configureHorizontalFormRow(passwordRow);
    auto *passwordLayout = new QHBoxLayout(passwordRow);
    passwordLayout->setContentsMargins(0, 0, 0, 0);
    passwordLayout->setSpacing(PageLayout::Space8);
    m_password = new QLineEdit(passwordRow);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText(QStringLiteral("可选"));
    PageLayout::configureFormInput(m_password);
    m_passwordVisibilityButton = new QPushButton(QStringLiteral("显示"), passwordRow);
    m_passwordVisibilityButton->setCheckable(true);
    m_passwordVisibilityButton->setMinimumWidth(56);
    m_passwordVisibilityButton->setFixedHeight(PageLayout::DialogFieldHeight);
    connect(m_passwordVisibilityButton, &QPushButton::toggled, this, [this](bool checked) {
        m_password->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
        m_passwordVisibilityButton->setText(checked ? QStringLiteral("隐藏") : QStringLiteral("显示"));
    });
    passwordLayout->addWidget(m_password, 1);
    passwordLayout->addWidget(m_passwordVisibilityButton);
    directForm->addRow(QStringLiteral("密码"), passwordRow);

    m_usernameLabel = new QLabel(QStringLiteral("用户名"), m_directSection);
    m_username = new QLineEdit(m_directSection);
    m_username->setPlaceholderText(QStringLiteral("可选，Redis ACL / ES / 数据库账号"));
    PageLayout::configureFormInput(m_username);
    directForm->addRow(m_usernameLabel, m_username);

    m_databaseLabel = new QLabel(QStringLiteral("数据库"), m_directSection);
    m_database = new QLineEdit(m_directSection);
    m_database->setPlaceholderText(QStringLiteral("库名 / Service Name"));
    PageLayout::configureFormInput(m_database);
    directForm->addRow(m_databaseLabel, m_database);

    m_kibanaPortLabel = new QLabel(QStringLiteral("Kibana 端口"), m_directSection);
    m_kibanaPort = new QLineEdit(m_directSection);
    m_kibanaPort->setPlaceholderText(QStringLiteral("默认 5601"));
    PageLayout::configureFormInput(m_kibanaPort);
    directForm->addRow(m_kibanaPortLabel, m_kibanaPort);

    form->addRow(m_directSection);

    m_managedSection = new QWidget(body);
    auto *managedForm = new QFormLayout(m_managedSection);
    managedForm->setContentsMargins(0, 0, 0, 0);
    managedForm->setHorizontalSpacing(PageLayout::Space12);
    managedForm->setVerticalSpacing(PageLayout::Space12);

    auto *serverRow = new QWidget(m_managedSection);
    PageLayout::configureHorizontalFormRow(serverRow);
    auto *serverLayout = new QHBoxLayout(serverRow);
    serverLayout->setContentsMargins(0, 0, 0, 0);
    serverLayout->setSpacing(PageLayout::Space8);
    m_server = new QComboBox(serverRow);
    m_server->setEditable(true);
    m_server->setInsertPolicy(QComboBox::NoInsert);
    PageLayout::configureFormInput(m_server);
    if (QLineEdit *lineEdit = m_server->lineEdit()) {
        lineEdit->setPlaceholderText(QStringLiteral("选择已有服务器或直接输入 IP/主机名"));
        lineEdit->setClearButtonEnabled(true);
    }
    auto *addServerButton = new QPushButton(QStringLiteral("+"), serverRow);
    addServerButton->setObjectName(QStringLiteral("backNavButton"));
    addServerButton->setFixedSize(PageLayout::DialogFieldHeight, PageLayout::DialogFieldHeight);
    addServerButton->setFlat(true);
    addServerButton->setCursor(Qt::PointingHandCursor);
    addServerButton->setToolTip(QStringLiteral("请先在「部署工具 → 服务器管理」中登记服务器"));
    QFont addServerFont = addServerButton->font();
    addServerFont.setPixelSize(18);
    addServerFont.setBold(true);
    addServerButton->setFont(addServerFont);
    auto *refreshServerButton = new QPushButton(QStringLiteral("刷新"), serverRow);
    refreshServerButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    serverLayout->addWidget(m_server, 1);
    serverLayout->addWidget(addServerButton);
    serverLayout->addWidget(refreshServerButton);
    managedForm->addRow(PageLayout::makeRequiredFormLabel(QStringLiteral("服务器"), serverRow), serverRow);

    m_info = new QLineEdit(m_managedSection);
    m_info->setPlaceholderText(QStringLiteral("请输入"));
    PageLayout::configureFormInput(m_info);
    managedForm->addRow(QStringLiteral("信息"), m_info);

    m_installPath = new QLineEdit(m_managedSection);
    PageLayout::configureFormInput(m_installPath);
    managedForm->addRow(QStringLiteral("安装路径"), m_installPath);

    m_storagePath = new QLineEdit(m_managedSection);
    PageLayout::configureFormInput(m_storagePath);
    managedForm->addRow(QStringLiteral("存储路径"), m_storagePath);

    form->addRow(m_managedSection);

    bodyLayout->addWidget(PageLayout::wrapDialogHintSection(QStringLiteral("填写说明"), body, &m_instructions));

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);

    m_testConnectionButton = new QPushButton(QStringLiteral("测试连接"), footer);
    m_testConnectionButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    connect(m_testConnectionButton, &QPushButton::clicked, this, &ServiceNodeDialog::onTestConnection);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, footer);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ServiceNodeDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    footerLayout->addWidget(m_testConnectionButton);
    footerLayout->addStretch();
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);

    connect(refreshServerButton, &QPushButton::clicked, this, &ServiceNodeDialog::reloadServers);
    connect(addServerButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this,
                                 QStringLiteral("添加服务器"),
                                 QStringLiteral("请切换到「部署工具 → 服务器管理」新建目标机，完成后点击刷新。"));
    });
    syncProductFields();
}

int ServiceNodeDialog::defaultPortForProduct() const
{
    const QString productKey = serviceProductKindKey(m_product);
    const int fromDefaultInfo = ServiceDefaults::defaultInfo(productKey).toInt();
    if (fromDefaultInfo > 0) {
        return fromDefaultInfo;
    }
    if (m_product == ServiceProductKind::Oracle) {
        return 1521;
    }
    if (m_product == ServiceProductKind::PostgreSQL) {
        return 5432;
    }
    return 0;
}

void ServiceNodeDialog::syncProductFields()
{
    const bool direct = serviceProductUsesDirectConnect(m_product);
    m_directSection->setVisible(direct);
    m_managedSection->setVisible(!direct);

    const bool databaseProduct = m_product == ServiceProductKind::Oracle
        || m_product == ServiceProductKind::PostgreSQL;
    m_databaseLabel->setVisible(direct && databaseProduct);
    m_database->setVisible(direct && databaseProduct);

    const bool elasticsearch = m_product == ServiceProductKind::Elasticsearch;
    m_kibanaPortLabel->setVisible(direct && elasticsearch);
    m_kibanaPort->setVisible(direct && elasticsearch);

    if (direct && m_product == ServiceProductKind::Redis) {
        m_username->setPlaceholderText(QStringLiteral("ACL in Redis >= 6.0"));
    } else if (direct) {
        m_username->setPlaceholderText(QStringLiteral("可选"));
    }
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

void ServiceNodeDialog::populateDirectFieldsFromNode(const QJsonObject &node)
{
    const QString host = node.value(QStringLiteral("customHost")).toString().trimmed();
    if (!host.isEmpty()) {
        m_host->setText(host);
    } else {
        const QString label = node.value(QStringLiteral("serverLabel")).toString();
        m_host->setText(label.section(QLatin1Char(':'), 0, 0).trimmed());
    }

    ServiceConnectionFields fields;
    ServiceNodeConnection::decodeInfo(node.value(QStringLiteral("info")).toString(),
                                      serviceProductKindKey(m_product),
                                      &fields);
    m_port->setText(fields.port > 0 ? QString::number(fields.port) : QString::number(defaultPortForProduct()));
    m_username->setText(fields.username);
    m_password->setText(fields.password);
    m_database->setText(fields.database);

    if (m_product == ServiceProductKind::Elasticsearch) {
        const int kibanaPort = node.value(QStringLiteral("kibanaPort")).toInt(0);
        m_kibanaPort->setText(kibanaPort > 0
                                  ? QString::number(kibanaPort)
                                  : QString::number(ServiceDefaults::kibanaPort(serviceProductKindKey(m_product))));
    }
}

ServiceConnectionFields ServiceNodeDialog::directFieldsFromUi() const
{
    ServiceConnectionFields fields;
    fields.port = m_port->text().trimmed().toInt();
    fields.username = m_username->text().trimmed();
    fields.password = m_password->text();
    fields.database = m_database->text().trimmed();
    return fields;
}

void ServiceNodeDialog::setContext(const QString &parentInstanceName, ServiceProductKind product)
{
    m_product = product;
    m_editMode = false;
    setWindowTitle(QStringLiteral("添加连接"));
    m_parentLabel->setText(QStringLiteral("%1 · %2")
                               .arg(serviceParentCategoryLabel(product), parentInstanceName));
    syncInstructionText();
    syncProductFields();
    applyPathDefaults();

    const QString productKey = serviceProductKindKey(product);
    if (serviceProductUsesDirectConnect(product)) {
        m_host->clear();
        m_port->setText(QString::number(defaultPortForProduct()));
        m_username->clear();
        m_password->clear();
        m_database->clear();
        if (product == ServiceProductKind::Elasticsearch) {
            m_kibanaPort->setText(QString::number(ServiceDefaults::kibanaPort(productKey)));
        }
    } else {
        m_info->setPlaceholderText(serviceNodeInfoPlaceholder(product));
        m_info->setText(ServiceDefaults::defaultInfo(productKey));
        m_installPath->setText(ServiceDefaults::installPath(productKey));
        m_storagePath->setText(ServiceDefaults::storagePath(productKey));
    }
}

void ServiceNodeDialog::setNode(const QJsonObject &node, bool editMode)
{
    m_editMode = editMode;
    setWindowTitle(editMode ? QStringLiteral("编辑连接") : QStringLiteral("添加连接"));
    syncProductFields();

    if (serviceProductUsesDirectConnect(m_product)) {
        populateDirectFieldsFromNode(node);
        return;
    }

    const QString serverId = node.value(QStringLiteral("serverId")).toString();
    if (!serverId.isEmpty()) {
        const int index = m_server->findData(serverId);
        if (index >= 0) {
            m_server->setCurrentIndex(index);
        }
    } else {
        const QString customHost = node.value(QStringLiteral("customHost")).toString();
        m_server->setCurrentText(customHost.isEmpty()
                                     ? node.value(QStringLiteral("serverLabel")).toString()
                                     : customHost);
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
    const QString productKey = serviceProductKindKey(m_product);
    if (serviceProductUsesDirectConnect(m_product)) {
        const QString host = m_host->text().trimmed();
        const ServiceConnectionFields fields = directFieldsFromUi();
        QJsonObject nodeObject{
            {QStringLiteral("serverId"), QString()},
            {QStringLiteral("serverLabel"), host.isEmpty() ? host : QStringLiteral("%1:%2").arg(host, QString::number(fields.port))},
            {QStringLiteral("customHost"), host},
            {QStringLiteral("info"), ServiceNodeConnection::encodeInfo(fields, productKey)},
            {QStringLiteral("installPath"), QString()},
            {QStringLiteral("storagePath"), QString()},
            {QStringLiteral("connectionMode"), QStringLiteral("direct")}
        };
        if (m_product == ServiceProductKind::Elasticsearch) {
            const int kibanaPort = m_kibanaPort->text().trimmed().toInt();
            if (kibanaPort > 0) {
                nodeObject.insert(QStringLiteral("kibanaPort"), kibanaPort);
            }
        }
        return nodeObject;
    }

    const QString text = m_server->currentText().trimmed();
    QString serverId;
    QString serverLabel = text;
    QString customHost;

    for (int i = 0; i < m_server->count(); ++i) {
        if (m_server->itemText(i) == text) {
            const QString id = m_server->itemData(i).toString();
            if (!id.isEmpty()) {
                serverId = id;
                serverLabel = text;
                break;
            }
        }
    }
    if (serverId.isEmpty()) {
        customHost = text;
        serverLabel = text;
    }

    return QJsonObject{
        {QStringLiteral("serverId"), serverId},
        {QStringLiteral("serverLabel"), serverLabel},
        {QStringLiteral("customHost"), customHost},
        {QStringLiteral("info"), m_info->text().trimmed()},
        {QStringLiteral("installPath"), m_installPath->text().trimmed()},
        {QStringLiteral("storagePath"), m_storagePath->text().trimmed()},
        {QStringLiteral("connectionMode"), QStringLiteral("managed")}
    };
}

void ServiceNodeDialog::reloadServers()
{
    if (m_store == nullptr || m_server == nullptr) {
        return;
    }

    const QString currentText = m_server->currentText().trimmed();
    QString currentId;
    for (int i = 0; i < m_server->count(); ++i) {
        if (m_server->itemText(i) == currentText) {
            currentId = m_server->itemData(i).toString();
            break;
        }
    }

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

    if (!currentId.isEmpty()) {
        const int index = m_server->findData(currentId);
        m_server->setCurrentIndex(index >= 0 ? index : 0);
    } else if (!currentText.isEmpty() && currentText != QStringLiteral("请选择服务器")) {
        m_server->setCurrentText(currentText);
    } else {
        m_server->setCurrentIndex(0);
    }
}

void ServiceNodeDialog::onTestConnection()
{
    const QJsonObject nodeObj = node();
    if (serviceProductUsesDirectConnect(m_product)) {
        if (nodeObj.value(QStringLiteral("customHost")).toString().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("测试连接"), QStringLiteral("请填写地址。"));
            return;
        }
    } else if (nodeObj.value(QStringLiteral("serverLabel")).toString().isEmpty()
        || nodeObj.value(QStringLiteral("serverLabel")).toString() == QStringLiteral("请选择服务器")) {
        QMessageBox::warning(this, QStringLiteral("测试连接"), QStringLiteral("请选择服务器或输入 IP/主机名。"));
        return;
    }

    QJsonObject server;
    QString error;
    if (!ServiceNodeConnection::resolveServerForNode(nodeObj, m_store, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("测试连接"), error);
        return;
    }

    QJsonObject instance{{QStringLiteral("nodes"), QJsonArray{nodeObj}}};
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
    const QString productKey = serviceProductKindKey(m_product);
    if (serviceProductUsesDirectConnect(m_product)) {
        watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, productKey]() {
            return ServiceBroker::testInstance(instanceCopy, serverCopy, productKey, {});
        }));
        return;
    }

    const RemoteConnectionContext remote =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache, this);
    const RemoteConnectionContext remoteCopy = remote;
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy, remoteCopy, productKey]() {
        return ServiceBroker::testInstance(instanceCopy, serverCopy, productKey, remoteCopy);
    }));
}

void ServiceNodeDialog::onAccept()
{
    const QString productKey = serviceProductKindKey(m_product);
    if (serviceProductUsesDirectConnect(m_product)) {
        const QString host = m_host->text().trimmed();
        if (host.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写地址。"));
            return;
        }
        bool ok = false;
        const int port = m_port->text().trimmed().toInt(&ok);
        if (!ok || port <= 0 || port > 65535) {
            QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("端口应为 1-65535 之间的整数。"));
            return;
        }
        if ((m_product == ServiceProductKind::Oracle || m_product == ServiceProductKind::PostgreSQL)
            && m_database->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写数据库名。"));
            return;
        }
        if (m_product == ServiceProductKind::Elasticsearch) {
            const QString kibanaText = m_kibanaPort->text().trimmed();
            if (!kibanaText.isEmpty()) {
                const int kibanaPort = kibanaText.toInt(&ok);
                if (!ok || kibanaPort <= 0 || kibanaPort > 65535) {
                    QMessageBox::warning(this,
                                         QStringLiteral("校验失败"),
                                         QStringLiteral("Kibana 端口应为 1-65535 之间的整数。"));
                    return;
                }
            }
        }
        accept();
        return;
    }

    const QString serverText = m_server->currentText().trimmed();
    if (serverText.isEmpty() || serverText == QStringLiteral("请选择服务器")) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请选择服务器或输入 IP/主机名。"));
        return;
    }
    if (m_info->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写信息。"));
        return;
    }
    accept();
}
