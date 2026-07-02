#pragma once

#include "infra/AiSettingsStore.h"

#include <QDialog>
#include <QVector>

class QLabel;
class QListWidget;
class QPushButton;
class CredentialStore;

class AiConnectionManagerDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit AiConnectionManagerDialog(CredentialStore *credentials,
                                      QWidget *parent = nullptr);

    QVector<AiConnection> connections() const { return m_workingConnections; }
    QString activeConnectionId() const { return m_workingActiveId; }
    QStringList removedCredentialRefs() const { return m_removedCredentialRefs; }

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSetActive();
    void onTest();
    void onCurrentChanged(int row);

public:
    void accept() override;

private:
    void buildUi();
    void reloadList();
    void persist();
    void applyAiSettings(AiSettings *settings) const;

    CredentialStore *m_credentials = nullptr;
    QVector<AiConnection> m_workingConnections;
    QString m_workingActiveId;
    QStringList m_removedCredentialRefs;

    QListWidget *m_list = nullptr;
    QLabel *m_detail = nullptr;
    QLabel *m_message = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_setActiveButton = nullptr;
    QPushButton *m_testButton = nullptr;
};
