#pragma once

#include "adapters/services/ServiceBroker.h"
#include "ui/ServiceNodeTypes.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QWidget>

#include <QVector>

class ConfigStore;
class CredentialSessionCache;
class CredentialStore;
class LineTabBarController;
class QButtonGroup;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class ServiceSqlWorkbenchWidget;

class ServiceProductPanel final : public QWidget
{
    Q_OBJECT

public:
    struct DetailTabSpec {
        QString title;
        QStringList tableHeaders;
        QStringList toolbarActions;
    };

    ServiceProductPanel(ServiceProductKind product,
                        ConfigStore *store,
                        CredentialStore *credentials,
                        CredentialSessionCache *sessionCache,
                        QWidget *parent = nullptr);

private slots:
    void reloadInstanceList();
    void addInstance();
    void editInstance();
    void deleteInstance();
    void openSelectedInstance();
    void backToList();
    void addNode();
    void editNode();
    void deleteNode();
    void onDetailTabChanged(int index);
    void onDetailAction();
    void onDetailRowActivated(int row, int column);
    void onDetailCellClicked(int row, int column);
    void onRemoteDataLoaded();
    void startKafkaTopicStatsLoad(int generation, int detailTab, const QVector<QJsonObject> &knownTopics = {});
    void mergeKafkaTopicStats(const QVector<QJsonObject> &rows);

private:
    QWidget *buildListPage();
    QWidget *buildDetailPage();
    QWidget *makeInstanceBanner();
    void refreshDetailTable();
    void refreshNodeTable();
    void refreshBanner();
    void persistInstances();
    void loadInstancesFromStore();
    void loadRemoteDetailAsync();
    void applyDetailRows(const QVector<QJsonObject> &rows, const QStringList &columns);
    void applyElasticsearchIndexRows();
    void toggleElasticsearchIndexGroup(const QString &groupKey);
    void setElasticsearchStatusCell(int row, const QString &status);
    void restoreDetailTabFromCache(int index);
    void showDetailLoading(const QString &message = QStringLiteral("正在加载…"));
    void runDetailRowAction(int row, const QString &action);
    void setOperationCell(int row, const QJsonObject &rowData);
    bool currentInstanceContext(QJsonObject *instance, QJsonObject *server) const;
    QJsonObject selectedRowData() const;
    int selectedInstanceIndex() const;
    int selectedNodeRow() const;
    int selectedDetailRow() const;
    QJsonArray nodesForCurrentInstance() const;
    void saveNodesForCurrentInstance(const QJsonArray &nodes);
    void saveCurrentInstance(const QJsonObject &instance);
    QString primaryNodeHost() const;
    QString currentSchema() const;
    ServiceBroker::TabKind currentTabKind() const;
    void updateSchemaCombo();
    void refreshSqlWorkbench();
    void executeSqlQuery(const QString &sql);
    void updateSqlConnectionSummary();

    ServiceProductKind m_product;
    ConfigStore *m_store = nullptr;
    CredentialStore *m_credentials = nullptr;
    CredentialSessionCache *m_sessionCache = nullptr;
    QVector<QJsonObject> m_instances;
    int m_currentInstanceIndex = -1;
    int m_loadGeneration = 0;
    int m_bannerGeneration = 0;

    QStackedWidget *m_stack = nullptr;
    QTableWidget *m_instanceTable = nullptr;
    QLabel *m_listEmptyState = nullptr;

    QWidget *m_detailPage = nullptr;
    QPushButton *m_backNavButton = nullptr;
    QLabel *m_bannerTitle = nullptr;
    QLabel *m_bannerStatus = nullptr;
    QLabel *m_bannerNode = nullptr;
    QStackedWidget *m_detailTabStack = nullptr;
    LineTabBarController *m_detailTabController = nullptr;
    QVector<DetailTabSpec> m_detailTabs;
    QTableWidget *m_detailTable = nullptr;
    QLabel *m_detailEmptyState = nullptr;
    QStackedWidget *m_detailContentStack = nullptr;
    QWidget *m_detailTablePage = nullptr;
    ServiceSqlWorkbenchWidget *m_sqlWorkbench = nullptr;
    QWidget *m_detailToolbar = nullptr;
    QHBoxLayout *m_detailToolbarLayout = nullptr;
    QComboBox *m_schemaCombo = nullptr;
    QString m_activeSchema;
    int m_currentDetailTab = 0;
    QHash<int, QVector<QJsonObject>> m_detailTabCache;
    QVector<QJsonObject> m_esIndexRowsCache;
    QSet<QString> m_expandedEsIndexGroups;
    QVector<QJsonObject> m_kafkaGroupPartitions;
};
