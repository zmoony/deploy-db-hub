#pragma once

#include "adapters/services/ServiceBroker.h"
#include "ui/ServiceNodeTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>

#include <QVector>

class ConfigStore;
class CredentialSessionCache;
class CredentialStore;
class QButtonGroup;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QStackedWidget;
class QTableWidget;

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
    void onRemoteDataLoaded();

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

    ServiceProductKind m_product;
    ConfigStore *m_store = nullptr;
    CredentialStore *m_credentials = nullptr;
    CredentialSessionCache *m_sessionCache = nullptr;
    QVector<QJsonObject> m_instances;
    int m_currentInstanceIndex = -1;
    int m_loadGeneration = 0;

    QStackedWidget *m_stack = nullptr;
    QTableWidget *m_instanceTable = nullptr;
    QLabel *m_listEmptyState = nullptr;

    QWidget *m_detailPage = nullptr;
    QLabel *m_bannerTitle = nullptr;
    QLabel *m_bannerStatus = nullptr;
    QLabel *m_bannerNode = nullptr;
    QStackedWidget *m_detailTabStack = nullptr;
    QButtonGroup *m_detailTabGroup = nullptr;
    QVector<DetailTabSpec> m_detailTabs;
    QTableWidget *m_detailTable = nullptr;
    QLabel *m_detailEmptyState = nullptr;
    QWidget *m_detailToolbar = nullptr;
    QHBoxLayout *m_detailToolbarLayout = nullptr;
    QComboBox *m_schemaCombo = nullptr;
    int m_currentDetailTab = 0;
};
