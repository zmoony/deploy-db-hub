#pragma once

#include <QObject>

class LogPathTest final : public QObject
{
    Q_OBJECT

private slots:
    void buildsRelativePath();
    void resolvesAbsolutePath();
    void rejectsInvalidDeployId();
    void rejectsInvalidRelativePath();
};
