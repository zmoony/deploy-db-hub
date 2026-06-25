#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QVector>

#include "infra/ConfigStore.h"

class QComboBox;
class QLineEdit;
class PathPickerWidget;
class QStackedWidget;
class QWidget;
class QLabel;

class ProjectDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ProjectDialog(const QVector<StoredRecord> &servers, QWidget *parent = nullptr);

    void setProject(const QJsonObject &project, bool editMode);
    QJsonObject project() const;

private slots:
    void onSourceKindChanged(int index);
    void onProjectTypeChanged(int index);
    void onAccept();

private:
    void buildUi();
    void syncSourceFields();
    void syncSourceStackHeight();
    void syncBuildModeFields();
    void syncRestartModePanel();
    void syncProjectTypeFields();
    bool isServiceCommandMode() const;
    bool isFrontendStatic() const;
    bool isJavaMaven() const;
    QString currentProjectType() const;

    QVector<StoredRecord> m_servers;
    bool m_editMode = false;

    QLabel *m_buildModeLabel = nullptr;
    QLabel *m_artifactPathLabel = nullptr;
    QLabel *m_prebuiltJarPathLabel = nullptr;
    QLabel *m_targetJarPathLabel = nullptr;
    QLabel *m_backupPolicyLabel = nullptr;
    QLabel *m_backupDirLabel = nullptr;
    QLabel *m_logDirLabel = nullptr;
    QLabel *m_artifactRenameLabel = nullptr;
    QWidget *m_restartModeBox = nullptr;

    QLineEdit *m_id = nullptr;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_group = nullptr;
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
    QLineEdit *m_artifactRename = nullptr;
    QComboBox *m_serverId = nullptr;
    QLineEdit *m_remoteBaseDir = nullptr;
    QLineEdit *m_logDir = nullptr;
    QComboBox *m_restartMode = nullptr;
    QStackedWidget *m_restartModeStack = nullptr;
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
