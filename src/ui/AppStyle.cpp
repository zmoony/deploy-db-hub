#include "ui/AppStyle.h"

#include <QApplication>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QColor>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProxyStyle>
#include <QPushButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionSpinBox>
#include <QSpinBox>
#include <QStyle>

namespace {

constexpr auto kPageBg = "#FAFAF9";
constexpr auto kCardBg = "#FFFFFF";
constexpr auto kCodeBg = "#F5F5F4";
constexpr auto kText = "#262626";
constexpr auto kMutedText = "#737373";
constexpr auto kPlaceholderText = "#A3A3A3";
constexpr auto kAccent = "#B8B0D1";
constexpr auto kAccentSoft = "#F2F0F8";
constexpr auto kAccentHover = "#EDEAF5";
constexpr auto kBorder = "#E5E5E5";
constexpr auto kSoftBorder = "#F0F0EF";
constexpr auto kSuccess = "#B4E2CB";
constexpr auto kWarning = "#F9D8B8";
constexpr auto kDanger = "#E8B8B8";

bool isSoftSurface(QObject *object)
{
    const QString name = object->objectName();
    return name == QStringLiteral("contentPanel")
        || name == QStringLiteral("metricCard")
        || name == QStringLiteral("deployExecBox")
        || name == QStringLiteral("remoteFileHeader")
        || name == QStringLiteral("dashboardHero")
        || name == QStringLiteral("dashboardStatCard")
        || name == QStringLiteral("dashboardTablePanel");
}

void applySoftShadow(QWidget *widget, qreal opacity)
{
    if (widget == nullptr || widget->graphicsEffect() != nullptr) {
        return;
    }

    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 6);
    shadow->setColor(QColor(0, 0, 0, static_cast<int>(255 * opacity)));
    widget->setGraphicsEffect(shadow);
}

void refreshShadow(QWidget *widget, qreal opacity)
{
    if (widget == nullptr) {
        return;
    }

    auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(widget->graphicsEffect());
    if (shadow == nullptr) {
        applySoftShadow(widget, opacity);
        return;
    }

    shadow->setBlurRadius(opacity > 0.04 ? 22 : 18);
    shadow->setOffset(0, opacity > 0.04 ? 7 : 6);
    shadow->setColor(QColor(0, 0, 0, static_cast<int>(255 * opacity)));
}

void drawChevron(QPainter *painter, const QRect &rect, bool up, bool enabled)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(enabled ? QColor(QString::fromLatin1(kAccent)) : QColor(QString::fromLatin1(kPlaceholderText)));
    pen.setWidthF(1.8);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);

    const QPointF center = rect.center();
    const qreal half = 4.0;
    QPainterPath path;
    if (up) {
        path.moveTo(center.x() - half, center.y() + 1.5);
        path.lineTo(center.x(), center.y() - 2.0);
        path.lineTo(center.x() + half, center.y() + 1.5);
    } else {
        path.moveTo(center.x() - half, center.y() - 1.5);
        path.lineTo(center.x(), center.y() + 2.0);
        path.lineTo(center.x() + half, center.y() - 1.5);
    }
    painter->drawPath(path);
    painter->restore();
}

void drawInputPanel(QPainter *painter, const QRect &rect, bool hover, bool focused, bool enabled)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QColor fill = enabled
        ? QColor(hover ? QStringLiteral("#FBFBFA") : QStringLiteral("#FFFFFF"))
        : QColor(QString::fromLatin1(kCodeBg));
    const QColor stroke = focused
        ? QColor(QString::fromLatin1(kAccent))
        : QColor(hover ? QStringLiteral("#E5E5E5") : QString::fromLatin1(kSoftBorder));

    QPen pen(stroke);
    pen.setWidthF(1.0);
    painter->setPen(pen);
    painter->setBrush(fill);
    painter->drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);
    painter->restore();
}

void drawControlPill(QPainter *painter, const QRect &rect, bool hover, bool enabled)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QColor color = enabled
        ? QColor(hover ? QStringLiteral("#EDEAF5") : QString::fromLatin1(kAccentSoft))
        : QColor(QString::fromLatin1(kCodeBg));
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawRoundedRect(QRectF(rect).adjusted(5, 7, -7, -7), 8, 8);
    painter->restore();
}

void applyComboPopupStyle(QAbstractItemView *view)
{
    if (view == nullptr) {
        return;
    }

    view->setObjectName(QStringLiteral("comboPopup"));
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setAutoFillBackground(true);
    view->viewport()->setAutoFillBackground(true);

    QPalette popupPalette = view->palette();
    popupPalette.setColor(QPalette::Base, QColor(QStringLiteral("#FFFFFF")));
    popupPalette.setColor(QPalette::AlternateBase, QColor(QStringLiteral("#FFFFFF")));
    popupPalette.setColor(QPalette::Window, QColor(QStringLiteral("#FFFFFF")));
    popupPalette.setColor(QPalette::Text, QColor(QStringLiteral("#262626")));
    popupPalette.setColor(QPalette::WindowText, QColor(QStringLiteral("#262626")));
    popupPalette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#262626")));
    popupPalette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#262626")));
    popupPalette.setColor(QPalette::Highlight, QColor(QStringLiteral("#EDEAF5")));
    view->setPalette(popupPalette);
    view->viewport()->setPalette(popupPalette);
    if (QWidget *popupWindow = view->window(); popupWindow != nullptr) {
        popupWindow->setPalette(popupPalette);
        popupWindow->setAutoFillBackground(true);
        popupWindow->setStyleSheet(QStringLiteral("background-color: #FFFFFF; color: #262626;"));
    }

    view->setStyleSheet(QStringLiteral(R"(
        QAbstractItemView {
            background-color: #FFFFFF;
            color: #262626;
            selection-background-color: #EDEAF5;
            selection-color: #262626;
            border: 1px solid #E5E5E5;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }
        QAbstractItemView::item {
            background-color: #FFFFFF;
            color: #262626;
            min-height: 30px;
            padding: 6px 14px;
            border-radius: 8px;
        }
        QAbstractItemView::item:hover {
            background-color: #F5F5F4;
            color: #262626;
        }
        QAbstractItemView::item:selected {
            background-color: #EDEAF5;
            color: #262626;
        }
    )"));
}

class AppProxyStyle final : public QProxyStyle
{
public:
    explicit AppProxyStyle(QStyle *baseStyle = nullptr)
        : QProxyStyle(baseStyle)
    {
    }

    QRect subControlRect(ComplexControl control,
                         const QStyleOptionComplex *option,
                         SubControl subControl,
                         const QWidget *widget = nullptr) const override
    {
        if (control == QStyle::CC_ComboBox && subControl == QStyle::SC_ComboBoxArrow) {
            QRect rect = option->rect;
            return QRect(rect.right() - 36, rect.top(), 36, rect.height());
        }
        if (control == QStyle::CC_ComboBox && subControl == QStyle::SC_ComboBoxEditField) {
            return option->rect.adjusted(12, 2, -42, -2);
        }
        if (control == QStyle::CC_SpinBox) {
            QRect rect = option->rect;
            const QRect buttonRect(rect.right() - 36, rect.top(), 36, rect.height());
            if (subControl == QStyle::SC_SpinBoxUp) {
                return QRect(buttonRect.left(), buttonRect.top(), buttonRect.width(), buttonRect.height() / 2);
            }
            if (subControl == QStyle::SC_SpinBoxDown) {
                return QRect(buttonRect.left(), buttonRect.center().y(), buttonRect.width(), buttonRect.height() / 2);
            }
            if (subControl == QStyle::SC_SpinBoxEditField) {
                return rect.adjusted(12, 2, -42, -2);
            }
        }
        return QProxyStyle::subControlRect(control, option, subControl, widget);
    }

    void drawComplexControl(ComplexControl control,
                            const QStyleOptionComplex *option,
                            QPainter *painter,
                            const QWidget *widget = nullptr) const override
    {
        if (control == QStyle::CC_ComboBox) {
            const bool hover = option->state & QStyle::State_MouseOver;
            const bool focused = option->state & QStyle::State_HasFocus;
            const bool enabled = option->state & QStyle::State_Enabled;
            drawInputPanel(painter, option->rect, hover, focused, enabled);

            const QRect arrowRect = subControlRect(control, option, QStyle::SC_ComboBoxArrow, widget);
            const bool arrowHover = (option->activeSubControls & QStyle::SC_ComboBoxArrow) != 0 || hover;
            drawControlPill(painter, arrowRect, arrowHover, enabled);
            drawChevron(painter, arrowRect.adjusted(0, 1, -3, 0), false, enabled);

            return;
        }

        if (control == QStyle::CC_SpinBox) {
            const bool hover = option->state & QStyle::State_MouseOver;
            const bool focused = option->state & QStyle::State_HasFocus;
            const bool enabled = option->state & QStyle::State_Enabled;
            drawInputPanel(painter, option->rect, hover, focused, enabled);
            const QRect upRect = subControlRect(control, option, QStyle::SC_SpinBoxUp, widget);
            const QRect downRect = subControlRect(control, option, QStyle::SC_SpinBoxDown, widget);
            const bool upHover = (option->activeSubControls & QStyle::SC_SpinBoxUp) != 0;
            const bool downHover = (option->activeSubControls & QStyle::SC_SpinBoxDown) != 0;
            const QRect buttonRect = QRect(upRect.left(), upRect.top(), upRect.width(), upRect.height() + downRect.height());
            drawControlPill(painter, buttonRect, upHover || downHover || hover, enabled);
            drawChevron(painter, upRect.adjusted(0, 1, -3, 0), true, enabled);
            drawChevron(painter, downRect.adjusted(0, -1, -3, 0), false, enabled);
            return;
        }

        QProxyStyle::drawComplexControl(control, option, painter, widget);
    }

    void drawPrimitive(PrimitiveElement element,
                       const QStyleOption *option,
                       QPainter *painter,
                       const QWidget *widget = nullptr) const override
    {
        if (element == QStyle::PE_FrameFocusRect) {
            return;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }
};

class AppPolishFilter final : public QObject
{
public:
    explicit AppPolishFilter(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Polish) {
            if (auto *label = qobject_cast<QLabel *>(watched)) {
                label->setTextInteractionFlags(Qt::NoTextInteraction);
                label->setFocusPolicy(Qt::NoFocus);
            }
            if (auto *groupBox = qobject_cast<QGroupBox *>(watched)) {
                groupBox->setFocusPolicy(Qt::NoFocus);
            }
            if (auto *frame = qobject_cast<QFrame *>(watched); frame != nullptr && isSoftSurface(frame)) {
                applySoftShadow(frame, 0.04);
            }
            if (auto *groupBox = qobject_cast<QGroupBox *>(watched)) {
                applySoftShadow(groupBox, 0.04);
            }
            if (auto *combo = qobject_cast<QComboBox *>(watched)) {
                combo->setMaxVisibleItems(12);
                applyComboPopupStyle(combo->view());
                if (QLineEdit *lineEdit = combo->lineEdit()) {
                    lineEdit->hide();
                }
            }
            if (auto *spin = qobject_cast<QSpinBox *>(watched)) {
                spin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
                spin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            }
        }
        if (event->type() == QEvent::Show || event->type() == QEvent::MouseButtonPress) {
            if (auto *combo = qobject_cast<QComboBox *>(watched)) {
                applyComboPopupStyle(combo->view());
            }
        }
        if (event->type() == QEvent::Enter) {
            if (auto *button = qobject_cast<QPushButton *>(watched)) {
                refreshShadow(button, 0.06);
            }
        }
        if (event->type() == QEvent::Leave) {
            if (auto *button = qobject_cast<QPushButton *>(watched)) {
                button->setGraphicsEffect(nullptr);
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

}

namespace AppStyle {

void apply(QApplication &app)
{
    app.setStyle(new AppProxyStyle(app.style()));
    app.installEventFilter(new AppPolishFilter(&app));
    app.setProperty("theme.pageBg", kPageBg);
    app.setProperty("theme.cardBg", kCardBg);
    app.setProperty("theme.codeBg", kCodeBg);
    app.setProperty("theme.text", kText);
    app.setProperty("theme.mutedText", kMutedText);
    app.setProperty("theme.placeholderText", kPlaceholderText);
    app.setProperty("theme.accent", kAccent);
    app.setProperty("theme.accentSoft", kAccentSoft);
    app.setProperty("theme.accentHover", kAccentHover);
    app.setProperty("theme.border", kBorder);
    app.setProperty("theme.softBorder", kSoftBorder);
    app.setProperty("theme.success", kSuccess);
    app.setProperty("theme.warning", kWarning);
    app.setProperty("theme.danger", kDanger);
    app.setStyleSheet(QStringLiteral(R"(
        /*
         * Theme tokens (CSS variable equivalents for Qt QSS):
         * --page-bg: #FAFAF9;
         * --card-bg: #FFFFFF;
         * --code-bg: #F5F5F4;
         * --text: #262626;
         * --muted-text: #737373;
         * --placeholder-text: #A3A3A3;
         * --accent: #B8B0D1;
         * --accent-soft: #F2F0F8;
         * --success-soft: #B4E2CB;
         * --warning-soft: #F9D8B8;
         * --danger-soft: #E8B8B8;
         * --border-hover: #E5E5E5;
         * --radius-md: 12px;
         * --space-1: 8px;
         * --space-2: 12px;
         * --space-3: 16px;
         * --space-4: 24px;
         */
        * {
            font-family: "Inter", "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 13px;
            letter-spacing: 0.2px;
            outline: none;
        }
        QMainWindow {
            background-color: #FAFAF9;
            color: #262626;
        }
        QMainWindow > QWidget {
            background-color: #FAFAF9;
        }
        QWidget {
            background: transparent;
            color: #262626;
        }
        QStatusBar {
            background-color: #FAFAF9;
            color: #737373;
            border: none;
            padding: 8px 12px;
            selection-background-color: transparent;
            selection-color: #737373;
        }
        QLabel, QGroupBox, QGroupBox::title, QPushButton, QProgressBar, QToolBar {
            background: transparent;
            selection-background-color: transparent;
            selection-color: inherit;
        }
        QListWidget#sidebarNav {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
            padding: 12px;
            outline: none;
        }
        QListWidget#sidebarNav::item {
            background-color: #F5F5F4;
            color: #737373;
            border-radius: 12px;
            padding: 12px 16px;
            margin: 4px 0;
            selection-background-color: transparent;
            selection-color: #737373;
        }
        QListWidget#sidebarNav::item:selected {
            background-color: #F2F0F8;
            color: #262626;
            border: none;
            font-weight: 600;
        }
        QListWidget#sidebarNav::item:hover {
            background-color: #F7F5FA;
        }
        QFrame#contentPanel {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
        }
        QWidget#dashboardTabBar {
            background-color: #F5F3FA;
            border: 1px solid transparent;
            border-radius: 12px;
            padding: 4px;
        }
        QWidget#dashboardHero,
        QWidget#dashboardSummary,
        QWidget#dashboardGrid,
        QWidget#dashboardTablePanel {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
        }
        QWidget#dashboardHero {
            background-color: #F7F5FA;
        }
        QLabel#dashboardKicker {
            color: #8A84A7;
            font-size: 12px;
            font-weight: 600;
            letter-spacing: 1px;
        }
        QLabel#dashboardTitle {
            color: #262626;
            font-size: 20px;
            font-weight: 700;
        }
        QLabel#dashboardMeta {
            color: #737373;
            font-size: 12px;
        }
        QFrame#dashboardStatCard {
            background-color: #FBFAFD;
            border: 1px solid transparent;
            border-radius: 12px;
            min-height: 92px;
        }
        QLabel#dashboardStatValue {
            color: #262626;
            font-size: 28px;
            font-weight: 700;
        }
        QLabel#dashboardStatLabel {
            color: #737373;
            font-size: 12px;
            font-weight: 500;
        }
        QLabel#dashboardMiniValue {
            color: #262626;
            font-size: 15px;
            font-weight: 600;
        }
        QLabel#dashboardMiniLabel {
            color: #737373;
            font-size: 12px;
        }
        QGroupBox {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
            margin-top: 18px;
            padding: 20px 18px 16px 18px;
            font-weight: 500;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 8px;
            color: #262626;
        }
        QGroupBox#deployConfigBox,
        QGroupBox#dialogFormBox {
            margin-top: 12px;
            margin-bottom: 0;
            padding: 16px 18px 18px 18px;
        }
        QGroupBox#deployConfigBox QLabel,
        QGroupBox#dialogFormBox QLabel {
            color: #737373;
            font-weight: 500;
            min-width: 64px;
        }
        QFrame#deployExecBox {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
        }
        QPlainTextEdit#deployLog {
            min-height: 220px;
        }
        QLabel#pageTitle {
            font-size: 18px;
            font-weight: 600;
            color: #262626;
            padding: 0;
            margin: 0;
        }
        QLabel#pageSubtitle {
            color: #737373;
            font-size: 13px;
            padding: 0;
            margin: 0;
        }
        QLabel#serviceStatusBadge {
            color: #262626;
            font-weight: 600;
            padding: 8px 12px;
            background-color: #F5F5F4;
            border: 1px solid transparent;
            border-radius: 12px;
        }
        QLabel#sectionLabel {
            color: #262626;
            font-weight: 600;
            font-size: 13px;
            padding: 8px 0 0 0;
            margin-top: 12px;
        }
        QFrame#remoteFileHeader {
            background-color: #F8FAFC;
            border: 1px solid #E5E7EB;
            border-radius: 8px;
        }
        QLabel#remoteConnectionLabel {
            color: #262626;
            font-weight: 600;
        }
        QLabel#remoteStatusLabel {
            color: #737373;
            font-size: 12px;
            padding: 0;
        }
        QWidget#terminalTabToolbar {
            background-color: transparent;
            min-height: 30px;
        }
        QLabel#terminalToolbarTitle {
            color: #0F172A;
            font-size: 12px;
            font-weight: 600;
            padding: 0;
            margin: 0;
        }
        QPushButton#terminalToolbarButton {
            min-height: 24px;
            padding: 4px 12px;
            border-radius: 8px;
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
        }
        QPushButton#terminalToolbarButton:hover {
            background-color: #EFF6FF;
            border-color: #BFDBFE;
        }
        QLabel#remoteStatusDot {
            min-width: 8px;
            max-width: 8px;
            min-height: 8px;
            max-height: 8px;
            border-radius: 4px;
            margin-top: 1px;
        }
        QLabel#remoteStatusDot[level="ok"] {
            background-color: #22C55E;
        }
        QLabel#remoteStatusDot[level="error"] {
            background-color: #EF4444;
        }
        QLabel#remoteStatusDot[level="pending"] {
            background-color: #A3A3A3;
        }
        QTreeWidget#remoteDirTree {
            background-color: #F8FAFC;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 6px;
        }
        QTreeWidget#remoteDirTree::item {
            color: #475569;
            border-radius: 6px;
            padding: 6px 8px;
            margin: 2px 0;
        }
        QTreeWidget#remoteDirTree::item:hover {
            background-color: #EFF6FF;
        }
        QTreeWidget#remoteDirTree::item:selected {
            background-color: #DCEBFF;
            color: #0F172A;
        }
        QTableWidget#remoteFileTable {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
        }
        QTabWidget#remoteOperationTabs::pane {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            top: -1px;
        }
        QTabWidget#remoteOperationTabs::tab-bar {
            alignment: left;
        }
        QTabBar::tab {
            background-color: #F8FAFC;
            color: #475569;
            border: 1px solid #E2E8F0;
            border-bottom: none;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            padding: 7px 14px;
            margin-right: 4px;
            min-height: 18px;
        }
        QTabBar::tab:selected {
            background-color: #FFFFFF;
            color: #0F172A;
            font-weight: 600;
        }
        QTabBar::tab:hover {
            background-color: #EFF6FF;
        }
        QSplitter::handle {
            background-color: transparent;
            width: 8px;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QAbstractItemView {
            outline: none;
            show-decoration-selected: 1;
        }
        QAbstractItemView::item:focus,
        QAbstractItemView::item:selected:focus {
            outline: none;
            border: none;
        }
        QLabel#emptyState {
            color: #737373;
            font-size: 14px;
            padding: 56px 32px;
            background: transparent;
            border: 1px dashed #E5E5E5;
            border-radius: 12px;
        }
        QLabel#metricTitle {
            background: transparent;
            color: #737373;
            font-size: 12px;
            font-weight: 500;
        }
        QLabel#metricValue {
            background: transparent;
            color: #262626;
            font-size: 26px;
            font-weight: 600;
        }
        QFrame#metricCard {
            background-color: #FBFAFD;
            border: 1px solid transparent;
            border-radius: 12px;
            min-height: 88px;
        }
        QTableWidget {
            background-color: #FBFBFA;
            alternate-background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
            gridline-color: transparent;
        }
        QTableWidget#dashboardTable {
            background-color: #FBFBFA;
        }
        QTableWidget::item {
            padding: 10px 12px;
            selection-background-color: transparent;
            selection-color: #262626;
        }
        QTableWidget::item:hover {
            background-color: #FAFAF9;
        }
        QTableWidget::item:selected {
            background-color: #F2F0F8;
            color: #262626;
        }
        QTableWidget::item:focus,
        QTableWidget::item:selected:focus {
            outline: none;
            border: none;
        }
        QHeaderView::section {
            background-color: #F5F3FA;
            color: #737373;
            padding: 12px 12px;
            border: none;
            font-weight: 600;
            selection-background-color: transparent;
            selection-color: #737373;
        }
        QListWidget#sidebarNav,
        QListWidget#sidebarNav::item {
            background-clip: padding;
        }
        QLineEdit, QPlainTextEdit {
            background-color: #FFFFFF;
            color: #262626;
            border: 1px solid #F0F0EF;
            border-radius: 12px;
            padding: 10px 12px;
            selection-background-color: #EDEAF5;
            selection-color: #262626;
        }
        QAbstractItemView#comboPopup {
            background-color: #FFFFFF;
            color: #262626;
            selection-background-color: #EDEDED;
            selection-color: #262626;
            border: 1px solid #E5E5E5;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }
        QAbstractItemView#comboPopup QWidget {
            background-color: #FFFFFF;
            color: #262626;
        }
        QAbstractItemView#comboPopup::item {
            min-height: 30px;
            padding: 6px 14px;
            border-radius: 8px;
        }
        QAbstractItemView#comboPopup::item:hover {
            background-color: #F5F5F4;
        }
        QAbstractItemView#comboPopup::item:selected {
            background-color: #EDEAF5;
            color: #262626;
        }
        QLineEdit {
            min-height: 22px;
        }
        QComboBox QLineEdit {
            border: none;
            padding: 0;
            background: transparent;
            selection-background-color: #EDEAF5;
            selection-color: #262626;
        }
        QPlainTextEdit {
            background-color: #F5F5F4;
        }
        QPlainTextEdit#terminalOutput {
            background-color: #07111E;
            color: #D8FFE4;
            border: 1px solid #12263F;
            border-radius: 8px;
            padding: 12px 14px;
            selection-background-color: #245B94;
            selection-color: #FFFFFF;
            font-size: 12px;
        }
        QPlainTextEdit#terminalOutput:disabled {
            color: #94A3B8;
        }
        QPlainTextEdit#terminalOutput[text=""] {
            color: #D8FFE4;
        }
        QLineEdit:focus, QPlainTextEdit:focus {
            border: 1px solid #B8B0D1;
            outline: none;
        }
        QPlainTextEdit#terminalOutput:focus {
            border: 2px solid #22C55E;
            padding: 11px 13px;
            outline: none;
        }
        QPlainTextEdit#remoteFileViewer {
            background-color: #0F172A;
            color: #DDE7F0;
            border: 1px solid #1E293B;
            border-radius: 8px;
            padding: 12px 14px;
            selection-background-color: #2563EB;
            selection-color: #FFFFFF;
        }
        QComboBox QLineEdit:focus {
            border: none;
            outline: none;
        }
        QPushButton:focus {
            outline: none;
        }
        QListWidget#sidebarNav::item:focus,
        QListWidget#sidebarNav::item:selected:focus {
            outline: none;
            border: none;
        }
        QTreeWidget::item:focus,
        QTreeWidget::item:selected:focus {
            outline: none;
            border: none;
        }
        QGroupBox:focus {
            outline: none;
            border: 1px solid transparent;
        }
        QPushButton {
            background-color: #FFFFFF;
            color: #262626;
            border: 1px solid #F0F0EF;
            border-radius: 12px;
            padding: 10px 16px;
            min-height: 22px;
        }
        QPushButton:hover {
            background-color: #FAFAF9;
            border-color: #E5E5E5;
        }
        QPushButton:pressed {
            background-color: #F5F5F4;
        }
        QPushButton:disabled {
            color: #A3A3A3;
            background-color: #F5F5F4;
        }
        QPushButton#primaryButton {
            background-color: #B8B0D1;
            color: #262626;
            border: 1px solid transparent;
            font-weight: 600;
        }
        QPushButton#primaryButton:hover {
            background-color: #C4BEDA;
            border-color: #E5E5E5;
        }
        QPushButton#tabButton {
            background-color: #FFFFFF;
            color: #737373;
            border: 1px solid transparent;
            padding: 9px 16px;
        }
        QPushButton#tabButton:hover {
            background-color: #FBFAFD;
            color: #262626;
            border-color: #E5E5E5;
        }
        QPushButton#tabButton:checked {
            background-color: #EDEAF5;
            color: #262626;
            font-weight: 600;
        }
        QPushButton#dangerButton {
            background-color: #F7E8E8;
            color: #8F4B4B;
            border: 1px solid transparent;
        }
        QPushButton#dangerButton:hover {
            background-color: #F2DEDE;
            border-color: #E8B8B8;
        }
        QProgressBar {
            background-color: #F5F5F4;
            border: 1px solid transparent;
            border-radius: 12px;
            text-align: center;
            color: #737373;
            height: 22px;
        }
        QProgressBar::chunk {
            background-color: #B8B0D1;
            border-radius: 12px;
        }
        QToolBar#pageToolbar {
            background: transparent;
            border: none;
            spacing: 14px;
            padding: 0;
            margin: 0;
        }
        QDialog {
            background-color: #FAFAF9;
        }
        QInputDialog, QMessageBox {
            background-color: #FAFAF9;
        }
        QInputDialog QLabel, QMessageBox QLabel {
            color: #262626;
            background: transparent;
        }
        QInputDialog QLineEdit, QMessageBox QLineEdit {
            background-color: #FFFFFF;
            color: #262626;
            border: 1px solid #E5E5E5;
            border-radius: 8px;
            padding: 8px 10px;
        }
        QMessageBox {
            color: #262626;
        }
        QMenu {
            background-color: #FFFFFF;
            color: #262626;
            border: 1px solid #E5E5E5;
            border-radius: 10px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 28px 8px 16px;
            border-radius: 6px;
            color: #262626;
            background-color: transparent;
        }
        QMenu::item:selected {
            background-color: #EDEAF5;
            color: #262626;
        }
        QMenu::item:disabled {
            color: #A3A3A3;
        }
        QMenu::separator {
            height: 1px;
            background: #F0F0EF;
            margin: 4px 8px;
        }
        QProgressBar#remoteUploadProgress {
            background-color: #F5F5F4;
            border: 1px solid #E5E5E5;
            border-radius: 8px;
            text-align: center;
            color: #737373;
            font-size: 11px;
        }
        QProgressBar#remoteUploadProgress::chunk {
            background-color: #B8B0D1;
            border-radius: 8px;
        }
        QDialogButtonBox {
            background: transparent;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 6px;
            margin: 2px 0;
        }
        QScrollBar::handle:vertical {
            background: #D9D6E6;
            border-radius: 3px;
            min-height: 32px;
        }
        QScrollBar::handle:vertical:hover {
            background: #C4BEDA;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical {
            background: transparent;
            height: 0;
            width: 0;
        }
        QScrollBar:horizontal {
            background: transparent;
            height: 6px;
            margin: 0 2px;
        }
        QScrollBar::handle:horizontal {
            background: #D9D6E6;
            border-radius: 3px;
            min-width: 32px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #C4BEDA;
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal,
        QScrollBar::sub-page:horizontal {
            background: transparent;
            height: 0;
            width: 0;
        }
    )"));
}

}
