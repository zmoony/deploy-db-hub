#pragma once

#include <QApplication>
#include <QClipboard>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QWidget>

namespace Ui {
namespace Tools {
namespace Helpers {

inline QPushButton *makeToolButton(const QString &text, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(QStringLiteral("toolBarButton"));
    button->setMinimumHeight(28);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return button;
}

inline void copyToClipboard(const QString &text)
{
    if (QClipboard *clipboard = QApplication::clipboard()) {
        clipboard->setText(text);
    }
}

} // namespace Helpers
} // namespace Tools
} // namespace Ui
