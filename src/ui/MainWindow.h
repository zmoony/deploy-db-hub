#pragma once

#include "infra/AppSettingsStore.h"
#include "infra/AiSettingsStore.h"
#include "infra/ConfigStore.h"
#include "infra/CredentialSessionCache.h"
#include "infra/CredentialStore.h"

#include <memory>

#include <QFrame>
#include <QList>
#include <QMainWindow>
#include <QPair>
#include <QVector>

#include <functional>

class QLabel;
class QListWidget;
class BigDataManagerWidget;
class CommonToolsWidget;
class DatabaseManagerWidget;
class ProjectManagerWidget;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QButtonGroup;
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
    void saveGlobalJdbcSettings();
    void onModuleChanged(int index);
    void onNavigationChanged(int row);
    void onSettingsClicked();
    void openAiConfigPage();

private:
    QWidget *createDashboardPage();
    QWidget *createDeployPage();
    QWidget *createHistoryPage();
    QWidget *createDeploySettingsPage();
    QWidget *createGlobalSettingsPage();
    void addModule(const QString &title, const QList<QPair<QString, QWidget *>> &pages);
    void addModuleFromPages(const QString &title, const QStringList &labels, const QList<QWidget *> &pages);
    void showModule(int index);
    void showMainModuleContent();
    void navigateToPage(int moduleIndex, int pageRow, int dashboardTabIndex = -1);
    QFrame *metricCard(const QString &title, QLabel **valueLabel) const;
    QTableWidget *createTable(const QStringList &headers, const QList<QStringList> &rows);
    void appendLog(const QString &stage, const QString &message);
    void openDeploymentLog(const QString &relativePath);
    void openRemoteDeploymentLog(const QString &remotePath);
    void applyRemoteLogPathOptions(const QStringList &options, const QString &preferred);
    QString selectedHistoryLogPath() const;
    void refreshDashboardTabData(const QVector<StoredRecord> *deployments = nullptr);
    void refreshJdkProfiles();
    void refreshLocalLogFiles(bool allowRemotePrompt);
    void refreshRemoteLogPathOptions(bool allowPrompt);
    void refreshServiceStatus(bool allowPrompt);
    void refreshAiSettingsSummary();

    std::unique_ptr<ConfigStore> m_store;
    std::unique_ptr<CredentialStore> m_credentials;
    std::unique_ptr<AiSettingsStore> m_aiSettings;
    std::unique_ptr<CredentialSessionCache> m_sessionCache;
    QStackedWidget *m_moduleStack = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QWidget *m_settingsPage = nullptr;
    QListWidget *m_navigation = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QButtonGroup *m_moduleTabGroup = nullptr;
    QVector<QStackedWidget *> m_modulePages;
    QStringList m_moduleTitles;
    QVector<QStringList> m_moduleNavigationLabels;
    CommonToolsWidget *m_commonTools = nullptr;
    ProjectManagerWidget *m_projectManager = nullptr;
    ServerManagerWidget *m_serverManager = nullptr;
    BigDataManagerWidget *m_bigDataManager = nullptr;
    DatabaseManagerWidget *m_databaseManager = nullptr;
    QButtonGroup *m_dashboardFilterGroup = nullptr;
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
    QLineEdit *m_postgresDriverJarInput = nullptr;
    QLineEdit *m_oracleDriverJarInput = nullptr;
    QLabel *m_driverProbeLabel = nullptr;
    QLineEdit *m_aiSummaryUrl = nullptr;
    QLineEdit *m_aiSummaryModel = nullptr;
    QLabel *m_aiSummaryKeyStatus = nullptr;
    bool m_deployRunning = false;
    int m_remoteLogRefreshGeneration = 0;
};
