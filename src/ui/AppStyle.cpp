#include "ui/AppStyle.h"

#include "ui/Theme.h"

#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QEvent>
#include <QFile>
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

// ── Static palette (updated on apply/reapply) ──
ThemePalette gCurrentPalette = ThemePalette::light();

const char *toCStr(const QString &s)
{
    // Lifetime tied to gCurrentPalette; safe for paint-time reads.
    return s.toLatin1().constData();
}

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
    shadow->setColor(QColor(0, 0, 0, static_cast<int>(gCurrentPalette.surface == QStringLiteral("#FFFFFF") ? 15 : 40)));
    widget->setGraphicsEffect(shadow);
}

void drawChevron(QPainter *painter, const QRect &rect, bool up, bool enabled)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(enabled ? QColor(gCurrentPalette.textMuted) : QColor(gCurrentPalette.placeholder));
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
    const QColor fill = enabled ? QColor(gCurrentPalette.surface) : QColor(gCurrentPalette.codeBg);
    QColor stroke;
    if (focused) {
        stroke = QColor(gCurrentPalette.primary);
    } else if (hover) {
        stroke = QColor(gCurrentPalette.inputHover);
    } else {
        stroke = QColor(gCurrentPalette.border);
    }

    QPen pen(stroke);
    pen.setWidthF(focused ? 1.5 : 1.0);
    painter->setPen(pen);
    painter->setBrush(fill);
    painter->drawRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), ThemeRadius::Medium, ThemeRadius::Medium);
    painter->restore();
}

void applyComboPopupStyle(QAbstractItemView *view)
{
    if (view == nullptr) {
        return;
    }

    const auto &p = gCurrentPalette;
    view->setObjectName(QStringLiteral("comboPopup"));
    view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setAutoFillBackground(true);
    view->viewport()->setAutoFillBackground(true);

    QPalette popupPalette = view->palette();
    popupPalette.setColor(QPalette::Base, QColor(p.surface));
    popupPalette.setColor(QPalette::AlternateBase, QColor(p.surface));
    popupPalette.setColor(QPalette::Window, QColor(p.surface));
    popupPalette.setColor(QPalette::Text, QColor(p.text));
    popupPalette.setColor(QPalette::WindowText, QColor(p.text));
    popupPalette.setColor(QPalette::ButtonText, QColor(p.text));
    popupPalette.setColor(QPalette::HighlightedText, QColor(p.text));
    popupPalette.setColor(QPalette::Highlight, QColor(p.primarySoft));
    view->setPalette(popupPalette);
    view->viewport()->setPalette(popupPalette);
    if (QWidget *popupWindow = view->window(); popupWindow != nullptr) {
        popupWindow->setPalette(popupPalette);
        popupWindow->setAutoFillBackground(true);
        popupWindow->setStyleSheet(QStringLiteral("background-color: %1; color: %2;").arg(p.surface, p.text));
    }

    view->setStyleSheet(QStringLiteral(R"(
        QAbstractItemView {
            background-color: %1;
            color: %2;
            selection-background-color: %3;
            selection-color: %2;
            border: 1px solid %4;
            border-radius: %5px;
            padding: 4px;
            outline: none;
        }
        QAbstractItemView::item {
            background-color: %1;
            color: %2;
            min-height: 28px;
            padding: 4px 10px;
            border-radius: %6px;
        }
        QAbstractItemView::item:hover {
            background-color: %7;
            color: %2;
        }
        QAbstractItemView::item:selected {
            background-color: %3;
            color: %2;
        }
    )").arg(p.surface, p.text, p.primarySoft, p.border,
            QString::number(ThemeRadius::Medium),
            QString::number(ThemeRadius::Small),
            p.primarySoftHover));
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
                label->setTextInteractionFlags(Qt::TextSelectableByMouse);
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

// ── QPalette for fundamental widget colors ──
void applyUiPalette(QApplication &app, const ThemePalette &p)
{
    QPalette palette = app.palette();
    palette.setColor(QPalette::Window, QColor(p.pageBg));
    palette.setColor(QPalette::WindowText, QColor(p.text));
    palette.setColor(QPalette::Base, QColor(p.inputBg));
    palette.setColor(QPalette::AlternateBase, QColor(p.surface));
    palette.setColor(QPalette::Text, QColor(p.text));
    palette.setColor(QPalette::Button, QColor(p.surface));
    palette.setColor(QPalette::ButtonText, QColor(p.text));
    palette.setColor(QPalette::Highlight, QColor(p.primary));
    palette.setColor(QPalette::HighlightedText, QColor(p.surface));
    palette.setColor(QPalette::ToolTipBase, QColor(p.surface));
    palette.setColor(QPalette::ToolTipText, QColor(p.text));
    palette.setColor(QPalette::PlaceholderText, QColor(p.placeholder));
    palette.setColor(QPalette::BrightText, QColor(p.danger));
    palette.setColor(QPalette::Link, QColor(p.primary));
    palette.setColor(QPalette::LinkVisited, QColor(p.primaryHover));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(p.placeholder));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(p.placeholder));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(p.placeholder));
    app.setPalette(palette);
}

// ── QSS placeholder replacement ──
QString buildStyleSheet(const QByteArray &templateBytes, const ThemePalette &p)
{
    QString styleTemplate = QString::fromUtf8(templateBytes);
    const QPair<QString, QString> replacements[] = {
        {QStringLiteral("@C1@"),  p.pageBg},
        {QStringLiteral("@C2@"),  p.text},
        {QStringLiteral("@C3@"),  p.textMuted},
        {QStringLiteral("@C4@"),  p.border},
        {QStringLiteral("@C5@"),  QString::number(ThemeRadius::Small)},
        {QStringLiteral("@C6@"),  p.primary},
        {QStringLiteral("@C7@"),  p.primarySoft},
        {QStringLiteral("@C8@"),  p.primarySoftHover},
        {QStringLiteral("@C9@"),  QString::number(ThemeRadius::Large)},
        {QStringLiteral("@C10@"), p.inputHover},
        {QStringLiteral("@C11@"), QString::number(ThemeRadius::Small / 2)},
        {QStringLiteral("@C12@"), p.success},
        {QStringLiteral("@C13@"), p.placeholder},
        {QStringLiteral("@C14@"), p.warning},
        {QStringLiteral("@C15@"), p.danger},
        {QStringLiteral("@C16@"), p.sidebarBg},
        {QStringLiteral("@C17@"), p.sidebarText},
        {QStringLiteral("@C18@"), p.primarySoftHover},
        {QStringLiteral("@C19@"), p.primarySoft},
        {QStringLiteral("@C20@"), p.sidebarTextSelected},
        {QStringLiteral("@C21@"), QString::number(ThemeRadius::Card)},
        {QStringLiteral("@C22@"), p.inputBg},
        {QStringLiteral("@C23@"), QStringLiteral("0px 2px 12px 0px ") + p.shadow},
        {QStringLiteral("@C24@"), p.botBubble},
        // New placeholders for hardcoded colors in style.qss
        {QStringLiteral("@C25@"), p.surface},
        {QStringLiteral("@C26@"), p.textSecondary},
        {QStringLiteral("@C27@"), p.codeBg},
        {QStringLiteral("@C28@"), p.statusBarBg},
        {QStringLiteral("@C29@"), p.dialogBg},
        {QStringLiteral("@C30@"), p.sectionHeaderBg},
        {QStringLiteral("@C31@"), p.statusPanelBg},
    };
    for (const auto &replacement : replacements) {
        styleTemplate.replace(replacement.first, replacement.second);
    }
    return styleTemplate;
}

// ── AppProperty setup (for runtime theme queries) ──
void applyAppProperties(QApplication &app, const ThemePalette &p)
{
    app.setProperty("theme.pageBg", p.pageBg);
    app.setProperty("theme.cardBg", p.surface);
    app.setProperty("theme.surface", p.surface);
    app.setProperty("theme.codeBg", p.codeBg);
    app.setProperty("theme.text", p.text);
    app.setProperty("theme.textSecondary", p.textSecondary);
    app.setProperty("theme.mutedText", p.textMuted);
    app.setProperty("theme.placeholderText", p.placeholder);
    app.setProperty("theme.accent", p.primary);
    app.setProperty("theme.primary", p.primary);
    app.setProperty("theme.primaryHover", p.primaryHover);
    app.setProperty("theme.accentSoft", p.primarySoft);
    app.setProperty("theme.accentHover", p.inputHover);
    app.setProperty("theme.border", p.border);
    app.setProperty("theme.softBorder", p.primarySoftHover);
    app.setProperty("theme.success", p.success);
    app.setProperty("theme.warning", p.warning);
    app.setProperty("theme.danger", p.danger);
    app.setProperty("theme.sidebarBg", p.sidebarBg);
    app.setProperty("theme.sidebarText", p.sidebarText);
    app.setProperty("theme.sidebarTextSelected", p.sidebarTextSelected);
    app.setProperty("theme.inputBg", p.inputBg);
    app.setProperty("theme.botBubble", p.botBubble);
}

} // namespace

namespace AppStyle {

static ThemeMode gMode = ThemeMode::System;

void apply(QApplication &app, ThemeMode mode)
{
    gMode = mode;
    gCurrentPalette = ThemePalette::fromMode(mode);

    // Set style proxy (only once per application lifecycle)
    if (auto *existing = dynamic_cast<AppProxyStyle *>(app.style()); existing == nullptr) {
        app.setStyle(new AppProxyStyle(app.style()));
        app.installEventFilter(new AppPolishFilter(&app));
    }

    applyUiPalette(app, gCurrentPalette);
    applyAppProperties(app, gCurrentPalette);

    // Load style.qss and apply
    QFile styleFile(QStringLiteral(":/style.qss"));
    if (!styleFile.exists()) {
        styleFile.setFileName(QStringLiteral(":/src/ui/style.qss"));
    }
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open style.qss from resources";
        return;
    }
    const QByteArray templateBytes = styleFile.readAll();
    styleFile.close();

    app.setStyleSheet(buildStyleSheet(templateBytes, gCurrentPalette));
}

void reapply(QApplication &app, ThemeMode mode)
{
    gMode = mode;
    gCurrentPalette = ThemePalette::fromMode(mode);

    applyUiPalette(app, gCurrentPalette);
    applyAppProperties(app, gCurrentPalette);

    QFile styleFile(QStringLiteral(":/style.qss"));
    if (!styleFile.exists()) {
        styleFile.setFileName(QStringLiteral(":/src/ui/style.qss"));
    }
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open style.qss from resources";
        return;
    }
    const QByteArray templateBytes = styleFile.readAll();
    styleFile.close();

    app.setStyleSheet(buildStyleSheet(templateBytes, gCurrentPalette));

    // Force repaint via palette change
    app.setPalette(app.palette());
}

const ThemePalette &currentPalette()
{
    return gCurrentPalette;
}

} // namespace AppStyle
