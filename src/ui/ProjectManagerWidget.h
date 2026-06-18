#pragma once

#include <QWidget>

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

signals:
    void projectsChanged();

private slots:
    void reload();
    void addProject();
    void editProject();
    void deleteProject();
    void refreshSelectedServiceStatus();
    void startSelectedService();
    void stopSelectedService();

private:
    QString selectedProjectId() const;
    void setupTable();
    void runServiceOperation(const QString &operation);
    static QString sourceSummary(const QJsonObject &project);

    ConfigStore *m_store = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_emptyState = nullptr;
};
