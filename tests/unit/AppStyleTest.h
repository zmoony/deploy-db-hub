#pragma once

#include <QObject>

class AppStyleTest final : public QObject
{
    Q_OBJECT

private slots:
    void comboBoxTextIsNotPaintedTwice();
    void sidebarStyleUsesSidebarPaletteSlots();
};
