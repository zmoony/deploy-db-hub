#pragma once

#include <QDialog>

class DeploymentLogDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit DeploymentLogDialog(const QString &relativePath, QWidget *parent = nullptr);

    static bool canOpen(const QString &relativePath);
    static QString loadContent(const QString &relativePath, QString *error);

private:
    void buildUi(const QString &relativePath, const QString &content);
};
