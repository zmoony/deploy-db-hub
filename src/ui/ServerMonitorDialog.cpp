#include "ui/ServerMonitorDialog.h"

#include "adapters/remote/RemoteMonitor.h"
#include "infra/AppBranding.h"
#include "ui/PageLayout.h"

#include <QAbstractItemView>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <QtConcurrent>

namespace {

class NumericTableItem final : public QTableWidgetItem
{
public:
    explicit NumericTableItem(double value, const QString &text)
        : QTableWidgetItem(text)
        , m_value(value)
    {
    }

    bool operator<(const QTableWidgetItem &other) const override
    {
        const auto *otherNumeric = dynamic_cast<const NumericTableItem *>(&other);
        if (otherNumeric != nullptr) {
            return m_value < otherNumeric->m_value;
        }
        return QTableWidgetItem::operator<(other);
    }

private:
    double m_value = 0.0;
};

QFrame *compactMetricTile(const QString &title, QLabel **valueOut, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName(QStringLiteral("metricCard"));
    frame->setMaximumHeight(72);
    frame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(PageLayout::Space16, PageLayout::Space12, PageLayout::Space16, PageLayout::Space12);
    layout->setSpacing(PageLayout::Space8);
    auto *titleLabel = new QLabel(title, frame);
    titleLabel->setObjectName(QStringLiteral("metricTitle"));
    auto *valueLabel = new QLabel(QStringLiteral("-"), frame);
    valueLabel->setObjectName(QStringLiteral("metricValue"));
    valueLabel->setWordWrap(true);
    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    if (valueOut != nullptr) {
        *valueOut = valueLabel;
    }
    return frame;
}

}

ServerMonitorDialog::ServerMonitorDialog(const RemoteConnectionContext &connectionContext,
                                         const QJsonObject &projectConfig,
                                         QWidget *parent)
    : QDialog(parent)
    , m_connectionContext(connectionContext)
    , m_serverConfig(connectionContext.serverConfig)
    , m_projectConfig(projectConfig)
    , m_monitor(createRemoteMonitor(connectionContext))
{
    const QString serverName = m_serverConfig.value(QStringLiteral("name")).toString();
    setWindowTitle(QStringLiteral("服务器监控 - %1").arg(serverName));
    setModal(false);
    PageLayout::applyRemoteToolDialog(this);
    resize(1100, 640);
    AppBranding::applyWindowIcon(this);
    buildUi();
    refreshAll();
}

void ServerMonitorDialog::buildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(PageLayout::Space24, PageLayout::Space24, PageLayout::Space24, PageLayout::Space24);
    layout->setSpacing(PageLayout::Space16);

    auto *metricsRow = new QHBoxLayout;
    metricsRow->setSpacing(PageLayout::Space16);
    metricsRow->addWidget(compactMetricTile(QStringLiteral("CPU"), &m_cpuValue, this), 1);
    metricsRow->addWidget(compactMetricTile(QStringLiteral("内存"), &m_memoryValue, this), 2);
    metricsRow->addWidget(compactMetricTile(QStringLiteral("磁盘"), &m_diskValue, this), 1);
    layout->addLayout(metricsRow, 0);

    if (!m_projectConfig.isEmpty()) {
        auto *serviceRow = new QHBoxLayout;
        serviceRow->setSpacing(PageLayout::Space12);
        auto *serviceLabel = new QLabel(QStringLiteral("服务"));
        serviceLabel->setFixedWidth(48);
        m_serviceStatusValue = new QLabel(QStringLiteral("-"));
        m_serviceStatusValue->setObjectName(QStringLiteral("serviceStatusBadge"));
        serviceRow->addWidget(serviceLabel);
        serviceRow->addWidget(m_serviceStatusValue, 1);
        layout->addLayout(serviceRow);
    }

    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(PageLayout::Space12);
    m_refreshButton = new QPushButton(QStringLiteral("刷新"));
    m_refreshButton->setObjectName(QStringLiteral("primaryButton"));
    m_killButton = new QPushButton(QStringLiteral("结束选中进程"));
    m_killButton->setObjectName(QStringLiteral("dangerButton"));
    m_processSearch = new QLineEdit;
    m_processSearch->setPlaceholderText(QStringLiteral("搜索 PID / 命令行"));
    PageLayout::configureFormInput(m_processSearch);
    connect(m_refreshButton, &QPushButton::clicked, this, &ServerMonitorDialog::refreshAll);
    connect(m_killButton, &QPushButton::clicked, this, &ServerMonitorDialog::killSelectedProcess);
    connect(m_processSearch, &QLineEdit::textChanged, this, &ServerMonitorDialog::applyProcessFilter);
    toolbar->addWidget(m_refreshButton);
    toolbar->addWidget(m_killButton);
    toolbar->addWidget(m_processSearch, 1);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_processTable = new QTableWidget(0, 6);
    m_processTable->setHorizontalHeaderLabels({
        QStringLiteral("PID"),
        QStringLiteral("进程"),
        QStringLiteral("用户"),
        QStringLiteral("命令行"),
        QStringLiteral("CPU %"),
        QStringLiteral("内存 %")
    });
    m_processTable->verticalHeader()->setVisible(false);
    m_processTable->setMinimumHeight(400);
    PageLayout::configureDataTable(m_processTable);
    m_processTable->setSortingEnabled(true);
    m_processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_processTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_processTable->horizontalHeader()->setStretchLastSection(false);
    m_processTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_processTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_processTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_processTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_processTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_processTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    layout->addWidget(m_processTable, 1);

    auto *closeButton = new QPushButton(QStringLiteral("关闭"));
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    auto *footer = new QHBoxLayout;
    footer->setContentsMargins(0, PageLayout::Space8, 0, 0);
    footer->setSpacing(PageLayout::Space12);
    footer->addStretch();
    footer->addWidget(closeButton);
    layout->addLayout(footer);
}

void ServerMonitorDialog::refreshAll()
{
    m_refreshButton->setEnabled(false);
    m_killButton->setEnabled(false);
    m_cpuValue->setText(QStringLiteral("..."));
    m_memoryValue->setText(QStringLiteral("..."));
    m_diskValue->setText(QStringLiteral("..."));

    const int generation = ++m_refreshGeneration;
    RemoteMonitor *monitor = m_monitor.get();
    const QJsonObject serverConfig = m_serverConfig;
    const QJsonObject projectConfig = m_projectConfig;
    const bool queryService = m_serviceStatusValue != nullptr && !m_projectConfig.isEmpty();

    auto future = QtConcurrent::run([monitor, serverConfig, projectConfig, queryService]() {
        RefreshData data;
        data.metrics = monitor->queryMetrics(serverConfig);
        data.processes = monitor->listTopProcesses(serverConfig, 20);
        if (queryService) {
            data.service = monitor->queryServiceStatus(serverConfig, projectConfig);
        }
        return data;
    });

    auto *watcher = new QFutureWatcher<RefreshData>(this);
    connect(watcher, &QFutureWatcher<RefreshData>::finished, this, [this, watcher, generation]() {
        if (generation != m_refreshGeneration) {
            watcher->deleteLater();
            return;
        }

        applyRefreshData(watcher->result());
        m_refreshButton->setEnabled(true);
        m_killButton->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void ServerMonitorDialog::applyRefreshData(const RefreshData &data)
{
    const ServerMetricsResult &metrics = data.metrics;
    if (!metrics.ok) {
        m_cpuValue->setText(QStringLiteral("异常"));
        const QString error = metrics.error.trimmed();
        m_memoryValue->setText(error.left(48));
        m_memoryValue->setToolTip(error);
        m_diskValue->setText(QStringLiteral("-"));
    } else {
        m_cpuValue->setText(QStringLiteral("%1%").arg(QString::number(metrics.cpuUsagePercent, 'f', 1)));
        m_memoryValue->setText(QStringLiteral("%1 / %2 MB").arg(metrics.memoryUsedMb).arg(metrics.memoryTotalMb));
        m_memoryValue->setToolTip({});
        m_diskValue->setText(QStringLiteral("%1 / %2 GB").arg(metrics.diskUsedGb).arg(metrics.diskTotalGb));
    }

    const ProcessListResult &processes = data.processes;
    m_processTable->setSortingEnabled(false);
    if (!processes.ok) {
        m_processTable->setRowCount(0);
    } else {
        m_processTable->setRowCount(processes.processes.size());
        for (int row = 0; row < processes.processes.size(); ++row) {
            const RemoteProcessEntry &entry = processes.processes.at(row);
            m_processTable->setItem(row, 0, new NumericTableItem(entry.pid, QString::number(entry.pid)));
            m_processTable->setItem(row, 1, new QTableWidgetItem(entry.name));
            m_processTable->setItem(row, 2, new QTableWidgetItem(entry.user));
            auto *commandItem = new QTableWidgetItem(entry.commandLine);
            commandItem->setToolTip(entry.commandLine);
            m_processTable->setItem(row, 3, commandItem);
            m_processTable->setItem(row, 4, new NumericTableItem(entry.cpuPercent, QString::number(entry.cpuPercent, 'f', 1)));
            m_processTable->setItem(row, 5, new NumericTableItem(entry.memoryPercent, QString::number(entry.memoryPercent, 'f', 1)));
        }
    }
    m_processTable->setSortingEnabled(true);
    applyProcessFilter();

    if (m_serviceStatusValue != nullptr && !m_projectConfig.isEmpty()) {
        const ServiceStatusResult &status = data.service;
        if (!status.ok) {
            m_serviceStatusValue->setText(status.error);
        } else {
            m_serviceStatusValue->setText(QStringLiteral("%1 - %2")
                                              .arg(serviceRunStateLabel(status.state), status.message));
        }
    }
}

void ServerMonitorDialog::applyProcessFilter()
{
    if (m_processTable == nullptr || m_processSearch == nullptr) {
        return;
    }

    const QString needle = m_processSearch->text().trimmed();
    for (int row = 0; row < m_processTable->rowCount(); ++row) {
        const QString pid = m_processTable->item(row, 0) != nullptr ? m_processTable->item(row, 0)->text() : QString();
        const QString command = m_processTable->item(row, 3) != nullptr ? m_processTable->item(row, 3)->text() : QString();
        const bool matched = needle.isEmpty()
            || pid.contains(needle, Qt::CaseInsensitive)
            || command.contains(needle, Qt::CaseInsensitive);
        m_processTable->setRowHidden(row, !matched);
    }
}

QString ServerMonitorDialog::selectedPid() const
{
    const auto rows = m_processTable->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        return {};
    }
    const QTableWidgetItem *item = m_processTable->item(rows.first().row(), 0);
    return item != nullptr ? item->text() : QString();
}

void ServerMonitorDialog::killSelectedProcess()
{
    const QString pidText = selectedPid();
    if (pidText.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("未选择进程"), QStringLiteral("请先在进程列表中选择要结束的进程。"));
        return;
    }

    const int pid = pidText.toInt();
    const auto answer = QMessageBox::question(this,
                                              QStringLiteral("确认结束进程"),
                                              QStringLiteral("确定结束 PID %1 ？此操作不可撤销。").arg(pid));
    if (answer != QMessageBox::Yes) {
        return;
    }

    const KillProcessResult result = m_monitor->killProcess(m_serverConfig, pid);
    if (!result.ok) {
        QMessageBox::warning(this, QStringLiteral("结束失败"), result.error);
        return;
    }

    QMessageBox::information(this,
                             QStringLiteral("已发送结束信号"),
                             QStringLiteral("已向 PID %1 发送结束请求。").arg(pid));
    refreshAll();
}
