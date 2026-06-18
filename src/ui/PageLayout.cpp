#include "ui/PageLayout.h"

#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QScreen>
#include <QSizePolicy>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

void configureStaticLabel(QLabel *label)
{
    if (label != nullptr) {
        label->setTextInteractionFlags(Qt::NoTextInteraction);
    }
}

QRect availableScreenGeometry(QWidget *window)
{
    QScreen *screen = window != nullptr ? window->screen() : nullptr;
    if (screen == nullptr) {
        screen = QGuiApplication::primaryScreen();
    }
    return screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1280, 720);
}

}

namespace PageLayout {

void applyPage(QVBoxLayout *layout)
{
    layout->setContentsMargins(Space16, Space16, Space16, Space16);
    layout->setSpacing(Space16);
}

void applyDialog(QVBoxLayout *layout)
{
    layout->setContentsMargins(Space24, Space24, Space24, Space24);
    layout->setSpacing(Space24);
}

void applyForm(QFormLayout *form)
{
    form->setSpacing(Space12);
    form->setContentsMargins(Space8, Space12, Space8, Space8);
}

void applyDialogForm(QFormLayout *form)
{
    applyForm(form);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setRowWrapPolicy(QFormLayout::DontWrapRows);
}

void applyInlineForm(QFormLayout *form)
{
    applyForm(form);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
}

void applyModalDialog(QDialog *dialog)
{
    if (dialog == nullptr) {
        return;
    }
    enableResizableWindow(dialog);
    dialog->setMinimumSize(DialogMinWidth, DialogMinHeight);
    fitWindowToScreen(dialog, DialogDefaultWidth, DialogDefaultHeight);
}

void applyRemoteToolDialog(QDialog *dialog,
                           int minWidth,
                           int minHeight,
                           int preferredWidth,
                           int preferredHeight)
{
    if (dialog == nullptr) {
        return;
    }
    enableResizableWindow(dialog);
    dialog->setMinimumSize(minWidth, minHeight);
    fitWindowToScreen(dialog, preferredWidth, preferredHeight);
}

void fitWindowToScreen(QWidget *window, int preferredWidth, int preferredHeight, qreal maxScreenRatio)
{
    if (window == nullptr) {
        return;
    }

    const QRect available = availableScreenGeometry(window);
    const int maxW = qMax(640, static_cast<int>(available.width() * maxScreenRatio));
    const int maxH = qMax(460, static_cast<int>(available.height() * maxScreenRatio));

    const int width = qBound(640, preferredWidth, maxW);
    const int height = qBound(460, preferredHeight, maxH);
    window->resize(width, height);

    const QSize minSize = window->minimumSize();
    if (minSize.width() > maxW) {
        window->setMinimumWidth(maxW);
    }
    if (minSize.height() > maxH) {
        window->setMinimumHeight(maxH);
    }
}

void enableResizableWindow(QWidget *window)
{
    if (window == nullptr) {
        return;
    }

    Qt::WindowFlags flags = window->windowFlags();
    flags &= ~Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::WindowSystemMenuHint;
    window->setWindowFlags(flags);
    window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (auto *dialog = qobject_cast<QDialog *>(window)) {
        dialog->setSizeGripEnabled(true);
    }
}

void configureFormInput(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }
    widget->setMinimumHeight(DialogFieldHeight);
    widget->setMinimumWidth(DialogFieldMinWidth);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        combo->setMinimumContentsLength(24);
        combo->setMaxVisibleItems(12);
        if (combo->view() != nullptr) {
            combo->view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            combo->view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    if (auto *spin = qobject_cast<QSpinBox *>(widget)) {
        spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        spin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        spin->setAccelerated(true);
    }
}

void configurePathField(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }
    widget->setMinimumHeight(DialogFieldHeight);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

QWidget *wrapScrollableBody(QVBoxLayout *dialogLayout)
{
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *content = new QWidget;
    scroll->setWidget(content);
    dialogLayout->addWidget(scroll, 1);
    return content;
}

void applyMetricCard(QVBoxLayout *layout)
{
    layout->setContentsMargins(Space16, Space16, Space16, Space16);
    layout->setSpacing(Space8);
}

QLabel *makePageTitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("pageTitle"));
    configureStaticLabel(label);
    return label;
}

QLabel *makePageSubtitle(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("pageSubtitle"));
    label->setWordWrap(true);
    configureStaticLabel(label);
    return label;
}

QLabel *makeSectionLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("sectionLabel"));
    configureStaticLabel(label);
    return label;
}

void configureDataTable(QTableWidget *table)
{
    if (table == nullptr) {
        return;
    }
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

QWidget *makeHeaderBlock(const QString &title, const QString &subtitle, QWidget *parent)
{
    auto *block = new QWidget(parent);
    auto *layout = new QVBoxLayout(block);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Space8);
    layout->addWidget(makePageTitle(title, block));
    if (!subtitle.isEmpty()) {
        layout->addWidget(makePageSubtitle(subtitle, block));
    }
    return block;
}

QWidget *wrapContentPanel(QWidget *page)
{
    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("contentPanel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);
    panelLayout->addWidget(page);
    return panel;
}

QWidget *wrapTableSection(QTableWidget *table, QLabel **emptyStateOut, const QString &emptyMessage)
{
    auto *section = new QWidget;
    auto *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *emptyState = new QLabel(emptyMessage, section);
    emptyState->setObjectName(QStringLiteral("emptyState"));
    emptyState->setAlignment(Qt::AlignCenter);
    emptyState->setWordWrap(true);
    configureStaticLabel(emptyState);
    layout->addWidget(emptyState, 1);

    configureDataTable(table);
    table->setMinimumHeight(200);
    layout->addWidget(table, 1);

    if (emptyStateOut != nullptr) {
        *emptyStateOut = emptyState;
    }

    updateTableEmptyState(table, emptyState, 0);
    return section;
}

void updateTableEmptyState(QTableWidget *table, QLabel *emptyState, int rowCount)
{
    const bool hasRows = rowCount > 0;
    if (emptyState != nullptr) {
        emptyState->setVisible(!hasRows);
    }
    if (table != nullptr) {
        table->setVisible(hasRows);
    }
}

}
