#pragma once

#include <QObject>

class RemoteOutputCleanerTest final : public QObject
{
    Q_OBJECT

private slots:
    void stripSshBannerRemovesLoginWarning();
    void normalizeRemoteErrorMapsPermissionDenied();
};
