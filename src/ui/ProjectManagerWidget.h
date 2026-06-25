#pragma once

#include "infra/ConfigStore.h"

#include <QWidget>

class QComboBox;
class QJsonObject;
class ConfigStore;
class QLabel;
class QTableWidget;

class ProjectManagerWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectManagerWidget(ConfigStore *store, QWidget *parent = nullptr);

    int projectCount() const;
    QStringList projectIds() const;
    static QJsonObject makeQuickAddDraft(const QJsonObject &sourceProject);

signals:
    void projectsChanged();

private slots:
    void reload();
    void addProject();
    void quickAddProject();
    void editProject();
    void deleteProject();
    void refreshSelectedServiceStatus();
    void startSelectedService();
    void stopSelectedService();
    void viewSelectedProjectLog();

private:
    QString selectedProjectId() const;
    void setupTable();
    void runServiceOperation(const QString &operation);
    void refreshGroupFilter(const QVector<StoredRecord> &records);
    void populateTable(const QVector<StoredRecord> &records);
    static QString sourceSummary(const QJsonObject &project);
    static bool isGroupHeaderRow(const QTableWidget *table, int row);

    ConfigStore *m_store = nullptr;
    QComboBox *m_groupFilter = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_emptyState = nullptr;
};
