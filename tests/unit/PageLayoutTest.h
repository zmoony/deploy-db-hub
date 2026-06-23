#pragma once

#include <QObject>

class PageLayoutTest final : public QObject
{
    Q_OBJECT

private slots:
    void fitWindowToScreenCentersWindow();
    void pageTransferDoesNotShowDetachedPages();
};
