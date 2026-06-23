#include "ui/PageLayout.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QMainWindow>
#include <QLabel>
#include <QListWidget>
#include <QScrollArea>
#include <QSpinBox>
#include <QScreen>
#include <QSizePolicy>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

class SidebarNavigationList final : public QListWidget
{
public:
    explicit SidebarNavigationList(QWidget *parent = nullptr)
        : QListWidget(parent)
    {
        setObjectName(QStringLiteral("sidebarNav"));
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setFrameShape(QFrame::NoFrame);
    }

    QSize minimumSizeHint() const override
    {
        return {PageLayout::SidebarNavWidth, 0};
    }
};

void centerWindowOnAvailableScreen(QWidget *window, const QRect &available)
{
    const QPoint frameOffset = window->pos() - window->frameGeometry().topLeft();
    QSize frameSize = window->frameGeometry().size();
    if (frameSize.width() <= 0 || frameSize.height() <= 0) {
        frameSize = window->size();
    }

    int x = available.x() + (available.width() - frameSize.width()) / 2;
    int y = available.y() + (available.height() - frameSize.height()) / 2;
    x = qBound(available.left(), x, qMax(available.left(), available.right() - frameSize.width() + 1));
    y = qBound(available.top(), y, qMax(available.top(), available.bottom() - frameSize.height() + 1));
    window->move(QPoint(x, y) + frameOffset);
}

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

    window->resize(width, height);
    centerWindowOnAvailableScreen(window, available);
}

void applyMainWindowGeometry(QMainWindow *window)
{
    if (window == nullptr) {
        return;
    }

    window->setMinimumSize(MainWindowMinWidth, MainWindowMinHeight);
    fitWindowToScreen(window, MainWindowDefaultWidth, MainWindowDefaultHeight);
}

void enableResizableWindow(QWidget *window, bool applyExpandingPolicy)
{
    if (window == nullptr) {
        return;
    }

    Qt::WindowFlags flags = window->windowFlags();
    flags &= ~Qt::MSWindowsFixedSizeDialogHint;
    flags |= Qt::WindowMinMaxButtonsHint;
    flags |= Qt::WindowSystemMenuHint;
    window->setWindowFlags(flags);
    if (applyExpandingPolicy) {
        window->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
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

QWidget *makeTabBar(const QStringList &labels,
                    QWidget *parent,
                    QButtonGroup **groupOut,
                    QStackedWidget *stack,
                    int defaultIndex)
{
    auto *tabBar = new QWidget(parent);
    tabBar->setObjectName(QStringLiteral("dashboardTabBar"));
    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(4, 4, 4, 4);
    tabLayout->setSpacing(Space8);

    auto *tabGroup = new QButtonGroup(tabBar);
    tabGroup->setExclusive(true);
    for (int i = 0; i < labels.size(); ++i) {
        auto *button = new QPushButton(labels.at(i), tabBar);
        button->setObjectName(QStringLiteral("tabButton"));
        button->setCheckable(true);
        button->setChecked(i == defaultIndex);
        tabGroup->addButton(button, i);
        tabLayout->addWidget(button);
    }
    tabLayout->addStretch();

    if (groupOut != nullptr) {
        *groupOut = tabGroup;
    }
    if (stack != nullptr) {
        QObject::connect(tabGroup, &QButtonGroup::idClicked, stack, &QStackedWidget::setCurrentIndex);
    }
    return tabBar;
}

QWidget *wrapContentPanel(QWidget *page)
{
    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("contentPanel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);
    if (page != nullptr) {
        page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        panelLayout->addWidget(page, 1);
    }
    return panel;
}

QWidget *wrapScrollableContentPanel(QWidget *page)
{
    auto *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("contentScroll"));
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll->setAttribute(Qt::WA_StyledBackground, true);
    scroll->setWidget(wrapContentPanel(page));
    return scroll;
}

QListWidget *createSidebarNavigationList(QWidget *parent)
{
    return new SidebarNavigationList(parent);
}

QWidget *wrapSidebarNavigation(QListWidget *navigation)
{
    if (navigation == nullptr) {
        return nullptr;
    }

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("sidebarNavPanel"));
    panel->setFixedWidth(SidebarNavWidth);
    panel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    panel->setAttribute(Qt::WA_StyledBackground, true);

    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(Space12, Space16, Space12, Space16);
    panelLayout->setSpacing(Space12);

    auto *brand = new QWidget(panel);
    brand->setObjectName(QStringLiteral("sidebarBrand"));
    auto *brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(0, 0, 0, 0);
    brandLayout->setSpacing(Space8);
    auto *brandMark = new QLabel(QStringLiteral("D"), brand);
    brandMark->setObjectName(QStringLiteral("sidebarBrandMark"));
    brandMark->setAlignment(Qt::AlignCenter);
    auto *brandTitle = new QLabel(QStringLiteral("Deploy Hub"), brand);
    brandTitle->setObjectName(QStringLiteral("sidebarBrandTitle"));
    brandLayout->addWidget(brandMark);
    brandLayout->addWidget(brandTitle, 1);
    panelLayout->addWidget(brand);

    navigation->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    panelLayout->addWidget(navigation, 1);

    auto *divider = new QFrame(panel);
    divider->setObjectName(QStringLiteral("sidebarDivider"));
    divider->setFrameShape(QFrame::HLine);
    panelLayout->addWidget(divider);

    auto *footer = new QWidget(panel);
    footer->setObjectName(QStringLiteral("sidebarFooter"));
    auto *footerLayout = new QVBoxLayout(footer);
    footerLayout->setContentsMargins(0, 0, 0, 0);
    footerLayout->setSpacing(Space8);
    auto *settings = new QLabel(QStringLiteral("⚙  设置"), footer);
    settings->setObjectName(QStringLiteral("sidebarFooterText"));
    auto *admin = new QLabel(QStringLiteral("Admin"), footer);
    admin->setObjectName(QStringLiteral("sidebarUserText"));
    footerLayout->addWidget(settings);
    footerLayout->addWidget(admin);
    panelLayout->addWidget(footer);
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
