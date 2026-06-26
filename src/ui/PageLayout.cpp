#include "ui/PageLayout.h"

#include "ui/Theme.h"
#include "qml/LineTabBarController.h"
#include "qml/QmlEngineProvider.h"

#include <QButtonGroup>
#include <QColor>
#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <Qt>
#include <QStackedWidget>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QHeaderView>
#include <QMainWindow>
#include <QLabel>
#include <QLineEdit>
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
        setIconSize(QSize(18, 18));
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
    layout->setContentsMargins(Space24, Space24, Space24, Space24);
    layout->setSpacing(Space24);
}

void applyDialog(QVBoxLayout *layout)
{
    layout->setContentsMargins(Space16, Space16, Space16, Space16);
    layout->setSpacing(Space16);
}

void applyForm(QFormLayout *form)
{
    form->setSpacing(Space16);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(Space16);
    form->setVerticalSpacing(Space16);
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
    Qt::WindowFlags flags = dialog->windowFlags();
    flags |= Qt::Window;
    dialog->setWindowFlags(flags);
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_QuitOnClose, false);
    dialog->setMinimumSize(DialogMinWidth, DialogMinHeight);
    fitWindowToScreen(dialog, DialogDefaultWidth, DialogDefaultHeight);
}

void applyCompactModalDialog(QDialog *dialog, int preferredWidth, int preferredHeight)
{
    if (dialog == nullptr) {
        return;
    }
    enableResizableWindow(dialog);
    Qt::WindowFlags flags = dialog->windowFlags();
    flags |= Qt::Window;
    dialog->setWindowFlags(flags);
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_QuitOnClose, false);
    dialog->setMinimumSize(CompactDialogMinWidth, CompactDialogMinHeight);
    fitWindowToScreen(dialog, preferredWidth, preferredHeight);
}

void applyFormModalDialog(QDialog *dialog, int preferredWidth, int preferredHeight)
{
    if (dialog == nullptr) {
        return;
    }
    enableResizableWindow(dialog);
    Qt::WindowFlags flags = dialog->windowFlags();
    flags |= Qt::Window;
    dialog->setWindowFlags(flags);
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_QuitOnClose, false);
    dialog->setMinimumSize(FormDialogMinWidth, FormDialogMinHeight);
    fitWindowToScreen(dialog, preferredWidth, preferredHeight);
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
    Qt::WindowFlags flags = dialog->windowFlags();
    flags |= Qt::Window;
    dialog->setWindowFlags(flags);
    dialog->setModal(false);
    dialog->setAttribute(Qt::WA_QuitOnClose, false);
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
    widget->setMinimumWidth(0);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        combo->setMinimumContentsLength(16);
        combo->setMaxVisibleItems(12);
        if (combo->view() != nullptr) {
            combo->view()->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
            combo->view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        }
    }

    if (auto *spin = qobject_cast<QSpinBox *>(widget)) {
        spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        spin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        spin->setAccelerated(true);
    }

    if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
}

void applyLighterCardShadow(QWidget *widget)
{
    if (widget == nullptr || widget->graphicsEffect() != nullptr) {
        return;
    }

    // Approximate Element Plus --el-box-shadow-lighter:
    // 0 1px 2px rgba(0, 0, 0, 0.03), 0 0 6px rgba(0, 0, 0, 0.02)
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(6);
    shadow->setOffset(0, 1);
    shadow->setColor(QColor(0, 0, 0, 18));
    widget->setGraphicsEffect(shadow);
}

void configurePathField(QWidget *widget)
{
    if (widget == nullptr) {
        return;
    }
    widget->setMinimumHeight(DialogFieldHeight);
    widget->setMinimumWidth(0);
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void configureHorizontalFormRow(QWidget *row)
{
    if (row == nullptr) {
        return;
    }
    row->setMinimumWidth(0);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void tuneDialogFormBox(QGroupBox *box, QFormLayout *form)
{
    if (box == nullptr || form == nullptr) {
        return;
    }
    box->setObjectName(QStringLiteral("dialogFormBox"));
    box->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    applyDialogForm(form);
}

QWidget *wrapScrollableBody(QVBoxLayout *dialogLayout)
{
    auto *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("dialogScroll"));
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *content = new QWidget;
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    scroll->setWidget(content);
    dialogLayout->addWidget(scroll, 1);
    return content;
}

QWidget *wrapDialogFormSection(const QString &title, QWidget *parent, QFormLayout **formOut)
{
    auto *container = new QWidget(parent);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(Space8);
    if (!title.isEmpty()) {
        containerLayout->addWidget(makeSectionLabel(title, container));
    }

    auto *panel = new QFrame(container);
    panel->setObjectName(QStringLiteral("dialogFormPanel"));
    panel->setAttribute(Qt::WA_StyledBackground, true);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(Space16, Space16, Space16, Space16);
    panelLayout->setSpacing(0);

    auto *form = new QFormLayout(panel);
    applyDialogForm(form);
    panelLayout->addLayout(form);
    containerLayout->addWidget(panel);

    if (formOut != nullptr) {
        *formOut = form;
    }
    return container;
}

QWidget *wrapDialogHintSection(const QString &title, QWidget *parent, QLabel **textOut)
{
    auto *container = new QWidget(parent);
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(Space8);
    if (!title.isEmpty()) {
        auto *titleLabel = makeSectionLabel(title, container);
        titleLabel->setObjectName(QStringLiteral("dialogHintTitle"));
        containerLayout->addWidget(titleLabel);
    }

    auto *panel = new QFrame(container);
    panel->setObjectName(QStringLiteral("dialogHintPanel"));
    panel->setAttribute(Qt::WA_StyledBackground, true);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(Space16, Space16, Space16, Space16);
    panelLayout->setSpacing(0);

    auto *text = new QLabel(panel);
    text->setObjectName(QStringLiteral("serviceHintText"));
    text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse);
    panelLayout->addWidget(text);
    containerLayout->addWidget(panel);

    if (textOut != nullptr) {
        *textOut = text;
    }
    return container;
}

QWidget *makeDialogFooter(QWidget *parent)
{
    auto *footer = new QWidget(parent);
    footer->setObjectName(QStringLiteral("dialogFooter"));
    footer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    return footer;
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

QLabel *makeRequiredFormLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(parent);
    label->setTextFormat(Qt::RichText);
    label->setText(QStringLiteral("%1 <span style=\"color:#C2185B;\">*</span>").arg(text.toHtmlEscaped()));
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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

void configureListingTable(QTableWidget *table, int stretchColumn)
{
    if (table == nullptr || table->columnCount() <= 0) {
        return;
    }
    configureDataTable(table);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->horizontalHeader()->setHighlightSections(false);
    const int stretch = stretchColumn >= 0 ? stretchColumn : table->columnCount() - 1;
    for (int col = 0; col < table->columnCount(); ++col) {
        if (col == stretch) {
            table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
        } else {
            table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
    }
}

void configureListingTableWithActionColumn(QTableWidget *table, int actionColumn, int stretchColumn)
{
    if (table == nullptr || table->columnCount() <= 0) {
        return;
    }
    configureDataTable(table);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    table->horizontalHeader()->setHighlightSections(false);

    const int stretch = stretchColumn >= 0 ? stretchColumn : qMax(0, actionColumn - 1);
    for (int col = 0; col < table->columnCount(); ++col) {
        if (col == actionColumn) {
            table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Fixed);
        } else if (col == stretch) {
            table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::Stretch);
        } else {
            table->horizontalHeader()->setSectionResizeMode(col, QHeaderView::ResizeToContents);
        }
    }
}

QPushButton *makeTableActionButton(const QString &label, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(label, parent);
    button->setObjectName(objectName);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    const int textWidth = QFontMetrics(button->font()).horizontalAdvance(label);
    button->setFixedSize(qMax(textWidth + 8, 28), 22);
    return button;
}

void ensureTableActionColumnWidth(QTableWidget *table, int actionColumn)
{
    if (table == nullptr || actionColumn < 0 || actionColumn >= table->columnCount()) {
        return;
    }
    int maxPanelWidth = 0;
    for (int row = 0; row < table->rowCount(); ++row) {
        if (QWidget *widget = table->cellWidget(row, actionColumn)) {
            widget->adjustSize();
            maxPanelWidth = qMax(maxPanelWidth, widget->sizeHint().width());
        }
    }
    const int width = qMax(132, maxPanelWidth + PageLayout::Space8);
    table->setColumnWidth(actionColumn, width);
    table->horizontalHeader()->setSectionResizeMode(actionColumn, QHeaderView::Fixed);
}

void refreshListingTableColumns(QTableWidget *table, int stretchColumn)
{
    if (table == nullptr || table->columnCount() <= 0) {
        return;
    }
    const int stretch = stretchColumn >= 0 ? stretchColumn : table->columnCount() - 1;
    const QHeaderView::ResizeMode actionMode =
        table->horizontalHeader()->sectionResizeMode(table->columnCount() - 1);
    const bool hasFixedActionColumn = actionMode == QHeaderView::Fixed;
    for (int col = 0; col < table->columnCount(); ++col) {
        if (col != stretch && !(hasFixedActionColumn && col == table->columnCount() - 1)) {
            table->resizeColumnToContents(col);
        }
    }
    if (hasFixedActionColumn) {
        ensureTableActionColumnWidth(table, table->columnCount() - 1);
    }
}

QWidget *makeHeaderBlock(const QString &title, const QString &subtitle, QWidget *parent)
{
    Q_UNUSED(title);
    Q_UNUSED(subtitle);
    Q_UNUSED(parent);
    return nullptr;
}

namespace {

QWidget *makeTabBarInternal(const QStringList &labels,
                            QWidget *parent,
                            QButtonGroup **groupOut,
                            QStackedWidget *stack,
                            int defaultIndex,
                            const QString &barObjectName,
                            const QString &buttonObjectName,
                            int spacing)
{
    auto *tabBar = new QWidget(parent);
    tabBar->setObjectName(barObjectName);
    auto *tabLayout = new QHBoxLayout(tabBar);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(spacing);

    auto *tabGroup = new QButtonGroup(tabBar);
    tabGroup->setExclusive(true);
    for (int i = 0; i < labels.size(); ++i) {
        auto *button = new QPushButton(labels.at(i), tabBar);
        button->setObjectName(buttonObjectName);
        button->setCheckable(true);
        button->setChecked(i == defaultIndex);
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::NoFocus);
        tabGroup->addButton(button, i);
        tabLayout->addWidget(button, 0, Qt::AlignVCenter);
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

}

QWidget *makeModuleTabBar(const QStringList &labels,
                          QWidget *parent,
                          QButtonGroup **groupOut,
                          QStackedWidget *stack,
                          int defaultIndex)
{
    return makeTabBarInternal(labels,
                              parent,
                              groupOut,
                              stack,
                              defaultIndex,
                              QStringLiteral("moduleTabBar"),
                              QStringLiteral("moduleTabButton"),
                              Space8);
}

QWidget *makeLineTabBar(const QStringList &labels,
                        QWidget *parent,
                        LineTabBarController **controllerOut,
                        QStackedWidget *stack,
                        int defaultIndex)
{
    QButtonGroup *tabGroup = nullptr;
    auto *tabBar = makeTabBarInternal(labels,
                                      parent,
                                      &tabGroup,
                                      stack,
                                      defaultIndex,
                                      QStringLiteral("lineTabBar"),
                                      QStringLiteral("lineTabButton"),
                                      Space12);

    auto *controller = new LineTabBarController(tabBar);
    controller->setLabels(labels);
    controller->bindStack(stack);
    controller->bindButtonGroup(tabGroup);
    if (defaultIndex >= 0 && defaultIndex < labels.size()) {
        controller->setCurrentIndex(defaultIndex);
    }

    if (controllerOut != nullptr) {
        *controllerOut = controller;
    }
    return tabBar;
}

QWidget *makeQmlContentPage(const QUrl &source,
                            const QList<QPair<QString, QObject *>> &contextObjects,
                            QWidget *parent)
{
    auto *page = new QWidget(parent);
    page->setProperty("fitFirstScreen", true);
    page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *quickWidget = QmlEngineProvider::instance().createWidget(page, QColor(QStringLiteral("#FFFFFF")));
    quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    for (const auto &entry : contextObjects) {
        quickWidget->rootContext()->setContextProperty(entry.first, entry.second);
    }
    quickWidget->setSource(source);
    layout->addWidget(quickWidget, 1);

    return page;
}

QWidget *makeTabBar(const QStringList &labels,
                    QWidget *parent,
                    QButtonGroup **groupOut,
                    QStackedWidget *stack,
                    int defaultIndex)
{
    return makeModuleTabBar(labels, parent, groupOut, stack, defaultIndex);
}

QWidget *wrapContentPanel(QWidget *page)
{
    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("contentPanel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(Space24, Space24, Space24, Space24);
    panelLayout->setSpacing(Space24);
    if (page != nullptr) {
        page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        panelLayout->addWidget(page, 1);
    }
    return panel;
}

QWidget *wrapContentPadding(QWidget *page)
{
    auto *shell = new QWidget;
    shell->setObjectName(QStringLiteral("contentPaddingShell"));
    shell->setAttribute(Qt::WA_StyledBackground, true);
    auto *panelLayout = new QVBoxLayout(shell);
    panelLayout->setContentsMargins(Space24, Space24, Space24, Space24);
    panelLayout->setSpacing(0);
    if (page != nullptr) {
        page->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        panelLayout->addWidget(page);
    }
    return shell;
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

QWidget *wrapScrollableCardStack(QWidget *page)
{
    auto *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("cardStackScroll"));
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll->setAttribute(Qt::WA_StyledBackground, true);
    scroll->setWidget(wrapContentPadding(page));
    return scroll;
}

QWidget *wrapModulePage(QWidget *page)
{
    if (page == nullptr) {
        return nullptr;
    }
    if (page->property("cardStackPage").toBool()) {
        return wrapScrollableCardStack(page);
    }
    if (page->property("fitFirstScreen").toBool()) {
        return wrapContentPanel(page);
    }
    return wrapScrollableContentPanel(page);
}

QListWidget *createSidebarNavigationList(QWidget *parent)
{
    return new SidebarNavigationList(parent);
}

QWidget *wrapSidebarNavigation(QListWidget *navigation, QPushButton **settingsButtonOut)
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
    panelLayout->setContentsMargins(0, Space12, Space16, Space16);
    panelLayout->setSpacing(Space12);

    auto *brand = new QWidget(panel);
    brand->setObjectName(QStringLiteral("sidebarBrand"));
    auto *brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(Space20, 0, 0, 0);
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
    footerLayout->setContentsMargins(Space20, 0, 0, 0);
    footerLayout->setSpacing(Space4);
    auto *settings = new QPushButton(QStringLiteral("设置"), footer);
    settings->setObjectName(QStringLiteral("sidebarFooterButton"));
    settings->setCheckable(true);
    settings->setCursor(Qt::PointingHandCursor);
    settings->setFocusPolicy(Qt::NoFocus);
    auto *admin = new QLabel(QStringLiteral("Admin"), footer);
    admin->setObjectName(QStringLiteral("sidebarUserText"));
    footerLayout->addWidget(settings);
    footerLayout->addWidget(admin);
    panelLayout->addWidget(footer);

    if (settingsButtonOut != nullptr) {
        *settingsButtonOut = settings;
    }
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
    configureListingTable(table);
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
