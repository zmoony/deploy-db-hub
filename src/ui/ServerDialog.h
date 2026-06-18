#pragma once

#include <QDialog>
#include <QJsonObject>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QStackedWidget;
class PathPickerWidget;

class ServerDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServerDialog(QWidget *parent = nullptr);

    void setServer(const QJsonObject &server, bool editMode, bool hasStoredPassword = false);
    QJsonObject server() const;

    QString pendingPassword() const;
    bool shouldRememberPassword() const;
    bool shouldClearStoredPassword() const;

private slots:
    void onOsChanged(int index);
    void onAuthModeChanged(int index);
    void browseRemoteBaseDir();
    void onAccept();

private:
    void buildUi();
    void syncOsFields();
    void syncAuthFields();
    QJsonObject draftServerConfig() const;

    bool m_editMode = false;
    bool m_hasStoredPassword = false;
    QJsonObject m_sourceServer;

    QLineEdit *m_id = nullptr;
    QLineEdit *m_name = nullptr;
    QComboBox *m_os = nullptr;
    QLineEdit *m_host = nullptr;
    QSpinBox *m_port = nullptr;
    QLineEdit *m_username = nullptr;
    QComboBox *m_authMode = nullptr;
    QLineEdit *m_password = nullptr;
    QCheckBox *m_rememberPassword = nullptr;
    PathPickerWidget *m_sshKeyPath = nullptr;
    QWidget *m_sshKeyRow = nullptr;
    PathPickerWidget *m_defaultRemoteBaseDir = nullptr;
    QStackedWidget *m_osStack = nullptr;
    QStackedWidget *m_authStack = nullptr;
    QComboBox *m_winrmScheme = nullptr;
};
