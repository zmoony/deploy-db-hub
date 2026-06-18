#pragma once

#include <QObject>

class RemoteFileBrowserTest final : public QObject
{
    Q_OBJECT

private slots:
    void stubReadFileTailReturnsLastRequestedLines();
};
