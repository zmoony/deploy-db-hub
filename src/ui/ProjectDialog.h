#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QVector>

#include "infra/ConfigStore.h"

class QComboBox;
class QLineEdit;
class PathPickerWidget;
class QStackedWidget;

class ProjectDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(const QVector<StoredRecord> &servers, QWidget *parent = nullptr);

    void setProject(const QJsonObject &project, bool editMode);
    QJsonObject project() const;

private slots:
    void onSourceKindChanged(int index);
    void onAccept();

private:
    void buildUi();
    void syncSourceFields();
    void syncSourceStackHeight();
    void syncBuildModeFields();

    QVector<StoredRecord> m_servers;
    bool m_editMode = false;

    QLineEdit *m_id = nullptr;
    QLineEdit *m_name = nullptr;
    QComboBox *m_type = nullptr;
    QComboBox *m_sourceKind = nullptr;
    QStackedWidget *m_sourceStack = nullptr;
    PathPickerWidget *m_localPath = nullptr;
    QLineEdit *m_repoUrl = nullptr;
    QLineEdit *m_ref = nullptr;
    QLineEdit *m_command = nullptr;
    QComboBox *m_buildMode = nullptr;
    PathPickerWidget *m_artifactPath = nullptr;
    PathPickerWidget *m_prebuiltJarPath = nullptr;
    PathPickerWidget *m_workingDir = nullptr;
    QComboBox *m_serverId = nullptr;
    QLineEdit *m_remoteBaseDir = nullptr;
    QLineEdit *m_logDir = nullptr;
    PathPickerWidget *m_restartScript = nullptr;
    QLineEdit *m_serviceMatch = nullptr;
    QLineEdit *m_startCommand = nullptr;
    QLineEdit *m_stopCommand = nullptr;
    QLineEdit *m_restartCommand = nullptr;
    QLineEdit *m_statusCommand = nullptr;
    QLineEdit *m_targetJarPath = nullptr;
    QComboBox *m_backupPolicy = nullptr;
    QLineEdit *m_backupDir = nullptr;
    QComboBox *m_failureStrategy = nullptr;
};
