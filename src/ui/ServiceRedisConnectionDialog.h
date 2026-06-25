#pragma once

#include <QDialog>
#include <QJsonObject>

class QLineEdit;
class QPushButton;
class QSpinBox;

class ServiceRedisConnectionDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceRedisConnectionDialog(QWidget *parent = nullptr);

    void setInstance(const QJsonObject &instance, bool editMode);
    QJsonObject instance() const;

private slots:
    void onTestConnection();
    void onAccept();

private:
    QJsonObject buildNode() const;

    bool m_editMode = false;
    QString m_existingId;

    QLineEdit *m_name = nullptr;
    QLineEdit *m_host = nullptr;
    QLineEdit *m_port = nullptr;
    QLineEdit *m_username = nullptr;
    QLineEdit *m_password = nullptr;
    QSpinBox *m_database = nullptr;
    QPushButton *m_passwordVisibilityButton = nullptr;
    QPushButton *m_testConnectionButton = nullptr;
};
