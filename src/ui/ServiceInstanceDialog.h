#pragma once

#include "ui/ServiceNodeTypes.h"

#include <QDialog>
#include <QJsonObject>

class QLineEdit;

class ServiceInstanceDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceInstanceDialog(ServiceProductKind product, QWidget *parent = nullptr);

    void setInstance(const QJsonObject &instance, bool editMode);
    QJsonObject instance() const;

private slots:
    void onAccept();

private:
    ServiceProductKind m_product = ServiceProductKind::Kafka;
    bool m_editMode = false;
    QLineEdit *m_id = nullptr;
    QLineEdit *m_name = nullptr;
};
