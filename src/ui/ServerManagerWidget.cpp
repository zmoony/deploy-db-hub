#include "ui/ServerManagerWidget.h"

#include "ui/PageLayout.h"
#include "ui/RemoteFileBrowserDialog.h"
#include "ui/ServerDialog.h"
#include "ui/ServerMonitorDialog.h"

#include "ui/RemoteCredentialResolver.h"
#include "infra/ConfigStore.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"
#include "infra/DataPaths.h"

#include <QJsonObject>
#include <QHeaderView>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QTcpSocket>
#include <QtConcurrent>
#include <QPointer>
#include <QGraphicsDropShadowEffect>

namespace {

QLabel *createStatusDot(QWidget *parent)
{
    auto *dot = new QLabel(QStringLiteral("\u25CF"), parent);
    dot->setAlignment(Qt::AlignCenter);
    dot->setFixedSize(24, 24);
    dot->setStyleSheet(QStringLiteral("color: #D97706; font-size: 16px;"));
    // Soft drop shadow for depth.
    auto *shadow = new QGraphicsDropShadowEffect(dot);
    shadow->setBlurRadius(4);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(0, 0, 0, 40));
    dot->setGraphicsEffect(shadow);
    return dot;
}

void setDotColor(QLabel *dot, const QString &colorHex)
{
    if (dot == nullptr) return;
    dot->style()->unpolish(dot);
    dot->setStyleSheet(QStringLiteral("color: %1; font-size: 16px;").arg(colorHex));
    dot->style()->polish(dot);
}

QWidget *wrapStatusLabel(QLabel *label, QWidget *parent)
{
    auto *wrapper = new QWidget(parent);
    auto *layout = new QHBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();
    return wrapper;
}

}

ServerManagerWidget::ServerManagerWidget(ConfigStore *store, QWidget *parent, bool showPageHeader)
    : QWidget(parent)
    , m_store(store)
    , m_credentials(new CredentialStore(DataPaths::credentialsFile()))
    , m_sessionCache(new CredentialSessionCache())
{
    auto *layout = new QVBoxLayout(this);
    if (showPageHeader) {
        setProperty("cardStackPage", true);
        PageLayout::applyToolPage(layout);
    } else {
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(PageLayout::Space12);
    }

    auto *toolbarWidget = new QWidget(this);
    auto *toolbar = new QHBoxLayout(toolbarWidget);
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setSpacing(PageLayout::Space12);
    auto *addButton = new QPushButton(QStringLiteral("新建服务器"));
    addButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *editButton = new QPushButton(QStringLiteral("编辑"));
    editButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *monitorButton = new QPushButton(QStringLiteral("服务器监控"));
    monitorButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *remoteFilesButton = new QPushButton(QStringLiteral("远程文件"));
    remoteFilesButton->setObjectName(QStringLiteral("secondaryButton"));
    auto *deleteButton = new QPushButton(QStringLiteral("删除"));
    deleteButton->setObjectName(QStringLiteral("dangerButton"));
    auto *refreshButton = new QPushButton(QStringLiteral("刷新"));
    refreshButton->setObjectName(QStringLiteral("secondaryButton"));
    toolbar->addWidget(addButton);
    toolbar->addWidget(editButton);
    toolbar->addWidget(monitorButton);
    toolbar->addWidget(remoteFilesButton);
    toolbar->addWidget(deleteButton);
    toolbar->addWidget(refreshButton);
    toolbar->addStretch();
    layout->addWidget(toolbarWidget);

    m_table = new QTableWidget(0, 6);
    setupTable();

    auto *tableSection = PageLayout::wrapTableSection(
        m_table,
        &m_emptyState,
        QStringLiteral("暂无服务器。点击「新建服务器」添加第一台目标机器。"));
    auto *contentPanel = new QFrame(this);
    contentPanel->setObjectName(QStringLiteral("contentPanel"));
    contentPanel->setAttribute(Qt::WA_StyledBackground, true);
    auto *panelLayout = new QVBoxLayout(contentPanel);
    PageLayout::configureToolCard(panelLayout);
    PageLayout::applyLighterCardShadow(contentPanel);
    panelLayout->addWidget(tableSection, 1);
    layout->addWidget(contentPanel, 1);

    connect(addButton, &QPushButton::clicked, this, &ServerManagerWidget::addServer);
    connect(editButton, &QPushButton::clicked, this, &ServerManagerWidget::editServer);
    connect(monitorButton, &QPushButton::clicked, this, &ServerManagerWidget::openServerMonitor);
    connect(remoteFilesButton, &QPushButton::clicked, this, &ServerManagerWidget::browseRemoteFiles);
    connect(deleteButton, &QPushButton::clicked, this, &ServerManagerWidget::deleteServer);
   connect(refreshButton, &QPushButton::clicked, this, &ServerManagerWidget::reload);
    connect(refreshButton, &QPushButton::clicked, this, &ServerManagerWidget::pingServers);
   connect(m_table, &QTableWidget::doubleClicked, this, &ServerManagerWidget::editServer);

    reload();
}

void ServerManagerWidget::setupTable()
{
    m_table->setHorizontalHeaderLabels({
        QStringLiteral("ID"),
        QStringLiteral("名称"),
        QStringLiteral("状态"),
        QStringLiteral("系统"),
        QStringLiteral("地址"),
        QStringLiteral("默认目录")
    });
    m_table->verticalHeader()->setVisible(false);
    PageLayout::configureListingTable(m_table);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setDefaultSectionSize(40);
}

int ServerManagerWidget::serverCount() const
{
    return m_table->rowCount();
}

QStringList ServerManagerWidget::serverIds() const
{
    QStringList ids;
    for (int row = 0; row < m_table->rowCount(); ++row) {
        ids.append(m_table->item(row, 0)->text());
    }
    return ids;
}

QString ServerManagerWidget::selectedServerId() const
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return {};
    }
    return m_table->item(rows.first().row(), 0)->text();
}

QString ServerManagerWidget::credentialRefFor(const QJsonObject &server) const
{
    return server.value(QStringLiteral("auth")).toObject().value(QStringLiteral("credentialRef")).toString();
}

void ServerManagerWidget::persistCredentials(const QJsonObject &server, const ServerDialog &dialog)
{
    const QString ref = credentialRefFor(server);
    if (dialog.shouldClearStoredPassword()) {
        if (!ref.isEmpty()) {
            m_credentials->remove(ref);
        }
        return;
    }

    if (!dialog.shouldRememberPassword() || ref.isEmpty()) {
        return;
    }

    const QString password = dialog.pendingPassword();
    if (password.isEmpty()) {
        return;
    }

    QString error;
    if (!m_credentials->save(ref, password, &error)) {
        QMessageBox::warning(this, QStringLiteral("凭据保存失败"), error);
    }
}

void ServerManagerWidget::reload()
{
    QVector<StoredRecord> records;
    QString error;
    if (!m_store->listServers(&records, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    m_table->setRowCount(records.size());
    for (int row = 0; row < records.size(); ++row) {
        const StoredRecord &record = records.at(row);
        const QString os = record.config.value(QStringLiteral("os")).toString();
        const QString osLabel = os == QStringLiteral("windows") ? QStringLiteral("Windows WinRM") : QStringLiteral("Linux SSH");
        const QString endpoint = record.config.value(QStringLiteral("host")).toString()
            + QLatin1Char(':')
            + QString::number(record.config.value(QStringLiteral("port")).toInt());

       auto *statusDot = createStatusDot(m_table);
       m_table->setItem(row, 0, new QTableWidgetItem(record.id));
       m_table->setItem(row, 1, new QTableWidgetItem(record.config.value(QStringLiteral("name")).toString()));
        m_table->setCellWidget(row, 2, statusDot);
        m_table->setItem(row, 3, new QTableWidgetItem(osLabel));
        m_table->setItem(row, 4, new QTableWidgetItem(endpoint));
        m_table->setItem(row, 5, new QTableWidgetItem(record.config.value(QStringLiteral("defaultRemoteBaseDir")).toString()));
    }

   PageLayout::updateTableEmptyState(m_table, m_emptyState, records.size());

   if (!records.isEmpty()) {
       m_table->selectRow(0);
   }
}

void ServerManagerWidget::addServer()
{
    auto *dialog = new ServerDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setCredentialStore(m_credentials);
    dialog->setServer(QJsonObject{
        {QStringLiteral("id"), QStringLiteral("prod-linux-1")},
        {QStringLiteral("name"), QStringLiteral("Prod Linux 1")},
        {QStringLiteral("os"), QStringLiteral("linux")},
        {QStringLiteral("host"), QStringLiteral("192.168.1.10")},
        {QStringLiteral("port"), 22},
        {QStringLiteral("username"), QStringLiteral("root")},
        {QStringLiteral("auth"), QJsonObject{{QStringLiteral("mode"), QStringLiteral("manual")}}},
        {QStringLiteral("defaultRemoteBaseDir"), QStringLiteral("/home")},
        {QStringLiteral("ssh"), QJsonObject{{QStringLiteral("knownHostsPolicy"), QStringLiteral("strict-with-prompt")}}},
        {QStringLiteral("winrm"), QJsonValue::Null}
    }, false, false);

    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        QString error;
        const QJsonObject server = dialog->server();
        if (!m_store->upsertServer(server.value(QStringLiteral("id")).toString(), server, &error)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), error);
            return;
        }
        persistCredentials(server, *dialog);
        reload();
        emit serversChanged();
    });
    dialog->show();
}

void ServerManagerWidget::editServer()
{
    const QString id = selectedServerId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要编辑的服务器。"));
        return;
    }

    QJsonObject server;
    QString error;
    if (!m_store->getServer(id, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    const QString ref = credentialRefFor(server);
    auto *dialog = new ServerDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setCredentialStore(m_credentials);
    dialog->setServer(server, true, !ref.isEmpty() && m_credentials->has(ref));
    connect(dialog, &ServerDialog::connectionTestSucceeded, this, [this, id]() {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->item(row, 0)->text() == id) {
             auto *wrapper = m_table->cellWidget(row, 2);
              if (auto *label = qobject_cast<QLabel *>(wrapper)) {
                setDotColor(label, QStringLiteral("#00874D"));
               }
                break;
            }
        }
    });
    connect(dialog, &QDialog::accepted, this, [this, dialog, id]() {
        QString error;
        const QJsonObject updated = dialog->server();
        if (!m_store->upsertServer(id, updated, &error)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), error);
            return;
        }
        persistCredentials(updated, *dialog);
        reload();
        emit serversChanged();
    });
    dialog->show();
}

void ServerManagerWidget::openServerMonitor()
{
    const QString id = selectedServerId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择一台服务器。"));
        return;
    }

    QJsonObject server;
    QString error;
    if (!m_store->getServer(id, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    auto *dialog = new ServerMonitorDialog(
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache.get(), this),
        QJsonObject{},
        this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void ServerManagerWidget::browseRemoteFiles()
{
    const QString id = selectedServerId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择一台 Linux 服务器。"));
        return;
    }

    QJsonObject server;
    QString error;
    if (!m_store->getServer(id, &server, &error)) {
        QMessageBox::warning(this, QStringLiteral("加载失败"), error);
        return;
    }

    if (server.value(QStringLiteral("os")).toString() != QStringLiteral("linux")) {
        QMessageBox::information(this, QStringLiteral("暂不支持"), QStringLiteral("远程文件浏览当前仅支持 Linux 服务器。"));
        return;
    }

    const RemoteConnectionContext context =
        RemoteCredentialResolver::resolve(server, m_credentials, m_sessionCache.get(), this);
    const QString authMode = server.value(QStringLiteral("auth")).toObject().value(QStringLiteral("mode")).toString();
    if (authMode != QStringLiteral("ssh-key") && context.password.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("无法连接"), QStringLiteral("未获取到服务器密码，请先在编辑服务器时保存密码。"));
        return;
    }

    RemoteFileBrowserDialog *dialog = new RemoteFileBrowserDialog(context, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void ServerManagerWidget::deleteServer()
{
    const QString id = selectedServerId();
    if (id.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择"), QStringLiteral("请先选择要删除的服务器。"));
        return;
    }

    const auto answer = QMessageBox::question(this,
                                              QStringLiteral("确认删除"),
                                              QStringLiteral("确定删除服务器「%1」？此操作不可恢复。").arg(id));
    if (answer != QMessageBox::Yes) {
        return;
    }

    QJsonObject server;
    QString loadError;
    if (m_store->getServer(id, &server, &loadError)) {
        const QString ref = credentialRefFor(server);
        if (!ref.isEmpty()) {
            m_credentials->remove(ref);
        }
    }

    QString error;
    if (!m_store->deleteServer(id, &error)) {
        QMessageBox::warning(this, QStringLiteral("删除失败"), error);
        return;
    }

    reload();
    emit serversChanged();
}

void ServerManagerWidget::pingServers()
{
    const int rowCount = m_table->rowCount();
    if (rowCount == 0) {
        return;
    }

    QStringList ids;
    QStringList hosts;
    QList<int> ports;
    for (int row = 0; row < rowCount; ++row) {
        ids.append(m_table->item(row, 0)->text());
        const QString endpoint = m_table->item(row, 4)->text();
        const int colon = endpoint.lastIndexOf(QLatin1Char(':'));
        hosts.append(endpoint.left(colon));
        ports.append(endpoint.mid(colon + 1).toInt());
    }

    QPointer<ServerManagerWidget> guard(this);

    for (int i = 0; i < rowCount; ++i) {
        const QString host = hosts.at(i);
        const int port = ports.at(i);
        const QString id = ids.at(i);

        QtConcurrent::run([guard, host, port, id]() {
            QTcpSocket sock;
            sock.connectToHost(host, static_cast<quint16>(port));
            const bool ok = sock.waitForConnected(3000);
            sock.disconnectFromHost();

            if (guard) {
                QMetaObject::invokeMethod(guard.data(), [guard, id, ok]() {
                    if (!guard) return;
                    for (int row = 0; row < guard->m_table->rowCount(); ++row) {
                        if (guard->m_table->item(row, 0)->text() == id) {
                           if (auto *label = qobject_cast<QLabel *>(guard->m_table->cellWidget(row, 2))) {
                                setDotColor(label, ok ? QStringLiteral("#00874D") : QStringLiteral("#CB2634"));
                           }
                            break;
                        }
                    }
                }, Qt::QueuedConnection);
            }
        });
    }
}
