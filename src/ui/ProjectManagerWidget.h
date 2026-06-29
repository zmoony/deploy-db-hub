#pragma once

#include "infra/ConfigStore.h"

#include <QWidget>

class QComboBox;
class QFrame;
class QJsonObject;
class QLabel;
class QListWidget;
class QListWidgetItem;
class ConfigStore;

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
    void setupList();
    void buildDetailCard();
    void runServiceOperation(const QString &operation);
    void refreshGroupFilter(const QVector<StoredRecord> &records);
    void populateList(const QVector<StoredRecord> &records);
    void refreshDetailCard();
    void setServiceStatusLabel(const QString &status, const QString &pid);
    static QString sourceSummary(const QJsonObject &project);
    static bool isGroupHeaderItem(const QListWidgetItem *item);

    ConfigStore *m_store = nullptr;
    QComboBox *m_groupFilter = nullptr;
    QListWidget *m_projectList = nullptr;
    QLabel *m_emptyState = nullptr;

    QFrame *m_detailCard = nullptr;
    QWidget *m_detailContent = nullptr;
    QLabel *m_detailEmptyState = nullptr;
    QLabel *m_nameLabel = nullptr;
    QLabel *m_typeValue = nullptr;
    QLabel *m_sourceValue = nullptr;
    QLabel *m_serverValue = nullptr;
    QLabel *m_groupValue = nullptr;
    QLabel *m_strategyValue = nullptr;
    QLabel *m_statusValue = nullptr;
    QLabel *m_pidValue = nullptr;
};
