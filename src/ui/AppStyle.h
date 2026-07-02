#pragma once

#include "ui/Theme.h"

class QApplication;
struct ThemePalette;

namespace AppStyle {
void apply(QApplication &app, ThemeMode mode = ThemeMode::System);
void reapply(QApplication &app, ThemeMode mode);
const ThemePalette &currentPalette();
}
