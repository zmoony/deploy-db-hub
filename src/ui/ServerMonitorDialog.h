#pragma once

#include "adapters/remote/RemoteConnection.h"
#include "adapters/remote/RemoteMonitor.h"

#include <QDialog>
#include <QJsonObject>

#include <memory>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

class ServerMonitorDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServerMonitorDialog(const RemoteConnectionContext &connectionContext,
                                 const QJsonObject &projectConfig = {},
                                 QWidget *parent = nullptr);

private slots:
    void refreshAll();
    void killSelectedProcess();

private:
    struct RefreshData
    {
        ServerMetricsResult metrics;
        ProcessListResult processes;
        ServiceStatusResult service;
    };

    void buildUi();
    void applyRefreshData(const RefreshData &data);
    void refreshProcesses();
    void applyProcessFilter();
    QString selectedPid() const;

    RemoteConnectionContext m_connectionContext;
    QJsonObject m_serverConfig;
    QJsonObject m_projectConfig;
    std::unique_ptr<RemoteMonitor> m_monitor;
    int m_refreshGeneration = 0;

    QLabel *m_cpuValue = nullptr;
    QLabel *m_memoryValue = nullptr;
    QLabel *m_diskValue = nullptr;
    QLabel *m_serviceStatusValue = nullptr;
    QLineEdit *m_processSearch = nullptr;
    QTableWidget *m_processTable = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_killButton = nullptr;
};
