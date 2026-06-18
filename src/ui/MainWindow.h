#pragma once

#include "infra/ConfigStore.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"

#include <memory>

#include <QFrame>
#include <QMainWindow>
#include <QVector>

class QLabel;
class QListWidget;
class ProjectManagerWidget;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class ServerManagerWidget;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void refreshDashboard();
    void refreshDeploySelectors();
    void refreshDeploymentTables();
    void refreshDeployLogPath();
    void refreshLocalLogFiles();
    void refreshRemoteLogPathOptions();
    void refreshServiceStatus();
    void startDeployment();
    void onDeploymentFinished(bool ok, const QString &summary, const QString &logRelativePath);
    void viewDeploymentLog();
    void viewHistoryLog();
    void clearDeploymentHistory();
    void manageJdkProfiles();
    void saveSettings();

private:
    QWidget *createDashboardPage();
    QWidget *createDeployPage();
    QWidget *createHistoryPage();
    QWidget *createSettingsPage();
    QFrame *metricCard(const QString &title, QLabel **valueLabel) const;
    QTableWidget *createTable(const QStringList &headers, const QList<QStringList> &rows);
    void appendLog(const QString &stage, const QString &message);
    void openDeploymentLog(const QString &relativePath);
    void openRemoteDeploymentLog(const QString &remotePath);
    void applyRemoteLogPathOptions(const QStringList &options, const QString &preferred);
    QString selectedHistoryLogPath() const;
    void applyDeploymentMetrics(const QVector<StoredRecord> &deployments);
    void refreshDashboardTabData(const QVector<StoredRecord> *deployments = nullptr);
    void refreshJdkProfiles();

    std::unique_ptr<ConfigStore> m_store;
    std::unique_ptr<CredentialStore> m_credentials;
    std::unique_ptr<CredentialSessionCache> m_sessionCache;
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
    ProjectManagerWidget *m_projectManager = nullptr;
    ServerManagerWidget *m_serverManager = nullptr;
    QLabel *m_metricProjects = nullptr;
    QLabel *m_metricServers = nullptr;
    QLabel *m_metricRecentSuccess = nullptr;
    QLabel *m_metricPendingFailures = nullptr;
    QStackedWidget *m_dashboardStack = nullptr;
    QTableWidget *m_dashboardServerTable = nullptr;
    QTableWidget *m_dashboardLogTable = nullptr;
    QLabel *m_dashboardServersEmpty = nullptr;
    QLabel *m_dashboardLogsEmpty = nullptr;
    QComboBox *m_deployProject = nullptr;
    QComboBox *m_deployServer = nullptr;
    QComboBox *m_jdkSelector = nullptr;
    QPushButton *m_manageJdkButton = nullptr;
    QComboBox *m_logPathInput = nullptr;
    QPushButton *m_refreshLogListButton = nullptr;
    QPushButton *m_viewLogButton = nullptr;
    QLabel *m_serviceStatusLabel = nullptr;
    QPushButton *m_refreshServiceStatusButton = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QProgressBar *m_progress = nullptr;
    QPushButton *m_deployButton = nullptr;
    QLabel *m_recentDeploymentsEmpty = nullptr;
    QLabel *m_historyEmpty = nullptr;
    QTableWidget *m_recentTable = nullptr;
    QTableWidget *m_historyTable = nullptr;
    QPushButton *m_historyViewLogButton = nullptr;
    QPushButton *m_clearHistoryButton = nullptr;
    QLineEdit *m_configDirInput = nullptr;
    QLineEdit *m_mavenHomeInput = nullptr;
    QLineEdit *m_mavenRepoInput = nullptr;
    bool m_deployRunning = false;
    int m_remoteLogRefreshGeneration = 0;
};
