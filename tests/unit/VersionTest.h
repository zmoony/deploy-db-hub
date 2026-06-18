#pragma once

#include <QObject>

class VersionTest final : public QObject
{
    Q_OBJECT

private slots:
    void formatsVersionTimestamp();
    void rejectsInvalidVersionTimestamp();
};
