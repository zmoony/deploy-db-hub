#pragma once

#include "infra/JdkProfileStore.h"

#include <QDialog>
#include <QVector>

class QLineEdit;
class QTableWidget;

class JdkProfileDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit JdkProfileDialog(QWidget *parent = nullptr);

    QVector<JdkProfile> profiles() const;

private slots:
    void addProfile();
    void editProfile();
    void removeProfile();

private:
    void buildUi();
    void populateTable();
    int selectedRow() const;
    QString makeId(const QString &version, const QString &path) const;

    QVector<JdkProfile> m_profiles;
    QTableWidget *m_table = nullptr;
};
