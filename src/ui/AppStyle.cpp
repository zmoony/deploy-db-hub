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

constexpr auto kPageBg = "#F8FAFC";
constexpr auto kCardBg = "#FFFFFF";
constexpr auto kCodeBg = "#F1F5F9";
constexpr auto kText = "#0F172A";
constexpr auto kMutedText = "#64748B";
constexpr auto kPlaceholderText = "#94A3B8";
constexpr auto kAccent = "#6366F1";
constexpr auto kAccentSoft = "#EEF2FF";
constexpr auto kAccentHover = "#E0E7FF";
constexpr auto kBorder = "#E2E8F0";
constexpr auto kSoftBorder = "#E5E7EB";
constexpr auto kSuccess = "#10B981";
constexpr auto kWarning = "#F59E0B";
constexpr auto kDanger = "#EF4444";

bool isSoftSurface(QObject *object)
{
    const QString name = object->objectName();
    return name == QStringLiteral("contentPanel")
        || name == QStringLiteral("contentScroll")
        || name == QStringLiteral("sidebarNavPanel")
        || name == QStringLiteral("metricCard")
        || name == QStringLiteral("deployExecBox")
        || name == QStringLiteral("remoteFileHeader")
        || name == QStringLiteral("dashboardHero")
        || name == QStringLiteral("serviceInstanceBanner")
        || name == QStringLiteral("dashboardStatCard")
        || name == QStringLiteral("dashboardTablePanel")
        || name == QStringLiteral("dashboardQuickAction");
}

void applySoftShadow(QWidget *widget, qreal opacity)
{
    if (widget == nullptr || widget->graphicsEffect() != nullptr) {
        return;
    }

    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(opacity > 0.05 ? 30 : 18);
    shadow->setOffset(0, opacity > 0.05 ? 8 : 6);
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
        : QColor(hover ? QStringLiteral("#CBD5E1") : QString::fromLatin1(kSoftBorder));

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
        ? QColor(hover ? QStringLiteral("#E0E7FF") : QString::fromLatin1(kAccentSoft))
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
    popupPalette.setColor(QPalette::Text, QColor(QStringLiteral("#0F172A")));
    popupPalette.setColor(QPalette::WindowText, QColor(QStringLiteral("#0F172A")));
    popupPalette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#0F172A")));
    popupPalette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#0F172A")));
    popupPalette.setColor(QPalette::Highlight, QColor(QStringLiteral("#E0E7FF")));
    view->setPalette(popupPalette);
    view->viewport()->setPalette(popupPalette);
    if (QWidget *popupWindow = view->window(); popupWindow != nullptr) {
        popupWindow->setPalette(popupPalette);
        popupWindow->setAutoFillBackground(true);
        popupWindow->setStyleSheet(QStringLiteral("background-color: #FFFFFF; color: #0F172A;"));
    }

    view->setStyleSheet(QStringLiteral(R"(
        QAbstractItemView {
            background-color: #FFFFFF;
            color: #0F172A;
            selection-background-color: #E0E7FF;
            selection-color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }
        QAbstractItemView::item {
            background-color: #FFFFFF;
            color: #0F172A;
            min-height: 30px;
            padding: 6px 14px;
            border-radius: 8px;
        }
        QAbstractItemView::item:hover {
            background-color: #F8FAFC;
            color: #0F172A;
        }
        QAbstractItemView::item:selected {
            background-color: #E0E7FF;
            color: #0F172A;
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
            return QRect(rect.right() - 32, rect.top(), 32, rect.height());
        }
        if (control == QStyle::CC_ComboBox && subControl == QStyle::SC_ComboBoxEditField) {
            return option->rect.adjusted(10, 2, -36, -2);
        }
        if (control == QStyle::CC_SpinBox) {
            QRect rect = option->rect;
            const QRect buttonRect(rect.right() - 32, rect.top(), 32, rect.height());
            if (subControl == QStyle::SC_SpinBoxUp) {
                return QRect(buttonRect.left(), buttonRect.top(), buttonRect.width(), buttonRect.height() / 2);
            }
            if (subControl == QStyle::SC_SpinBoxDown) {
                return QRect(buttonRect.left(), buttonRect.center().y(), buttonRect.width(), buttonRect.height() / 2);
            }
            if (subControl == QStyle::SC_SpinBoxEditField) {
                return rect.adjusted(10, 2, -36, -2);
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
            if (auto *widget = qobject_cast<QWidget *>(watched); widget != nullptr && isSoftSurface(widget)) {
                applySoftShadow(widget, 0.04);
            }
            if (auto *groupBox = qobject_cast<QGroupBox *>(watched)) {
                applySoftShadow(groupBox, 0.04);
            }
            if (auto *combo = qobject_cast<QComboBox *>(watched)) {
                combo->setMaxVisibleItems(12);
                applyComboPopupStyle(combo->view());
                if (!combo->isEditable()) {
                    combo->setEditable(true);
                }
                if (QLineEdit *lineEdit = combo->lineEdit()) {
                    lineEdit->setFrame(false);
                    lineEdit->setReadOnly(!combo->property("manualEdit").toBool());
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
                const QString name = button->objectName();
                if (!name.startsWith(QStringLiteral("tableAction"))
                    && name != QStringLiteral("lineTabButton")
                    && name != QStringLiteral("moduleTabButton")) {
                    refreshShadow(button, 0.06);
                }
            }
            if (auto *frame = qobject_cast<QFrame *>(watched);
                frame != nullptr
                && (frame->objectName() == QStringLiteral("dashboardStatCard")
                    || frame->objectName() == QStringLiteral("dashboardQuickAction"))) {
                refreshShadow(frame, 0.08);
            }
        }
        if (event->type() == QEvent::Leave) {
            if (auto *button = qobject_cast<QPushButton *>(watched)) {
                if (!button->objectName().startsWith(QStringLiteral("tableAction"))
                    && button->objectName() != QStringLiteral("lineTabButton")
                    && button->objectName() != QStringLiteral("moduleTabButton")) {
                    button->setGraphicsEffect(nullptr);
                }
            }
            if (auto *frame = qobject_cast<QFrame *>(watched);
                frame != nullptr
                && (frame->objectName() == QStringLiteral("dashboardStatCard")
                    || frame->objectName() == QStringLiteral("dashboardQuickAction"))) {
                refreshShadow(frame, 0.04);
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
         * --page-bg: #F8FAFC;
         * --card-bg: #FFFFFF;
         * --code-bg: #F1F5F9;
         * --text: #0F172A;
         * --muted-text: #64748B;
         * --placeholder-text: #94A3B8;
         * --accent: #6366F1;
         * --accent-soft: #EEF2FF;
         * --success: #10B981;
         * --warning: #F59E0B;
         * --danger: #EF4444;
         * --border-hover: #E2E8F0;
         * --radius-md: 12px;
         * --space-1: 8px;
         * --space-2: 12px;
         * --space-3: 16px;
         * --space-4: 24px;
         */
        * {
            font-family: "Inter", "Segoe UI", "Microsoft YaHei UI", sans-serif;
            font-size: 13px;
            letter-spacing: 0;
            outline: none;
        }
        QMainWindow {
            background-color: #F8FAFC;
            color: #0F172A;
        }
        QMainWindow > QWidget {
            background-color: #F8FAFC;
        }
        QWidget {
            background: transparent;
            color: #0F172A;
        }
        QStatusBar {
            background-color: #F8FAFC;
            color: #64748B;
            border: none;
            padding: 8px 12px;
            selection-background-color: transparent;
            selection-color: #64748B;
        }
        QLabel, QGroupBox, QGroupBox::title, QPushButton, QProgressBar, QToolBar {
            background: transparent;
            selection-background-color: transparent;
            selection-color: inherit;
        }
        QListWidget#sidebarNav {
            background: transparent;
            border: none;
            border-radius: 0;
            padding: 0;
            outline: none;
        }
        QFrame#sidebarNavPanel {
            background-color: #1E293B;
            border: 1px solid #1E293B;
            border-radius: 24px;
        }
        QWidget#sidebarBrand {
            background-color: transparent;
            min-height: 48px;
        }
        QLabel#sidebarBrandMark {
            background-color: #4F46E5;
            color: #FFFFFF;
            border-radius: 12px;
            min-width: 32px;
            max-width: 32px;
            min-height: 32px;
            max-height: 32px;
            font-size: 15px;
            font-weight: 800;
        }
        QLabel#sidebarBrandTitle {
            color: #F8FAFC;
            font-size: 15px;
            font-weight: 700;
        }
        QFrame#sidebarDivider {
            border: none;
            background-color: #334155;
            min-height: 1px;
            max-height: 1px;
        }
        QWidget#sidebarFooter {
            background: transparent;
        }
        QLabel#sidebarFooterText {
            color: #E2E8F0;
            padding: 10px 12px;
            border-radius: 12px;
            font-weight: 500;
        }
        QPushButton#sidebarFooterButton {
            background-color: transparent;
            color: #E2E8F0;
            border: none;
            padding: 10px 12px;
            border-radius: 12px;
            font-weight: 500;
            text-align: left;
        }
        QPushButton#sidebarFooterButton:hover {
            background-color: rgba(255, 255, 255, 0.08);
        }
        QPushButton#sidebarFooterButton:checked {
            background-color: #4F46E5;
            color: #FFFFFF;
        }
        QLabel#sidebarUserText {
            color: #CBD5E1;
            padding: 10px 12px 2px 12px;
        }
        QScrollArea#contentScroll {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 24px;
        }
        QListWidget#toolListNav {
            background-color: #F8FAFC;
            border: 1px solid transparent;
            border-radius: 12px;
            padding: 10px;
            outline: none;
        }
        QListWidget#sidebarNav::item {
            background-color: transparent;
            color: #E2E8F0;
            border-radius: 12px;
            padding: 12px 16px;
            margin: 4px 0;
            selection-background-color: transparent;
            selection-color: #E2E8F0;
        }
        QListWidget#sidebarNav::item:selected {
            background-color: #4F46E5;
            color: #FFFFFF;
            border: none;
            font-weight: 700;
        }
        QListWidget#sidebarNav::item:hover {
            background-color: #334155;
            color: #FFFFFF;
        }
        QListWidget#toolListNav::item {
            background-color: transparent;
            color: #64748B;
            border-radius: 10px;
            padding: 9px 10px;
            margin: 2px 0;
            selection-background-color: transparent;
            selection-color: #64748B;
        }
        QListWidget#toolListNav::item:selected {
            background-color: #EEF2FF;
            color: #0F172A;
            font-weight: 600;
        }
        QListWidget#toolListNav::item:hover {
            background-color: #F1F5F9;
        }
        QFrame#contentPanel {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 24px;
        }
        QWidget#dashboardTabBar,
        QWidget#lineTabBar {
            background-color: transparent;
            border: none;
            border-bottom: 1px solid #E4E7ED;
            border-radius: 0;
            padding: 0;
        }
        QWidget#moduleTabBar {
            background-color: transparent;
            border: none;
            border-radius: 0;
            padding: 0;
        }
        QWidget#dashboardHero,
        QWidget#serviceInstanceBanner,
        QWidget#dashboardSummary,
        QWidget#dashboardGrid,
        QWidget#dashboardTablePanel {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 18px;
        }
        QWidget#dashboardHero {
            background-color: transparent;
            border-radius: 24px;
            min-height: 132px;
            max-height: 132px;
        }
        QLabel#dashboardHeroImage {
            background-color: transparent;
            border: none;
        }
        QWidget#dashboardHeroVisual {
            background-color: rgba(255, 255, 255, 35);
            border-radius: 18px;
            min-width: 320px;
            min-height: 112px;
            max-height: 112px;
        }
        QWidget#serviceInstanceBanner {
            background-color: #EEF2FF;
        }
        QLabel#serviceInstanceTitle {
            color: #0F172A;
            font-size: 16px;
            font-weight: 700;
        }
        QLabel#serviceInstanceStatusOk {
            color: #166534;
            font-weight: 600;
            padding: 4px 10px;
            background-color: #DCFCE7;
            border-radius: 10px;
        }
        QLabel#serviceInstanceStatusBad {
            color: #991B1B;
            font-weight: 600;
            padding: 4px 10px;
            background-color: #FEE2E2;
            border-radius: 10px;
        }
        QLabel#serviceInstanceNode {
            color: #475569;
            font-size: 13px;
        }
        QLabel#serviceParentTag {
            color: #0F172A;
            font-weight: 600;
            padding: 6px 10px;
            background-color: #EEF2FF;
            border-radius: 10px;
        }
        QGroupBox#serviceHintBox {
            border: 1px solid #E2E8F0;
        }
        QWidget#toolPlaceholderBox {
            background-color: #F8FAFC;
            border: 1px solid transparent;
            border-radius: 12px;
            padding: 16px;
        }
        QLabel#toolMessage {
            color: #64748B;
            font-size: 12px;
        }
        QGroupBox#serviceHintBox::title {
            color: #16A34A;
            font-weight: 600;
        }
        QLabel#serviceHintText {
            color: #64748B;
            font-size: 12px;
            line-height: 1.5;
        }
        QLabel#dashboardKicker {
            color: #6366F1;
            font-size: 11px;
            font-weight: 700;
        }
        QLabel#dashboardTitle {
            color: #0F172A;
            font-size: 24px;
            font-weight: 800;
        }
        QLabel#dashboardMeta {
            color: #64748B;
            font-size: 13px;
        }
        QFrame#dashboardHeroMetric {
            background-color: rgba(255, 255, 255, 28);
            border: 1px solid rgba(255, 255, 255, 45);
            border-radius: 12px;
            min-height: 30px;
            max-height: 30px;
        }
        QLabel#dashboardHeroMetricValue {
            color: #FFFFFF;
            font-size: 15px;
            font-weight: 800;
        }
        QLabel#dashboardHeroMetricLabel {
            color: #E0E7FF;
            font-size: 11px;
            font-weight: 600;
        }
        QLabel#dashboardHeroCloud {
            color: #FFFFFF;
            font-size: 34px;
            font-weight: 700;
        }
        QLabel#dashboardHeroArrows {
            color: #E0E7FF;
            font-size: 21px;
            font-weight: 700;
        }
        QLabel#dashboardHeroServers {
            color: #FFFFFF;
            font-size: 12px;
            font-weight: 700;
        }
        QFrame#dashboardStatCard {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 18px;
            min-height: 102px;
            max-height: 102px;
        }
        QFrame#dashboardStatCard:hover {
            background-color: #FFFFFF;
            border: 1px solid #EEF2FF;
        }
        QLabel#dashboardStatIconBox {
            background-color: #EEF2FF;
            color: #6366F1;
            border-radius: 14px;
            font-size: 20px;
            min-width: 44px;
            max-width: 44px;
            min-height: 44px;
            max-height: 44px;
        }
        QLabel#dashboardStatValue {
            color: #0F172A;
            font-size: 24px;
            font-weight: 800;
        }
        QLabel#dashboardStatLabel {
            color: #64748B;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#dashboardMiniLabel {
            color: #64748B;
            font-size: 12px;
        }
        QFrame#resourceStatusItem {
            background-color: #FFFFFF;
            border: none;
            border-bottom: 1px solid #E2E8F0;
            border-radius: 0;
            min-height: 46px;
            max-height: 46px;
        }
        QFrame#resourceStatusItem:hover {
            background-color: #F8FAFC;
        }
        QLabel#resourceStatusIcon {
            color: #6366F1;
            font-size: 18px;
            min-width: 28px;
            max-width: 28px;
        }
        QLabel#resourceStatusTitle {
            color: #0F172A;
            font-size: 13px;
            font-weight: 700;
        }
        QLabel#resourceStatusMeta {
            color: #64748B;
            font-size: 12px;
        }
        QFrame#dashboardQuickAction {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            min-height: 64px;
            max-height: 64px;
        }
        QFrame#dashboardQuickAction:hover {
            background-color: #EEF2FF;
            border: 1px solid #6366F1;
        }
        QLabel#dashboardQuickIconBox {
            background-color: #EEF2FF;
            color: #6366F1;
            border-radius: 12px;
            font-size: 22px;
            min-width: 42px;
            max-width: 42px;
            min-height: 42px;
            max-height: 42px;
        }
        QLabel#dashboardQuickTitle {
            color: #0F172A;
            font-size: 13px;
            font-weight: 800;
        }
        QLabel#dashboardQuickMeta {
            color: #64748B;
            font-size: 12px;
        }
        QLabel#dashboardQuickArrow {
            color: #94A3B8;
            font-size: 22px;
            font-weight: 600;
            min-width: 14px;
            max-width: 14px;
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
            color: #0F172A;
        }
        QGroupBox#deployConfigBox,
        QGroupBox#dialogFormBox {
            margin-top: 12px;
            margin-bottom: 0;
            padding: 16px 18px 18px 18px;
        }
        QGroupBox#dialogFormBox QStackedWidget,
        QGroupBox#dialogFormBox QLineEdit,
        QGroupBox#dialogFormBox QComboBox,
        QGroupBox#dialogFormBox QSpinBox,
        QWidget#pathPickerRow {
            min-width: 0;
        }
        QFrame#dialogFormPanel {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
        }
        QFrame#dialogHintPanel {
            background-color: #F8FAFC;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
        }
        QWidget#dialogFooter {
            background-color: transparent;
            border-top: 1px solid #E2E8F0;
            padding-top: 4px;
        }
        QScrollArea#dialogScroll {
            background-color: transparent;
            border: none;
        }
        QDialog QLabel#sectionLabel,
        QDialog QLabel#dialogHintTitle {
            margin-top: 0;
            padding: 0;
        }
        QLabel#dialogHintTitle {
            color: #475569;
        }
        QGroupBox#deployConfigBox QLabel,
        QGroupBox#dialogFormBox QLabel {
            color: #64748B;
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
        QFrame#sqlWorkbenchSidebar {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
        }
        QPlainTextEdit#sqlWorkbenchEditor {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            padding: 16px;
            min-height: 160px;
            font-family: "Cascadia Mono", "Consolas", monospace;
            font-size: 13px;
        }
        QLabel#mutedText {
            color: #64748B;
            font-size: 13px;
        }
        QLabel#pageTitle {
            font-size: 18px;
            font-weight: 600;
            color: #0F172A;
            padding: 0;
            margin: 0;
        }
        QLabel#pageSubtitle {
            color: #64748B;
            font-size: 13px;
            padding: 0;
            margin: 0;
        }
        QLabel#serviceStatusBadge {
            color: #0F172A;
            font-weight: 600;
            padding: 8px 12px;
            background-color: #F1F5F9;
            border: 1px solid transparent;
            border-radius: 12px;
        }
        QLabel#sectionLabel {
            color: #0F172A;
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
            color: #0F172A;
            font-weight: 600;
        }
        QLabel#remoteStatusLabel {
            color: #64748B;
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
        QWidget#httpHistoryPanel {
            background-color: #F8FAFC;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
        }
        QWidget#httpWorkspace,
        QWidget#httpResponsePanel {
            background-color: transparent;
        }
        QWidget#httpMainSplitter::handle {
            background-color: #E2E8F0;
            width: 5px;
            margin: 0;
        }
        QLabel#httpSectionTitle {
            color: #334155;
            font-size: 13px;
            font-weight: 600;
            background: transparent;
        }
        QLabel#httpResponseTitle {
            color: #64748B;
            font-size: 12px;
            font-weight: 700;
            letter-spacing: 0.08em;
            background: transparent;
        }
        QLabel#httpMetricLabel {
            color: #64748B;
            font-size: 12px;
            font-weight: 500;
            padding: 4px 8px;
            background: #F1F5F9;
            border-radius: 8px;
        }
        QLabel#httpStatusOk {
            color: #166534;
            font-size: 12px;
            font-weight: 700;
            padding: 4px 10px;
            background: #DCFCE7;
            border-radius: 8px;
            min-width: 36px;
        }
        QLabel#httpStatusError {
            color: #991B1B;
            font-size: 12px;
            font-weight: 700;
            padding: 4px 10px;
            background: #FEE2E2;
            border-radius: 8px;
            min-width: 36px;
        }
        QLabel#httpStatusNeutral {
            color: #475569;
            font-size: 12px;
            font-weight: 600;
            padding: 4px 10px;
            background: #E2E8F0;
            border-radius: 8px;
            min-width: 36px;
        }
        QLineEdit#httpHistorySearch,
        QLineEdit#httpUrlInput {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 6px 10px;
            min-height: 28px;
        }
        QLineEdit#httpHistorySearch:focus,
        QLineEdit#httpUrlInput:focus {
            border-color: #7C3AED;
        }
        QComboBox#httpMethodCombo {
            border: none;
            border-radius: 8px;
            padding: 4px 8px;
            min-height: 28px;
            font-weight: 700;
        }
        QComboBox#httpMethodCombo[httpVerb="GET"] {
            background-color: #DCFCE7;
            color: #166534;
        }
        QComboBox#httpMethodCombo[httpVerb="POST"] {
            background-color: #DBEAFE;
            color: #1D4ED8;
        }
        QComboBox#httpMethodCombo[httpVerb="PUT"] {
            background-color: #FFEDD5;
            color: #C2410C;
        }
        QComboBox#httpMethodCombo[httpVerb="DELETE"] {
            background-color: #FEE2E2;
            color: #B91C1C;
        }
        QComboBox#httpMethodCombo[httpVerb="PATCH"],
        QComboBox#httpMethodCombo[httpVerb="HEAD"] {
            background-color: #F3E8FF;
            color: #7E22CE;
        }
        QPushButton#httpGhostButton {
            background-color: #FFFFFF;
            color: #475569;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 4px 12px;
            min-height: 28px;
        }
        QPushButton#httpGhostButton:hover {
            background-color: #F8FAFC;
            border-color: #CBD5E1;
        }
        QPushButton#httpRowRemoveButton {
            background-color: transparent;
            color: #94A3B8;
            border: none;
            border-radius: 6px;
            padding: 0;
            min-height: 24px;
        }
        QPushButton#httpRowRemoveButton:hover {
            background-color: #FEE2E2;
            color: #B91C1C;
        }
        QWidget#httpCapsuleTabBar {
            background-color: #F5F3FF;
            border: 1px solid #EDE9FE;
            border-radius: 10px;
        }
        QPushButton#httpCapsuleTabButton {
            background-color: transparent;
            color: #64748B;
            border: none;
            border-radius: 8px;
            padding: 4px 12px;
            min-height: 24px;
            font-size: 12px;
            font-weight: 500;
        }
        QPushButton#httpCapsuleTabButton:hover {
            color: #6D28D9;
            background-color: rgba(255, 255, 255, 60);
        }
        QPushButton#httpCapsuleTabButton:checked {
            background-color: #FFFFFF;
            color: #7C3AED;
            font-weight: 600;
        }
        QTableWidget#httpKeyValueTable {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            gridline-color: #F1F5F9;
            alternate-background-color: #F8FAFC;
        }
        QTableWidget#httpKeyValueTable QHeaderView::section {
            background-color: #F8FAFC;
            color: #64748B;
            border: none;
            border-bottom: 1px solid #E2E8F0;
            padding: 8px 10px;
            font-size: 12px;
            font-weight: 600;
        }
        QTableWidget#httpKeyValueTable::item {
            padding: 6px 8px;
            border: none;
        }
        QPlainTextEdit#httpCodeEditor {
            background-color: #FFFFFF;
            color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 10px 12px;
            font-family: "Cascadia Code", "Consolas", "Courier New", monospace;
            font-size: 13px;
            selection-background-color: #EDE9FE;
        }
        QListWidget#httpHistoryList {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 4px;
            outline: none;
        }
        QListWidget#httpHistoryList::item {
            color: #475569;
            border-radius: 8px;
            padding: 4px 8px;
            margin: 1px 0;
            font-size: 12px;
        }
        QListWidget#httpHistoryList::item:hover {
            background-color: #F5F3FF;
        }
        QListWidget#httpHistoryList::item:selected {
            background-color: #EDE9FE;
            color: #0F172A;
        }
        QTreeWidget#jsonTree {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 4px;
        }
        QTreeWidget#jsonTree::item {
            padding: 4px 6px;
            border-radius: 6px;
        }
        QTreeWidget#jsonTree::item:hover {
            background-color: #F1F5F9;
        }
        QTreeWidget#jsonTree::item:selected {
            background-color: #E0E7FF;
            color: #0F172A;
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
            color: #64748B;
            font-size: 14px;
            padding: 56px 32px;
            background: transparent;
            border: 1px dashed #E2E8F0;
            border-radius: 12px;
        }
        QLabel#metricTitle {
            background: transparent;
            color: #64748B;
            font-size: 12px;
            font-weight: 500;
        }
        QLabel#metricValue {
            background: transparent;
            color: #0F172A;
            font-size: 26px;
            font-weight: 600;
        }
        QFrame#metricCard {
            background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
            min-height: 88px;
        }
        QTableWidget {
            background-color: #FFFFFF;
            alternate-background-color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 12px;
            gridline-color: transparent;
        }
        QTableWidget#dashboardTable {
            background-color: #FFFFFF;
            border: 1px solid #EEF2FF;
            border-radius: 12px;
        }
        QTableWidget::item {
            padding: 10px 12px;
            selection-background-color: transparent;
            selection-color: #0F172A;
        }
        QTableWidget::item:hover {
            background-color: #F8FAFC;
        }
        QTableWidget::item:selected {
            background-color: #EEF2FF;
            color: #0F172A;
        }
        QTableWidget::item:focus,
        QTableWidget::item:selected:focus {
            outline: none;
            border: none;
        }
        QHeaderView::section {
            background-color: #EEF2FF;
            color: #64748B;
            padding: 9px 12px;
            border: none;
            font-weight: 600;
            selection-background-color: transparent;
            selection-color: #64748B;
        }
        QListWidget#sidebarNav,
        QListWidget#sidebarNav::item {
            background-clip: padding;
        }
        QLineEdit, QPlainTextEdit {
            background-color: #FFFFFF;
            color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 7px 10px;
            selection-background-color: #E0E7FF;
            selection-color: #0F172A;
        }
        QAbstractItemView#comboPopup {
            background-color: #FFFFFF;
            color: #0F172A;
            selection-background-color: #E0E7FF;
            selection-color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            padding: 6px;
            outline: none;
        }
        QAbstractItemView#comboPopup QWidget {
            background-color: #FFFFFF;
            color: #0F172A;
        }
        QAbstractItemView#comboPopup::item {
            min-height: 30px;
            padding: 6px 14px;
            border-radius: 8px;
        }
        QAbstractItemView#comboPopup::item:hover {
            background-color: #F8FAFC;
        }
        QAbstractItemView#comboPopup::item:selected {
            background-color: #E0E7FF;
            color: #0F172A;
        }
        QLineEdit {
            min-height: 18px;
        }
        QComboBox QLineEdit {
            border: none;
            padding: 0;
            background: transparent;
            selection-background-color: #E0E7FF;
            selection-color: #0F172A;
        }
        QPlainTextEdit {
            background-color: #F1F5F9;
        }
        QPlainTextEdit#terminalOutput,
        QTextEdit#terminalOutput {
            background-color: #07111E;
            color: #D8FFE4;
            border: 1px solid #12263F;
            border-radius: 8px;
            padding: 12px 14px;
            selection-background-color: #245B94;
            selection-color: #FFFFFF;
            font-size: 13px;
        }
        QPlainTextEdit#terminalOutput:disabled,
        QTextEdit#terminalOutput:disabled {
            color: #94A3B8;
        }
        QPlainTextEdit#terminalOutput[text=""],
        QTextEdit#terminalOutput[text=""] {
            color: #D8FFE4;
        }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus {
            border: 1px solid #6366F1;
            outline: none;
        }
        QPlainTextEdit#terminalOutput:focus,
        QTextEdit#terminalOutput:focus {
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
            color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 4px 10px;
            min-height: 16px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #F8FAFC;
            border-color: #CBD5E1;
        }
        QPushButton:pressed {
            background-color: #EEF2FF;
        }
        QPushButton:disabled {
            color: #94A3B8;
            background-color: #F1F5F9;
        }
        QPushButton#primaryButton {
            background-color: #6366F1;
            color: #FFFFFF;
            border: 1px solid transparent;
            font-weight: 600;
        }
        QPushButton#primaryButton:hover {
            background-color: #4F46E5;
            border-color: #4F46E5;
        }
        QPushButton#pathBrowseButton {
            padding: 0 10px;
            min-height: 0;
            min-width: 52px;
            max-height: 32px;
            border-radius: 8px;
        }
        QPushButton#toolBarButton {
            padding: 4px 8px;
            min-height: 16px;
            border-radius: 8px;
            font-size: 12px;
        }
        QPushButton#formActionButton {
            padding: 0 12px;
            min-height: 0;
            min-width: 48px;
            max-height: 32px;
            border-radius: 8px;
        }
        QLabel#secondaryText {
            color: #64748B;
            font-size: 12px;
        }
        QPlainTextEdit#aiChatHistory {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
            padding: 12px;
            color: #0F172A;
        }
        QFrame#aiInputCard {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 14px;
        }
        QPlainTextEdit#aiChatInput {
            background-color: transparent;
            border: none;
            color: #0F172A;
        }
        QLabel#aiAvatar {
            border-radius: 14px;
            background-color: #EEF2FF;
        }
        QPushButton#aiNewChatButton {
            background-color: #F1F5F9;
            color: #475569;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 4px 10px;
            font-weight: 500;
        }
        QPushButton#aiNewChatButton:hover {
            background-color: #E2E8F0;
            color: #1E293B;
        }
        QPushButton#aiActionChip {
            background-color: #F8FAFC;
            color: #475569;
            border: 1px solid #E2E8F0;
            border-radius: 16px;
            padding: 4px 10px;
            font-size: 12px;
        }
        QPushButton#aiActionChip:hover {
            background-color: #EEF2FF;
            color: #4338CA;
            border-color: #C7D2FE;
        }
        QPushButton#aiSendButton {
            background-color: #6366F1;
            color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 18px;
        }
        QPushButton#aiSendButton:hover {
            background-color: #4F46E5;
        }
        QPushButton#aiSendButton:disabled {
            background-color: #C7D2FE;
        }
        QPushButton#aiStopButton {
            background-color: #FFFFFF;
            color: #DC2626;
            border: 1px solid #FECACA;
            border-radius: 18px;
        }
        QPushButton#aiStopButton:hover {
            background-color: #FEF2F2;
        }
        QPushButton#httpSendButton {
            background-color: #7C3AED;
            color: #FFFFFF;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 4px 18px;
            min-height: 28px;
            font-weight: 600;
        }
        QPushButton#httpSendButton:hover {
            background-color: #6D28D9;
            border-color: #6D28D9;
        }
        QPushButton#httpSendButton:disabled {
            background-color: #C4B5FD;
            color: #FFFFFF;
        }
        QPushButton#moduleTabButton {
            background-color: transparent;
            color: #64748B;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 6px 14px;
            min-height: 18px;
            font-weight: 500;
        }
        QPushButton#moduleTabButton:hover {
            color: #6366F1;
            background-color: #F1F5F9;
            border-color: transparent;
        }
        QPushButton#moduleTabButton:checked {
            color: #FFFFFF;
            font-weight: 600;
            background-color: #6366F1;
            border: 1px solid transparent;
        }
        QPushButton#moduleTabButton:checked:hover {
            background-color: #4F46E5;
        }
        QPushButton#lineTabButton {
            background-color: transparent;
            color: #303133;
            border: none;
            border-bottom: 2px solid transparent;
            border-radius: 0;
            padding: 10px 4px;
            min-height: 32px;
            margin-bottom: -1px;
            font-size: 13px;
            font-weight: 400;
        }
        QPushButton#lineTabButton:hover {
            color: #409EFF;
        }
        QPushButton#lineTabButton:checked {
            color: #409EFF;
            font-weight: 500;
            border-bottom: 2px solid #409EFF;
        }
        QPushButton#lineTabButton:checked:hover {
            color: #409EFF;
            background-color: transparent;
        }
        QPushButton#tabButton {
            background-color: transparent;
            color: #64748B;
            border: 1px solid transparent;
            border-radius: 8px;
            padding: 6px 14px;
            min-height: 18px;
            font-weight: 500;
        }
        QPushButton#tabButton:hover {
            color: #6366F1;
            background-color: #F1F5F9;
            border-color: transparent;
        }
        QPushButton#tabButton:checked {
            color: #FFFFFF;
            font-weight: 600;
            background-color: #6366F1;
            border: 1px solid transparent;
        }
        QPushButton#tabButton:checked:hover {
            background-color: #4F46E5;
        }
        QPushButton#backNavButton {
            background-color: #FFFFFF;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 0;
            min-width: 32px;
            max-width: 32px;
            min-height: 32px;
            max-height: 32px;
        }
        QPushButton#backNavButton:hover {
            background-color: #F8FAFC;
            border-color: #CBD5E1;
        }
        QPushButton#backNavButton:pressed {
            background-color: #F1F5F9;
        }
        QPushButton#tableActionView {
            color: #64B5F6;
            background-color: transparent;
            border: none;
            padding: 0 2px;
            min-width: 0;
            min-height: 0;
            font-weight: 500;
        }
        QPushButton#tableActionView:hover {
            color: #42A5F5;
            background-color: transparent;
        }
        QPushButton#tableActionView:pressed {
            color: #1E88E5;
            background-color: transparent;
        }
        QPushButton#tableActionWrite {
            color: #FBC02D;
            background-color: transparent;
            border: none;
            padding: 0 2px;
            min-width: 0;
            min-height: 0;
            font-weight: 500;
        }
        QPushButton#tableActionWrite:hover {
            color: #F9A825;
            background-color: transparent;
        }
        QPushButton#tableActionWrite:pressed {
            color: #F57F17;
            background-color: transparent;
        }
        QPushButton#tableActionDelete {
            color: #F4511E;
            background-color: transparent;
            border: none;
            padding: 0 2px;
            min-width: 0;
            min-height: 0;
            font-weight: 500;
        }
        QPushButton#tableActionDelete:hover {
            color: #E64A19;
            background-color: transparent;
        }
        QPushButton#tableActionDelete:pressed {
            color: #D84315;
            background-color: transparent;
        }
        QLabel#healthTagGreen {
            color: #166534;
            background-color: #DCFCE7;
            border: 1px solid #86EFAC;
            border-radius: 6px;
            padding: 2px 10px;
            font-weight: 500;
        }
        QLabel#healthTagYellow {
            color: #92400E;
            background-color: #FEF3C7;
            border: 1px solid #FCD34D;
            border-radius: 6px;
            padding: 2px 10px;
            font-weight: 500;
        }
        QLabel#healthTagRed {
            color: #991B1B;
            background-color: #FEE2E2;
            border: 1px solid #FCA5A5;
            border-radius: 6px;
            padding: 2px 10px;
            font-weight: 500;
        }
        QPushButton#dangerButton {
            background-color: #FEE2E2;
            color: #991B1B;
            border: 1px solid transparent;
        }
        QPushButton#dangerButton:hover {
            background-color: #FECACA;
            border-color: #EF4444;
        }
        QProgressBar {
            background-color: #F1F5F9;
            border: 1px solid transparent;
            border-radius: 12px;
            text-align: center;
            color: #64748B;
            height: 22px;
        }
        QProgressBar::chunk {
            background-color: #6366F1;
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
            background-color: #F8FAFC;
        }
        QDialog QFormLayout QLabel {
            color: #475569;
            font-weight: 500;
            min-width: 72px;
            padding-right: 4px;
        }
        QInputDialog, QMessageBox {
            background-color: #F8FAFC;
        }
        QInputDialog QLabel, QMessageBox QLabel {
            color: #0F172A;
            background: transparent;
        }
        QInputDialog QLineEdit, QMessageBox QLineEdit {
            background-color: #FFFFFF;
            color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            padding: 8px 10px;
        }
        QMessageBox {
            color: #0F172A;
        }
        QMenu {
            background-color: #FFFFFF;
            color: #0F172A;
            border: 1px solid #E2E8F0;
            border-radius: 10px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 28px 8px 16px;
            border-radius: 6px;
            color: #0F172A;
            background-color: transparent;
        }
        QMenu::item:selected {
            background-color: #EEF2FF;
            color: #0F172A;
        }
        QMenu::item:disabled {
            color: #A3A3A3;
        }
        QMenu::separator {
            height: 1px;
            background: #E2E8F0;
            margin: 4px 8px;
        }
        QProgressBar#remoteUploadProgress {
            background-color: #F1F5F9;
            border: 1px solid #E2E8F0;
            border-radius: 8px;
            text-align: center;
            color: #64748B;
            font-size: 11px;
        }
        QProgressBar#remoteUploadProgress::chunk {
            background-color: #6366F1;
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
            background: #CBD5E1;
            border-radius: 3px;
            min-height: 32px;
        }
        QScrollBar::handle:vertical:hover {
            background: #94A3B8;
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
            background: #CBD5E1;
            border-radius: 3px;
            min-width: 32px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #94A3B8;
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
