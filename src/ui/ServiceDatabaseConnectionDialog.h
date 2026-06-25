#pragma once

#include "ui/ServiceNodeTypes.h"

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QPushButton;

class ServiceDatabaseConnectionDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceDatabaseConnectionDialog(ServiceProductKind product, QWidget *parent = nullptr);

    void setInstance(const QJsonObject &instance, bool editMode);
    QJsonObject instance() const;

private slots:
    void onTestConnection();
    void onAccept();

private:
    int defaultPort() const;
    QJsonObject buildNode() const;

    ServiceProductKind m_product = ServiceProductKind::PostgreSQL;
    bool m_editMode = false;
    QString m_existingId;

    QLineEdit *m_name = nullptr;
    QLineEdit *m_host = nullptr;
    QLineEdit *m_port = nullptr;
    QLineEdit *m_database = nullptr;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QPushButton *m_passwordVisibilityButton = nullptr;
    QPushButton *m_testConnectionButton = nullptr;
};
