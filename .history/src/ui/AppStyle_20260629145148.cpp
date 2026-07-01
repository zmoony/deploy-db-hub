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

constexpr auto kPageBg = ThemeColors::PageBg;
constexpr auto kCardBg = ThemeColors::Surface;
constexpr auto kCodeBg = ThemeColors::PageBg;
constexpr auto kText = ThemeColors::Text;
constexpr auto kMutedText = ThemeColors::TextMuted;
constexpr auto kPlaceholderText = ThemeColors::Placeholder;
constexpr auto kAccent = ThemeColors::Primary;
constexpr auto kAccentSoft = ThemeColors::PrimarySoft;
constexpr auto kAccentHover = ThemeColors::InputHover;
constexpr auto kBorder = ThemeColors::Border;
constexpr auto kSoftBorder = ThemeColors::PrimarySoftHover;
constexpr auto kInputBg = ThemeColors::InputBg;
constexpr auto kBotBubble = ThemeColors::BotBubble;
constexpr auto kSuccess = ThemeColors::Success;
constexpr auto kWarning = ThemeColors::Warning;
constexpr auto kDanger = ThemeColors::Danger;
constexpr auto kSidebarBg = ThemeColors::SidebarBg;
constexpr auto kSidebarItemHover = ThemeColors::PrimarySoftHover;
constexpr auto kSidebarItemSelected = ThemeColors::PrimarySoft;
constexpr auto kSidebarText = ThemeColors::SidebarText;
constexpr auto kSidebarTextSelected = ThemeColors::SidebarTextSelected;

constexpr int kRadiusSm = ThemeRadius::Small;
constexpr int kRadiusMd = ThemeRadius::Medium;
constexpr int kRadiusLg = ThemeRadius::Large;
constexpr int kRadiusCard = ThemeRadius::Card;

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
        stroke = QColor(QString::fromLatin1(kAccentHover));
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
    QFile styleFile(QStringLiteral(":/style.qss"));
    if (!styleFile.exists()) {
        styleFile.setFileName(QStringLiteral(":/src/ui/style.qss"));
    }
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open style.qss from resources";
        return;
    }
    QString styleTemplate = QString::fromUtf8(styleFile.readAll());
    styleFile.close();
    const QPair<QString, QString> replacements[] = {
        {QStringLiteral("@C1@"), QString::fromLatin1(kPageBg)},
        {QStringLiteral("@C2@"), QString::fromLatin1(kText)},
        {QStringLiteral("@C3@"), QString::fromLatin1(kMutedText)},
        {QStringLiteral("@C4@"), QString::fromLatin1(kBorder)},
        {QStringLiteral("@C5@"), QString::number(kRadiusSm)},
        {QStringLiteral("@C6@"), QString::fromLatin1(kAccent)},
        {QStringLiteral("@C7@"), QString::fromLatin1(kAccentSoft)},
        {QStringLiteral("@C8@"), QString::fromLatin1(kSoftBorder)},
        {QStringLiteral("@C9@"), QString::number(kRadiusLg)},
        {QStringLiteral("@C10@"), QString::fromLatin1(kAccentHover)},
        {QStringLiteral("@C11@"), QString::number(kRadiusSm / 2)},
        {QStringLiteral("@C12@"), QString::fromLatin1(kSuccess)},
        {QStringLiteral("@C13@"), QString::fromLatin1(kPlaceholderText)},
        {QStringLiteral("@C14@"), QString::fromLatin1(kWarning)},
        {QStringLiteral("@C15@"), QString::fromLatin1(kDanger)},
        {QStringLiteral("@C16@"), QString::fromLatin1(kSidebarBg)},
        {QStringLiteral("@C17@"), QString::fromLatin1(kSidebarText)},
        {QStringLiteral("@C18@"), QString::fromLatin1(kSidebarItemHover)},
        {QStringLiteral("@C19@"), QString::fromLatin1(kSidebarItemSelected)},
        {QStringLiteral("@C20@"), QString::fromLatin1(kSidebarTextSelected)},
        {QStringLiteral("@C21@"), QString::number(kRadiusCard)},
        {QStringLiteral("@C22@"), QString::fromLatin1(kInputBg)},
        {QStringLiteral("@C23@"), QStringLiteral("0px 2px 12px 0px rgba(30, 64, 175, 0.094118)")},
        {QStringLiteral("@C24@"), QString::fromLatin1(kBotBubble)}
    };
    for (const auto &replacement : replacements) {
        styleTemplate.replace(replacement.first, replacement.second);
    }
    app.setStyleSheet(styleTemplate);
}

}
