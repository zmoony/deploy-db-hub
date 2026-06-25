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

constexpr auto kPageBg = "#F7F8FA";
constexpr auto kCardBg = "#FFFFFF";
constexpr auto kCodeBg = "#F7F8FA";
constexpr auto kText = "#1D2129";
constexpr auto kMutedText = "#86909C";
constexpr auto kPlaceholderText = "#C0C4CC";
constexpr auto kAccent = "#1677FF";
constexpr auto kAccentSoft = "#E6F4FF";
constexpr auto kAccentHover = "#BAE0FF";
constexpr auto kBorder = "#E5E6EB";
constexpr auto kSoftBorder = "#F2F3F5";
constexpr auto kSuccess = "#00B42A";
constexpr auto kWarning = "#FF7D00";
constexpr auto kDanger = "#F53F3F";
constexpr auto kSidebarBg = "#FBFCFD";
constexpr auto kSidebarItemHover = "#F2F3F5";
constexpr auto kSidebarItemSelected = "#E8F3FF";
constexpr auto kSidebarText = "#4E5969";
constexpr auto kSidebarTextSelected = "#1677FF";

constexpr int kRadiusSm = 4;
constexpr int kRadiusMd = 4;
constexpr int kRadiusLg = 6;

bool isPopupSurface(QObject *object)
{
    const QString name = object->objectName();
    return name == QStringLiteral("comboPopup");
}

void applyPopupShadow(QWidget *widget)
{
    if (widget == nullptr || widget->graphicsEffect() != nullptr) {
        return;
    }
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(12);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 15));
    widget->setGraphicsEffect(shadow);
}

void drawChevron(QPainter *painter, const QRect &rect, bool up, bool enabled)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(enabled ? QColor(QString::fromLatin1(kMutedText)) : QColor(QString::fromLatin1(kPlaceholderText)));
    pen.setWidthF(1.5);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);

    const QPointF center = rect.center();
    const qreal half = 3.5;
    QPainterPath path;
    if (up) {
        path.moveTo(center.x() - half, center.y() + 1.0);
        path.lineTo(center.x(), center.y() - 1.5);
        path.lineTo(center.x() + half, center.y() + 1.0);
    } else {
        path.moveTo(center.x() - half, center.y() - 1.0);
        path.lineTo(center.x(), center.y() + 1.5);
        path.lineTo(center.x() + half, center.y() - 1.0);
    }
    painter->drawPath(path);
    painter->restore();
}

void drawInputPanel(QPainter *painter, const QRect &rect, bool hover, bool focused, bool enabled)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const QColor fill = enabled ? QColor(QStringLiteral("#FFFFFF")) : QColor(QString::fromLatin1(kCodeBg));
    QColor stroke;
    if (focused) {
        stroke = QColor(QString::fromLatin1(kAccent));
    } else if (hover) {
        stroke = QColor(QString::fromLatin1(kMutedText));
    } else {
        stroke = QColor(QString::fromLatin1(kBorder));
    }

    QPen pen(stroke);
    pen.setWidthF(focused ? 1.5 : 1.0);
    painter->setPen(pen);
    painter->setBrush(fill);
    painter->drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), kRadiusMd, kRadiusMd);
    painter->restore();
}

void drawControlPill(QPainter *painter, const QRect &rect, bool hover, bool enabled)
{
    Q_UNUSED(hover);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::NoBrush);
    Q_UNUSED(enabled);
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
    popupPalette.setColor(QPalette::Text, QColor(QString::fromLatin1(kText)));
    popupPalette.setColor(QPalette::WindowText, QColor(QString::fromLatin1(kText)));
    popupPalette.setColor(QPalette::ButtonText, QColor(QString::fromLatin1(kText)));
    popupPalette.setColor(QPalette::HighlightedText, QColor(QString::fromLatin1(kText)));
    popupPalette.setColor(QPalette::Highlight, QColor(QString::fromLatin1(kAccentSoft)));
    view->setPalette(popupPalette);
    view->viewport()->setPalette(popupPalette);
    if (QWidget *popupWindow = view->window(); popupWindow != nullptr) {
        popupWindow->setPalette(popupPalette);
        popupWindow->setAutoFillBackground(true);
        popupWindow->setStyleSheet(QStringLiteral("background-color: #FFFFFF; color: %1;").arg(kText));
    }

    view->setStyleSheet(QStringLiteral(R"(
        QAbstractItemView {
            background-color: #FFFFFF;
            color: %1;
            selection-background-color: %2;
            selection-color: %1;
            border: 1px solid %3;
            border-radius: %4px;
            padding: 4px;
            outline: none;
        }
        QAbstractItemView::item {
            background-color: #FFFFFF;
            color: %1;
            min-height: 28px;
            padding: 4px 10px;
            border-radius: %5px;
        }
        QAbstractItemView::item:hover {
            background-color: %6;
            color: %1;
        }
        QAbstractItemView::item:selected {
            background-color: %2;
            color: %1;
        }
    )").arg(kText, kAccentSoft, kBorder, QString::number(kRadiusMd), QString::number(kRadiusSm), kSoftBorder));
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
            return QRect(rect.right() - 28, rect.top(), 28, rect.height());
        }
        if (control == QStyle::CC_ComboBox && subControl == QStyle::SC_ComboBoxEditField) {
            return option->rect.adjusted(8, 1, -30, -1);
        }
        if (control == QStyle::CC_SpinBox) {
            QRect rect = option->rect;
            const QRect buttonRect(rect.right() - 28, rect.top(), 28, rect.height());
            if (subControl == QStyle::SC_SpinBoxUp) {
                return QRect(buttonRect.left(), buttonRect.top(), buttonRect.width(), buttonRect.height() / 2);
            }
            if (subControl == QStyle::SC_SpinBoxDown) {
                return QRect(buttonRect.left(), buttonRect.center().y(), buttonRect.width(), buttonRect.height() / 2);
            }
            if (subControl == QStyle::SC_SpinBoxEditField) {
                return rect.adjusted(8, 1, -30, -1);
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
            drawChevron(painter, arrowRect.adjusted(0, 1, -2, 0), false, enabled);
            return;
        }

        if (control == QStyle::CC_SpinBox) {
            const bool hover = option->state & QStyle::State_MouseOver;
            const bool focused = option->state & QStyle::State_HasFocus;
            const bool enabled = option->state & QStyle::State_Enabled;
            drawInputPanel(painter, option->rect, hover, focused, enabled);
            const QRect upRect = subControlRect(control, option, QStyle::SC_SpinBoxUp, widget);
            const QRect downRect = subControlRect(control, option, QStyle::SC_SpinBoxDown, widget);
            drawChevron(painter, upRect.adjusted(0, 1, -2, 0), true, enabled);
            drawChevron(painter, downRect.adjusted(0, -1, -2, 0), false, enabled);
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
            if (auto *widget = qobject_cast<QWidget *>(watched); widget != nullptr && isPopupSurface(widget)) {
                applyPopupShadow(widget);
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
        * {
            font-family: "Segoe UI", "Microsoft YaHei UI", "Inter", sans-serif;
            font-size: 13px;
            letter-spacing: 0;
            outline: none;
        }
        QMainWindow {
            background-color: %1;
            color: %2;
        }
        QMainWindow > QWidget {
            background-color: %1;
        }
        QWidget {
            background: transparent;
            color: %2;
        }
        QStatusBar {
            background-color: %1;
            color: %3;
            border: none;
            border-top: 1px solid %4;
            padding: 4px 12px;
            selection-background-color: transparent;
            selection-color: %3;
            font-size: 12px;
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
            padding: 4px 0;
            outline: none;
        }
        QFrame#sidebarNavPanel {
            background-color: %15;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QWidget#sidebarBrand {
            background-color: transparent;
            min-height: 40px;
        }
        QLabel#sidebarBrandMark {
            background-color: %6;
            color: #FFFFFF;
            border-radius: %5px;
            min-width: 26px;
            max-width: 26px;
            min-height: 26px;
            max-height: 26px;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#sidebarBrandTitle {
            color: %2;
            font-size: 13px;
            font-weight: 600;
        }
        QFrame#sidebarDivider {
            border: none;
            background-color: %4;
            min-height: 1px;
            max-height: 1px;
        }
        QWidget#sidebarFooter {
            background: transparent;
        }
        QLabel#sidebarFooterText {
            color: %3;
            padding: 4px 8px;
            border-radius: %5px;
            font-weight: 400;
            font-size: 12px;
        }
        QPushButton#sidebarFooterButton {
            background-color: transparent;
            color: %16;
            border: none;
            padding: 6px 8px;
            border-radius: %5px;
            font-weight: 400;
            text-align: left;
            font-size: 13px;
        }
        QPushButton#sidebarFooterButton:hover {
            background-color: %17;
            color: %2;
        }
        QPushButton#sidebarFooterButton:checked {
            background-color: %18;
            color: %19;
            font-weight: 500;
        }
        QLabel#sidebarUserText {
            color: %3;
            padding: 6px 8px 2px 8px;
            font-size: 11px;
        }
        QScrollArea#contentScroll {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QListWidget#toolListNav {
            background-color: %1;
            border: none;
            border-radius: 0;
            padding: 4px;
            outline: none;
        }
        QListWidget#sidebarNav::item {
            background-color: transparent;
            color: %16;
            border-radius: %5px;
            padding: 6px 10px;
            margin: 1px 2px;
            selection-background-color: transparent;
            selection-color: %16;
            font-size: 13px;
        }
        QListWidget#sidebarNav::item:selected {
            background-color: %18;
            color: %19;
            border: none;
            font-weight: 500;
        }
        QListWidget#sidebarNav::item:hover {
            background-color: %17;
            color: %2;
        }
        QListWidget#toolListNav::item {
            background-color: transparent;
            color: %3;
            border-radius: %5px;
            padding: 6px 8px;
            margin: 1px 0;
            selection-background-color: transparent;
            selection-color: %3;
            font-size: 13px;
        }
        QListWidget#toolListNav::item:selected {
            background-color: %7;
            color: %6;
            font-weight: 500;
        }
        QListWidget#toolListNav::item:hover {
            background-color: %8;
        }
        QFrame#contentPanel {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QWidget#dashboardTabBar,
        QWidget#lineTabBar {
            background-color: transparent;
            border: none;
            border-bottom: 1px solid %4;
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
            border: 1px solid %4;
            border-radius: %5px;
        }
        QWidget#dashboardHero {
            background-color: %7;
            border: 1px solid %10;
            border-radius: %9px;
            min-height: 88px;
            max-height: 88px;
        }
        QLabel#dashboardHeroImage {
            background-color: transparent;
            border: none;
        }
        QWidget#dashboardHeroVisual {
            background-color: %8;
            border-radius: %5px;
            min-width: 240px;
            min-height: 64px;
            max-height: 64px;
        }
        QWidget#serviceInstanceBanner {
            background-color: %7;
            border-color: %10;
        }
        QLabel#serviceInstanceTitle {
            color: %2;
            font-size: 14px;
            font-weight: 600;
        }
        QLabel#serviceInstanceStatusOk {
            color: #00874D;
            font-weight: 500;
            padding: 2px 8px;
            background-color: #D4F1DC;
            border-radius: %11px;
            font-size: 12px;
        }
        QLabel#serviceInstanceStatusBad {
            color: #CB2634;
            font-weight: 500;
            padding: 2px 8px;
            background-color: #FFECE8;
            border-radius: %11px;
            font-size: 12px;
        }
        QLabel#serviceInstanceNode {
            color: %3;
            font-size: 12px;
        }
        QLabel#serviceParentTag {
            color: %2;
            font-weight: 500;
            padding: 4px 8px;
            background-color: %8;
            border-radius: %5px;
            font-size: 12px;
        }
        QGroupBox#serviceHintBox {
            border: 1px solid %4;
        }
        QWidget#toolPlaceholderBox {
            background-color: %1;
            border: 1px dashed %4;
            border-radius: %5px;
            padding: 12px;
        }
        QLabel#toolMessage {
            color: %3;
            font-size: 12px;
        }
        QGroupBox#serviceHintBox::title {
            color: %12;
            font-weight: 600;
        }
        QLabel#serviceHintText {
            color: %3;
            font-size: 12px;
            line-height: 1.5;
        }
        QLabel#dashboardKicker {
            color: %3;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 0.5px;
        }
        QLabel#dashboardTitle {
            color: %2;
            font-size: 18px;
            font-weight: 700;
        }
        QLabel#dashboardMeta {
            color: %3;
            font-size: 12px;
        }
        QFrame#dashboardHeroMetric {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %11px;
            min-height: 24px;
            max-height: 24px;
        }
        QLabel#dashboardHeroMetricValue {
            color: %6;
            font-size: 12px;
            font-weight: 700;
        }
        QLabel#dashboardHeroMetricLabel {
            color: %3;
            font-size: 11px;
            font-weight: 500;
        }
        QLabel#dashboardHeroCloud {
            color: %6;
            font-size: 24px;
            font-weight: 700;
        }
        QLabel#dashboardHeroArrows {
            color: %10;
            font-size: 16px;
            font-weight: 600;
        }
        QLabel#dashboardHeroServers {
            color: %3;
            font-size: 11px;
            font-weight: 600;
        }
        QFrame#dashboardStatCard {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            min-height: 88px;
            max-height: 88px;
        }
        QFrame#dashboardStatCard:hover {
            background-color: #FFFFFF;
            border-color: %10;
        }
        QLabel#dashboardStatIconBox {
            background-color: %7;
            color: %6;
            border-radius: %5px;
            font-size: 14px;
            min-width: 32px;
            max-width: 32px;
            min-height: 32px;
            max-height: 32px;
        }
        QLabel#dashboardStatValue {
            color: %2;
            font-size: 20px;
            font-weight: 700;
        }
        QLabel#dashboardStatLabel {
            color: %3;
            font-size: 12px;
            font-weight: 500;
        }
        QLabel#dashboardMiniLabel {
            color: %3;
            font-size: 12px;
        }
        QFrame#resourceStatusItem {
            background-color: #FFFFFF;
            border: none;
            border-bottom: 1px solid %8;
            border-radius: 0;
            min-height: 40px;
            max-height: 40px;
        }
        QFrame#resourceStatusItem:hover {
            background-color: %8;
        }
        QLabel#resourceStatusIcon {
            color: %6;
            font-size: 16px;
            min-width: 24px;
            max-width: 24px;
        }
        QLabel#resourceStatusTitle {
            color: %2;
            font-size: 13px;
            font-weight: 500;
        }
        QLabel#resourceStatusMeta {
            color: %3;
            font-size: 12px;
        }
        QFrame#dashboardQuickAction {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            min-height: 56px;
            max-height: 56px;
        }
        QFrame#dashboardQuickAction:hover {
            background-color: %7;
            border-color: %10;
        }
        QLabel#dashboardQuickIconBox {
            background-color: %7;
            color: %6;
            border-radius: %5px;
            font-size: 14px;
            min-width: 32px;
            max-width: 32px;
            min-height: 32px;
            max-height: 32px;
        }
        QLabel#dashboardQuickTitle {
            color: %2;
            font-size: 13px;
            font-weight: 600;
        }
        QLabel#dashboardQuickMeta {
            color: %3;
            font-size: 12px;
        }
        QLabel#dashboardQuickArrow {
            color: %13;
            font-size: 18px;
            font-weight: 500;
            min-width: 12px;
            max-width: 12px;
        }
        QGroupBox {
            background-color: transparent;
            border: none;
            border-radius: 0;
            margin-top: 0;
            padding: 0;
            font-weight: 400;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 0;
            top: 0;
            padding: 0 0 8px 0;
            color: %2;
            font-size: 14px;
            font-weight: 600;
        }
        QGroupBox#deployConfigBox,
        QGroupBox#dialogFormBox {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            margin-top: 0;
            margin-bottom: 0;
            padding: 12px 14px 14px 14px;
        }
        QGroupBox#deployConfigBox::title,
        QGroupBox#dialogFormBox::title {
            subcontrol-origin: margin;
            left: 10px;
            top: -8px;
            padding: 0 6px;
            background-color: #FFFFFF;
            color: %2;
            font-size: 13px;
            font-weight: 600;
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
            border: 1px solid %4;
            border-radius: %5px;
        }
        QFrame#dialogHintPanel {
            background-color: %1;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QWidget#dialogFooter {
            background-color: transparent;
            border-top: 1px solid %4;
            padding-top: 12px;
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
            color: %3;
        }
        QGroupBox#deployConfigBox QLabel,
        QGroupBox#dialogFormBox QLabel {
            color: %2;
            font-weight: 400;
            min-width: 60px;
            font-size: 13px;
        }
        QFrame#deployExecBox {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QPlainTextEdit#deployLog {
            min-height: 200px;
        }
        QFrame#sqlWorkbenchSidebar {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QPlainTextEdit#sqlWorkbenchEditor {
            background-color: %1;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 12px;
            min-height: 160px;
            font-family: "Cascadia Mono", "Consolas", monospace;
            font-size: 13px;
        }
        QLabel#mutedText {
            color: %3;
            font-size: 13px;
        }
        QLabel#pageTitle {
            font-size: 16px;
            font-weight: 600;
            color: %2;
            padding: 0;
            margin: 0;
        }
        QLabel#pageSubtitle {
            color: %3;
            font-size: 13px;
            padding: 0;
            margin: 0;
        }
        QLabel#serviceStatusBadge {
            color: %2;
            font-weight: 500;
            padding: 4px 10px;
            background-color: %8;
            border: 1px solid %4;
            border-radius: %11px;
            font-size: 12px;
        }
        QLabel#sectionLabel {
            color: %2;
            font-weight: 600;
            font-size: 14px;
            padding: 0;
            margin: 0;
        }
        QFrame#remoteFileHeader {
            background-color: %8;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QLabel#remoteConnectionLabel {
            color: %2;
            font-weight: 500;
        }
        QLabel#remoteStatusLabel {
            color: %3;
            font-size: 12px;
            padding: 0;
        }
        QWidget#terminalTabToolbar {
            background-color: transparent;
            min-height: 28px;
        }
        QLabel#terminalToolbarTitle {
            color: %2;
            font-size: 12px;
            font-weight: 600;
            padding: 0;
            margin: 0;
        }
        QPushButton#terminalToolbarButton {
            min-height: 24px;
            padding: 3px 10px;
            border-radius: %11px;
            background-color: #FFFFFF;
            border: 1px solid %4;
            font-size: 12px;
        }
        QPushButton#terminalToolbarButton:hover {
            color: %6;
            background-color: %7;
            border-color: %10;
        }
        QLabel#remoteStatusDot {
            min-width: 6px;
            max-width: 6px;
            min-height: 6px;
            max-height: 6px;
            border-radius: 3px;
            margin-top: 2px;
        }
        QLabel#remoteStatusDot[level="ok"] {
            background-color: %12;
        }
        QLabel#remoteStatusDot[level="error"] {
            background-color: %14;
        }
        QLabel#remoteStatusDot[level="pending"] {
            background-color: %13;
        }
        QTreeWidget#remoteDirTree {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px;
        }
        QTreeWidget#remoteDirTree::item {
            color: %2;
            border-radius: %11px;
            padding: 4px 6px;
            margin: 1px 0;
        }
        QTreeWidget#remoteDirTree::item:hover {
            background-color: %8;
        }
        QTreeWidget#remoteDirTree::item:selected {
            background-color: %7;
            color: %6;
        }
        QTableWidget#remoteFileTable {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QTabWidget#remoteOperationTabs::pane {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            top: -1px;
        }
        QTabWidget#remoteOperationTabs::tab-bar {
            alignment: left;
        }
        QTabBar::tab {
            background-color: %1;
            color: %3;
            border: 1px solid %4;
            border-bottom: none;
            border-top-left-radius: %5px;
            border-top-right-radius: %5px;
            padding: 6px 12px;
            margin-right: 2px;
            min-height: 16px;
            font-size: 13px;
        }
        QTabBar::tab:selected {
            background-color: #FFFFFF;
            color: %2;
            font-weight: 500;
            border-color: %4;
            border-bottom-color: #FFFFFF;
        }
        QTabBar::tab:hover {
            color: %6;
        }
        QWidget#httpHistoryPanel {
            background-color: %1;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QWidget#httpWorkspace,
        QWidget#httpResponsePanel {
            background-color: transparent;
        }
        QWidget#httpMainSplitter::handle {
            background-color: %4;
            width: 1px;
            margin: 0;
        }
        QLabel#httpSectionTitle {
            color: %2;
            font-size: 13px;
            font-weight: 600;
            background: transparent;
        }
        QLabel#httpResponseTitle {
            color: %3;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 0.5px;
            background: transparent;
        }
        QLabel#httpMetricLabel {
            color: %3;
            font-size: 12px;
            font-weight: 400;
            padding: 2px 8px;
            background: %8;
            border-radius: %11px;
        }
        QLabel#httpStatusOk {
            color: #006435;
            font-size: 12px;
            font-weight: 600;
            padding: 2px 8px;
            background: #D4F1DC;
            border-radius: %11px;
            min-width: 36px;
        }
        QLabel#httpStatusError {
            color: #CB2634;
            font-size: 12px;
            font-weight: 600;
            padding: 2px 8px;
            background: #FFECE8;
            border-radius: %11px;
            min-width: 36px;
        }
        QLabel#httpStatusNeutral {
            color: %2;
            font-size: 12px;
            font-weight: 500;
            padding: 2px 8px;
            background: %8;
            border-radius: %11px;
            min-width: 36px;
        }
        QLineEdit#httpHistorySearch,
        QLineEdit#httpUrlInput {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 6px 10px;
            min-height: 20px;
        }
        QLineEdit#httpHistorySearch:focus,
        QLineEdit#httpUrlInput:focus {
            border-color: %6;
        }
        QComboBox#httpMethodCombo {
            border: none;
            border-radius: %5px;
            padding: 4px 8px;
            min-height: 20px;
            font-weight: 600;
            font-size: 12px;
        }
        QComboBox#httpMethodCombo[httpVerb="GET"] {
            background-color: #D4F1DC;
            color: #006435;
        }
        QComboBox#httpMethodCombo[httpVerb="POST"] {
            background-color: #DBEAFF;
            color: #0E42D2;
        }
        QComboBox#httpMethodCombo[httpVerb="PUT"] {
            background-color: #FFF3E8;
            color: #D35400;
        }
        QComboBox#httpMethodCombo[httpVerb="DELETE"] {
            background-color: #FFECE8;
            color: #CB2634;
        }
        QComboBox#httpMethodCombo[httpVerb="PATCH"],
        QComboBox#httpMethodCombo[httpVerb="HEAD"] {
            background-color: #F5E8FF;
            color: #722ED1;
        }
        QPushButton#httpGhostButton {
            background-color: #FFFFFF;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px 12px;
            min-height: 20px;
        }
        QPushButton#httpGhostButton:hover {
            color: %6;
            background-color: %7;
            border-color: %10;
        }
        QPushButton#httpRowRemoveButton {
            background-color: transparent;
            color: %13;
            border: none;
            border-radius: %11px;
            padding: 0;
            min-height: 20px;
        }
        QPushButton#httpRowRemoveButton:hover {
            background-color: #FFECE8;
            color: %14;
        }
        QWidget#httpCapsuleTabBar {
            background-color: %1;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QPushButton#httpCapsuleTabButton {
            background-color: transparent;
            color: %3;
            border: none;
            border-radius: %11px;
            padding: 4px 12px;
            min-height: 20px;
            font-size: 12px;
            font-weight: 400;
        }
        QPushButton#httpCapsuleTabButton:hover {
            color: %6;
            background-color: %8;
        }
        QPushButton#httpCapsuleTabButton:checked {
            background-color: #FFFFFF;
            color: %6;
            font-weight: 500;
        }
        QTableWidget#httpKeyValueTable {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            gridline-color: %8;
            alternate-background-color: %1;
        }
        QTableWidget#httpKeyValueTable QHeaderView::section {
            background-color: %8;
            color: %3;
            border: none;
            border-bottom: 1px solid %4;
            padding: 6px 8px;
            font-size: 12px;
            font-weight: 500;
        }
        QTableWidget#httpKeyValueTable::item {
            padding: 4px 6px;
            border: none;
        }
        QPlainTextEdit#httpCodeEditor {
            background-color: %1;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 10px;
            font-family: "Cascadia Code", "Consolas", "Courier New", monospace;
            font-size: 13px;
            selection-background-color: %7;
        }
        QListWidget#httpHistoryList {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px;
            outline: none;
        }
        QListWidget#httpHistoryList::item {
            color: %3;
            border-radius: %11px;
            padding: 4px 8px;
            margin: 1px 0;
            font-size: 12px;
        }
        QListWidget#httpHistoryList::item:hover {
            background-color: %8;
        }
        QListWidget#httpHistoryList::item:selected {
            background-color: %7;
            color: %6;
        }
        QTreeWidget#jsonTree {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px;
        }
        QTreeWidget#jsonTree::item {
            padding: 4px 6px;
            border-radius: %11px;
        }
        QTreeWidget#jsonTree::item:hover {
            background-color: %8;
        }
        QTreeWidget#jsonTree::item:selected {
            background-color: %7;
            color: %6;
        }
        QSplitter::handle {
            background-color: transparent;
            width: 6px;
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
            color: %3;
            font-size: 13px;
            padding: 40px 24px;
            background: transparent;
            border: 1px dashed %4;
            border-radius: %5px;
        }
        QLabel#metricTitle {
            background: transparent;
            color: %3;
            font-size: 12px;
            font-weight: 400;
        }
        QLabel#metricValue {
            background: transparent;
            color: %2;
            font-size: 22px;
            font-weight: 600;
        }
        QFrame#metricCard {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            min-height: 76px;
        }
        QTableWidget {
            background-color: #FFFFFF;
            alternate-background-color: %1;
            border: 1px solid %4;
            border-radius: %5px;
            gridline-color: %8;
        }
        QTableWidget#dashboardTable {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QTableWidget::item {
            padding: 6px 10px;
            selection-background-color: transparent;
            selection-color: %2;
            border-bottom: 1px solid %8;
        }
        QTableWidget::item:hover {
            background-color: %8;
        }
        QTableWidget::item:selected {
            background-color: %7;
            color: %6;
        }
        QTableWidget::item:focus,
        QTableWidget::item:selected:focus {
            outline: none;
            border: none;
        }
        QHeaderView::section {
            background-color: %8;
            color: %3;
            padding: 7px 10px;
            border: none;
            border-bottom: 1px solid %4;
            font-weight: 500;
            font-size: 12px;
            selection-background-color: transparent;
            selection-color: %3;
        }
        QListWidget#sidebarNav,
        QListWidget#sidebarNav::item {
            background-clip: padding;
        }
        QLineEdit, QPlainTextEdit, QTextEdit {
            background-color: #FFFFFF;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 5px 8px;
            selection-background-color: %7;
            selection-color: %2;
        }
        QAbstractItemView#comboPopup {
            background-color: #FFFFFF;
            color: %2;
            selection-background-color: %7;
            selection-color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px;
            outline: none;
        }
        QAbstractItemView#comboPopup QWidget {
            background-color: #FFFFFF;
            color: %2;
        }
        QAbstractItemView#comboPopup::item {
            min-height: 28px;
            padding: 4px 10px;
            border-radius: %11px;
        }
        QAbstractItemView#comboPopup::item:hover {
            background-color: %8;
        }
        QAbstractItemView#comboPopup::item:selected {
            background-color: %7;
            color: %6;
        }
        QLineEdit {
            min-height: 18px;
        }
        QComboBox QLineEdit {
            border: none;
            padding: 0;
            background: transparent;
            selection-background-color: %7;
            selection-color: %2;
        }
        QPlainTextEdit {
            background-color: %1;
        }
        QPlainTextEdit#terminalOutput,
        QTextEdit#terminalOutput {
            background-color: #141414;
            color: #E6E6E6;
            border: 1px solid #303030;
            border-radius: %5px;
            padding: 10px 12px;
            selection-background-color: #264F78;
            selection-color: #FFFFFF;
            font-size: 13px;
        }
        QPlainTextEdit#terminalOutput:disabled,
        QTextEdit#terminalOutput:disabled {
            color: %13;
        }
        QPlainTextEdit#terminalOutput[text=""],
        QTextEdit#terminalOutput[text=""] {
            color: #E6E6E6;
        }
        QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus {
            border: 1px solid %6;
            outline: none;
        }
        QPlainTextEdit#terminalOutput:focus,
        QTextEdit#terminalOutput:focus {
            border: 1px solid %12;
            padding: 10px 12px;
            outline: none;
        }
        QPlainTextEdit#remoteFileViewer {
            background-color: #1D1D1D;
            color: #E6E6E6;
            border: 1px solid #303030;
            border-radius: %5px;
            padding: 10px 12px;
            selection-background-color: #264F78;
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
            border: none;
        }
        QPushButton {
            background-color: #FFFFFF;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px 12px;
            min-height: 16px;
            font-size: 13px;
        }
        QPushButton:hover {
            color: %6;
            border-color: %10;
            background-color: #FFFFFF;
        }
        QPushButton:pressed {
            background-color: %7;
            border-color: %6;
        }
        QPushButton:disabled {
            color: %13;
            background-color: %1;
            border-color: %8;
        }
        QPushButton#primaryButton {
            background-color: %6;
            color: #FFFFFF;
            border: 1px solid %6;
            font-weight: 500;
        }
        QPushButton#primaryButton:hover {
            background-color: #4096FF;
            border-color: #4096FF;
            color: #FFFFFF;
        }
        QPushButton#primaryButton:pressed {
            background-color: #0958D9;
            border-color: #0958D9;
        }
        QPushButton#primaryButton:disabled {
            background-color: %10;
            border-color: %10;
            color: #FFFFFF;
        }
        QPushButton#pathBrowseButton {
            padding: 0 10px;
            min-height: 0;
            min-width: 48px;
            max-height: 30px;
            border-radius: %5px;
        }
        QPushButton#toolBarButton {
            padding: 4px 8px;
            min-height: 16px;
            border-radius: %5px;
            font-size: 12px;
        }
        QPushButton#formActionButton {
            padding: 0 12px;
            min-height: 0;
            min-width: 48px;
            max-height: 30px;
            border-radius: %5px;
        }
        QLabel#secondaryText {
            color: %3;
            font-size: 12px;
        }
        QPlainTextEdit#aiChatHistory {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 12px;
            color: %2;
        }
        QFrame#aiInputCard {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
        }
        QPlainTextEdit#aiChatInput {
            background-color: transparent;
            border: none;
            color: %2;
        }
        QLabel#aiAvatar {
            border-radius: %5px;
            background-color: %7;
        }
        QPushButton#aiNewChatButton {
            background-color: %8;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px 10px;
            font-weight: 400;
        }
        QPushButton#aiNewChatButton:hover {
            background-color: %4;
            color: %2;
        }
        QPushButton#aiActionChip {
            background-color: #FFFFFF;
            color: %3;
            border: 1px solid %4;
            border-radius: 9px;
            padding: 3px 10px;
            font-size: 12px;
        }
        QPushButton#aiActionChip:hover {
            color: %6;
            border-color: %10;
            background-color: %7;
        }
        QPushButton#aiSendButton {
            background-color: %6;
            color: #FFFFFF;
            border: 1px solid %6;
            border-radius: %5px;
        }
        QPushButton#aiSendButton:hover {
            background-color: #4096FF;
            border-color: #4096FF;
        }
        QPushButton#aiSendButton:disabled {
            background-color: %10;
            border-color: %10;
        }
        QPushButton#aiStopButton {
            background-color: #FFFFFF;
            color: %14;
            border: 1px solid #FFD4D0;
            border-radius: %5px;
        }
        QPushButton#aiStopButton:hover {
            background-color: #FFF1F0;
        }
        QPushButton#httpSendButton {
            background-color: %6;
            color: #FFFFFF;
            border: 1px solid %6;
            border-radius: %5px;
            padding: 5px 16px;
            min-height: 20px;
            font-weight: 500;
        }
        QPushButton#httpSendButton:hover {
            background-color: #4096FF;
            border-color: #4096FF;
        }
        QPushButton#httpSendButton:disabled {
            background-color: %10;
            border-color: %10;
            color: #FFFFFF;
        }
        QPushButton#moduleTabButton {
            background-color: transparent;
            color: %3;
            border: 1px solid transparent;
            border-radius: %5px;
            padding: 6px 12px;
            min-height: 16px;
            font-weight: 400;
        }
        QPushButton#moduleTabButton:hover {
            color: %6;
            background-color: %8;
            border-color: transparent;
        }
        QPushButton#moduleTabButton:checked {
            color: #FFFFFF;
            font-weight: 500;
            background-color: %6;
            border: 1px solid %6;
        }
        QPushButton#moduleTabButton:checked:hover {
            background-color: #4096FF;
            border-color: #4096FF;
        }
        QPushButton#lineTabButton {
            background-color: transparent;
            color: %3;
            border: none;
            border-bottom: 2px solid transparent;
            border-radius: 0;
            padding: 8px 4px;
            min-height: 28px;
            margin-bottom: -1px;
            font-size: 13px;
            font-weight: 400;
        }
        QPushButton#lineTabButton:hover {
            color: %6;
        }
        QPushButton#lineTabButton:checked {
            color: %6;
            font-weight: 500;
            border-bottom: 2px solid %6;
        }
        QPushButton#lineTabButton:checked:hover {
            color: %6;
            background-color: transparent;
        }
        QPushButton#tabButton {
            background-color: transparent;
            color: %3;
            border: 1px solid transparent;
            border-radius: %5px;
            padding: 6px 12px;
            min-height: 16px;
            font-weight: 400;
        }
        QPushButton#tabButton:hover {
            color: %6;
            background-color: %8;
            border-color: transparent;
        }
        QPushButton#tabButton:checked {
            color: #FFFFFF;
            font-weight: 500;
            background-color: %6;
            border: 1px solid %6;
        }
        QPushButton#tabButton:checked:hover {
            background-color: #4096FF;
            border-color: #4096FF;
        }
        QPushButton#backNavButton {
            background-color: #FFFFFF;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 0;
            min-width: 28px;
            max-width: 28px;
            min-height: 28px;
            max-height: 28px;
        }
        QPushButton#backNavButton:hover {
            color: %6;
            border-color: %10;
            background-color: #FFFFFF;
        }
        QPushButton#backNavButton:pressed {
            background-color: %8;
        }
        QPushButton#tableActionView {
            color: %6;
            background-color: transparent;
            border: none;
            padding: 0 4px;
            min-width: 0;
            min-height: 0;
            font-weight: 400;
        }
        QPushButton#tableActionView:hover {
            color: #4096FF;
            background-color: transparent;
        }
        QPushButton#tableActionWrite {
            color: #FF7D00;
            background-color: transparent;
            border: none;
            padding: 0 4px;
            min-width: 0;
            min-height: 0;
            font-weight: 400;
        }
        QPushButton#tableActionWrite:hover {
            color: #D35400;
            background-color: transparent;
        }
        QPushButton#tableActionDelete {
            color: %14;
            background-color: transparent;
            border: none;
            padding: 0 4px;
            min-width: 0;
            min-height: 0;
            font-weight: 400;
        }
        QPushButton#tableActionDelete:hover {
            color: #CB2634;
            background-color: transparent;
        }
        QLabel#healthTagGreen {
            color: #006435;
            background-color: #D4F1DC;
            border: 1px solid #A9E0B8;
            border-radius: %11px;
            padding: 2px 8px;
            font-weight: 500;
            font-size: 12px;
        }
        QLabel#healthTagYellow {
            color: #A35200;
            background-color: #FFF3E8;
            border: 1px solid #FFD4B8;
            border-radius: %11px;
            padding: 2px 8px;
            font-weight: 500;
            font-size: 12px;
        }
        QLabel#healthTagRed {
            color: #CB2634;
            background-color: #FFECE8;
            border: 1px solid #FFCCC7;
            border-radius: %11px;
            padding: 2px 8px;
            font-weight: 500;
            font-size: 12px;
        }
        QPushButton#dangerButton {
            background-color: #FFF1F0;
            color: %14;
            border: 1px solid #FFCCC7;
        }
        QPushButton#dangerButton:hover {
            background-color: #FFECE8;
            border-color: %14;
            color: #CB2634;
        }
        QProgressBar {
            background-color: %8;
            border: 1px solid transparent;
            border-radius: %11px;
            text-align: center;
            color: %3;
            height: 18px;
            font-size: 11px;
        }
        QProgressBar::chunk {
            background-color: %6;
            border-radius: %11px;
        }
        QToolBar#pageToolbar {
            background: transparent;
            border: none;
            spacing: 8px;
            padding: 0;
            margin: 0;
        }
        QDialog {
            background-color: #FFFFFF;
        }
        QDialog QFormLayout QLabel {
            color: %2;
            font-weight: 400;
            min-width: 68px;
            padding-right: 4px;
        }
        QInputDialog, QMessageBox {
            background-color: #FFFFFF;
        }
        QInputDialog QLabel, QMessageBox QLabel {
            color: %2;
            background: transparent;
        }
        QInputDialog QLineEdit, QMessageBox QLineEdit {
            background-color: #FFFFFF;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 6px 10px;
        }
        QMessageBox {
            color: %2;
        }
        QMenu {
            background-color: #FFFFFF;
            color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 20px 6px 12px;
            border-radius: %11px;
            color: %2;
            background-color: transparent;
        }
        QMenu::item:selected {
            background-color: %7;
            color: %6;
        }
        QMenu::item:disabled {
            color: %13;
        }
        QMenu::separator {
            height: 1px;
            background: %8;
            margin: 2px 6px;
        }
        QProgressBar#remoteUploadProgress {
            background-color: %8;
            border: 1px solid %4;
            border-radius: %5px;
            text-align: center;
            color: %3;
            font-size: 11px;
        }
        QProgressBar#remoteUploadProgress::chunk {
            background-color: %6;
            border-radius: %5px;
        }
        QDialogButtonBox {
            background: transparent;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 2px 0;
        }
        QScrollBar::handle:vertical {
            background: #C9CDD4;
            border-radius: 4px;
            min-height: 28px;
        }
        QScrollBar::handle:vertical:hover {
            background: #A8ABB2;
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
            height: 8px;
            margin: 0 2px;
        }
        QScrollBar::handle:horizontal {
            background: #C9CDD4;
            border-radius: 4px;
            min-width: 28px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #A8ABB2;
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal,
        QScrollBar::add-page:horizontal,
        QScrollBar::sub-page:horizontal {
            background: transparent;
            height: 0;
            width: 0;
        }
        QCheckBox {
            color: %2;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %4;
            border-radius: %11px;
            background-color: #FFFFFF;
        }
        QCheckBox::indicator:hover {
            border-color: %10;
        }
        QCheckBox::indicator:checked {
            background-color: %6;
            border-color: %6;
            image: none;
        }
    )").arg(kPageBg,
           kText,
           kMutedText,
           kBorder,
           QString::number(kRadiusSm),
           kAccent,
           kAccentSoft,
           kSoftBorder,
           QString::number(kRadiusLg),
           kAccentHover,
           QString::number(kRadiusSm / 2),
           kSuccess,
           kPlaceholderText,
           kWarning,
           kDanger,
           kSidebarBg,
           kSidebarText,
           kSidebarItemHover,
           kSidebarItemSelected,
           kSidebarTextSelected);
}

}
