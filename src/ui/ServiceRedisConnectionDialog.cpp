#include "ui/ServiceRedisConnectionDialog.h"

#include "adapters/services/ServiceBroker.h"
#include "infra/ServiceNodeConnection.h"
#include "ui/PageLayout.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

namespace {

QString connectionIdFromName(const QString &name, const QString &host, int port)
{
    QString id = name.trimmed();
    if (id.isEmpty()) {
        id = QStringLiteral("%1-%2").arg(host.trimmed(), QString::number(port));
    }
    id.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9._-]+")), QStringLiteral("-"));
    while (id.contains(QStringLiteral("--"))) {
        id.replace(QStringLiteral("--"), QStringLiteral("-"));
    }
    return id.trimmed();
}

}

ServiceRedisConnectionDialog::ServiceRedisConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("新建 Redis 连接"));

    auto *layout = new QVBoxLayout(this);
    PageLayout::applyDialog(layout);

    QFormLayout *form = nullptr;
    layout->addWidget(PageLayout::wrapDialogFormSection(QStringLiteral("连接信息"), this, &form));

    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(QStringLiteral("例如 测试 Redis"));
    PageLayout::configureFormInput(m_name);
    form->addRow(QStringLiteral("连接名称"), m_name);

    m_host = new QLineEdit(this);
    m_host->setPlaceholderText(QStringLiteral("IP 或域名"));
    PageLayout::configureFormInput(m_host);
    form->addRow(QStringLiteral("地址"), m_host);

    m_port = new QLineEdit(this);
    PageLayout::configureFormInput(m_port);
    form->addRow(QStringLiteral("端口"), m_port);

    m_username = new QLineEdit(this);
    m_username->setPlaceholderText(QStringLiteral("可选，ACL 用户名"));
    PageLayout::configureFormInput(m_username);
    form->addRow(QStringLiteral("用户名"), m_username);

    auto *passwordRow = new QWidget(this);
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
    form->addRow(QStringLiteral("密码"), passwordRow);

    m_database = new QSpinBox(this);
    m_database->setRange(0, 15);
    PageLayout::configureFormInput(m_database);
    form->addRow(QStringLiteral("数据库"), m_database);

    auto *footer = PageLayout::makeDialogFooter(this);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(0, PageLayout::Space12, 0, 0);
    footerLayout->setSpacing(PageLayout::Space8);

    m_testConnectionButton = new QPushButton(QStringLiteral("测试连接"), footer);
    m_testConnectionButton->setMinimumHeight(PageLayout::DialogFieldHeight);
    connect(m_testConnectionButton, &QPushButton::clicked, this, &ServiceRedisConnectionDialog::onTestConnection);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, footer);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primaryButton"));
    connect(buttons, &QDialogButtonBox::accepted, this, &ServiceRedisConnectionDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    footerLayout->addWidget(m_testConnectionButton);
    footerLayout->addStretch();
    footerLayout->addWidget(buttons);
    layout->addWidget(footer);

    m_port->setText(QStringLiteral("6379"));
    PageLayout::applyFormModalDialog(this);
}

void ServiceRedisConnectionDialog::setInstance(const QJsonObject &instance, bool editMode)
{
    m_editMode = editMode;
    m_existingId = instance.value(QStringLiteral("id")).toString();
    setWindowTitle(editMode ? QStringLiteral("编辑 Redis 连接") : QStringLiteral("新建 Redis 连接"));

    m_name->setText(instance.value(QStringLiteral("name")).toString());

    const QJsonObject node = ServiceNodeConnection::primaryNode(instance);
    const QString host = node.value(QStringLiteral("customHost")).toString().trimmed();
    m_host->setText(host.isEmpty()
                        ? node.value(QStringLiteral("serverLabel")).toString().section(QLatin1Char(':'), 0, 0).trimmed()
                        : host);

    ServiceConnectionFields fields;
    ServiceNodeConnection::decodeInfo(node.value(QStringLiteral("info")).toString(),
                                      QStringLiteral("redis"),
                                      &fields);
    m_port->setText(fields.port > 0 ? QString::number(fields.port) : QStringLiteral("6379"));
    m_username->setText(fields.username);
    m_password->setText(fields.password);
    m_database->setValue(qBound(0, node.value(QStringLiteral("redisDb")).toInt(0), 15));
}

QJsonObject ServiceRedisConnectionDialog::buildNode() const
{
    const QString host = m_host->text().trimmed();
    ServiceConnectionFields fields;
    fields.port = m_port->text().trimmed().toInt();
    fields.username = m_username->text().trimmed();
    fields.password = m_password->text();

    return QJsonObject{
        {QStringLiteral("serverId"), QString()},
        {QStringLiteral("serverLabel"), QStringLiteral("%1:%2").arg(host, QString::number(fields.port))},
        {QStringLiteral("customHost"), host},
        {QStringLiteral("info"), ServiceNodeConnection::encodeInfo(fields, QStringLiteral("redis"))},
        {QStringLiteral("redisDb"), m_database->value()},
        {QStringLiteral("installPath"), QString()},
        {QStringLiteral("storagePath"), QString()},
        {QStringLiteral("connectionMode"), QStringLiteral("direct")}
    };
}

QJsonObject ServiceRedisConnectionDialog::instance() const
{
    const QString name = m_name->text().trimmed();
    const QString host = m_host->text().trimmed();
    const int port = m_port->text().trimmed().toInt();
    const QString id = m_editMode && !m_existingId.isEmpty()
        ? m_existingId
        : connectionIdFromName(name, host, port);

    return QJsonObject{
        {QStringLiteral("id"), id},
        {QStringLiteral("name"), name},
        {QStringLiteral("product"), QStringLiteral("redis")},
        {QStringLiteral("status"), QStringLiteral("running")},
        {QStringLiteral("nodes"), QJsonArray{buildNode()}}
    };
}

void ServiceRedisConnectionDialog::onTestConnection()
{
    const QString host = m_host->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试连接"), QStringLiteral("请填写地址。"));
        return;
    }

    QJsonObject server;
    QString error;
    const QJsonObject node = buildNode();
    if (!ServiceNodeConnection::resolveServerForNode(node, nullptr, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("测试连接"), error);
        return;
    }

    QJsonObject instance{
        {QStringLiteral("nodes"), QJsonArray{node}}
    };
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
    watcher->setFuture(QtConcurrent::run([instanceCopy, serverCopy]() {
        return ServiceBroker::testInstance(instanceCopy, serverCopy, QStringLiteral("redis"), {});
    }));
}

void ServiceRedisConnectionDialog::onAccept()
{
    if (m_name->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写连接名称。"));
        return;
    }
    if (m_host->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("请填写地址。"));
        return;
    }
    bool ok = false;
    const int port = m_port->text().trimmed().toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        QMessageBox::warning(this, QStringLiteral("校验失败"), QStringLiteral("端口应为 1-65535 之间的整数。"));
        return;
    }
    accept();
}
