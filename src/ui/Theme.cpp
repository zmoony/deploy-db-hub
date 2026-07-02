#include "ui/Theme.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QCoreApplication>
#include <QGuiApplication>
#include <QStyleHints>
#endif

ThemePalette ThemePalette::light()
{
    return {
        QString::fromLatin1(ThemeColors::Light::PageBg),
        QString::fromLatin1(ThemeColors::Light::Surface),
        QString::fromLatin1(ThemeColors::Light::Text),
        QString::fromLatin1(ThemeColors::Light::TextSecondary),
        QString::fromLatin1(ThemeColors::Light::TextMuted),
        QString::fromLatin1(ThemeColors::Light::Placeholder),
        QString::fromLatin1(ThemeColors::Light::Primary),
        QString::fromLatin1(ThemeColors::Light::PrimaryHover),
        QString::fromLatin1(ThemeColors::Light::PrimarySoft),
        QString::fromLatin1(ThemeColors::Light::PrimarySoftHover),
        QString::fromLatin1(ThemeColors::Light::Border),
        QString::fromLatin1(ThemeColors::Light::InputBorder),
        QString::fromLatin1(ThemeColors::Light::InputHover),
        QString::fromLatin1(ThemeColors::Light::InputBg),
        QString::fromLatin1(ThemeColors::Light::BotBubble),
        QString::fromLatin1(ThemeColors::Light::Success),
        QString::fromLatin1(ThemeColors::Light::SuccessBg),
        QString::fromLatin1(ThemeColors::Light::Warning),
        QString::fromLatin1(ThemeColors::Light::WarningBg),
        QString::fromLatin1(ThemeColors::Light::Danger),
        QString::fromLatin1(ThemeColors::Light::DangerHover),
        QString::fromLatin1(ThemeColors::Light::DangerBg),
        QString::fromLatin1(ThemeColors::Light::SidebarBg),
        QString::fromLatin1(ThemeColors::Light::SidebarText),
        QString::fromLatin1(ThemeColors::Light::SidebarTextSelected),
        QString::fromLatin1(ThemeColors::Light::Shadow),
        QString::fromLatin1(ThemeColors::Light::TableStripe),
        QString::fromLatin1(ThemeColors::Light::CodeBg),
        QString::fromLatin1(ThemeColors::Light::StatusBarBg),
        QString::fromLatin1(ThemeColors::Light::DialogBg),
        QString::fromLatin1(ThemeColors::Light::SectionHeaderBg),
        QString::fromLatin1(ThemeColors::Light::StatusPanelBg)
    };
}

ThemePalette ThemePalette::dark()
{
    return {
        QString::fromLatin1(ThemeColors::Dark::PageBg),
        QString::fromLatin1(ThemeColors::Dark::Surface),
        QString::fromLatin1(ThemeColors::Dark::Text),
        QString::fromLatin1(ThemeColors::Dark::TextSecondary),
        QString::fromLatin1(ThemeColors::Dark::TextMuted),
        QString::fromLatin1(ThemeColors::Dark::Placeholder),
        QString::fromLatin1(ThemeColors::Dark::Primary),
        QString::fromLatin1(ThemeColors::Dark::PrimaryHover),
        QString::fromLatin1(ThemeColors::Dark::PrimarySoft),
        QString::fromLatin1(ThemeColors::Dark::PrimarySoftHover),
        QString::fromLatin1(ThemeColors::Dark::Border),
        QString::fromLatin1(ThemeColors::Dark::InputBorder),
        QString::fromLatin1(ThemeColors::Dark::InputHover),
        QString::fromLatin1(ThemeColors::Dark::InputBg),
        QString::fromLatin1(ThemeColors::Dark::BotBubble),
        QString::fromLatin1(ThemeColors::Dark::Success),
        QString::fromLatin1(ThemeColors::Dark::SuccessBg),
        QString::fromLatin1(ThemeColors::Dark::Warning),
        QString::fromLatin1(ThemeColors::Dark::WarningBg),
        QString::fromLatin1(ThemeColors::Dark::Danger),
        QString::fromLatin1(ThemeColors::Dark::DangerHover),
        QString::fromLatin1(ThemeColors::Dark::DangerBg),
        QString::fromLatin1(ThemeColors::Dark::SidebarBg),
        QString::fromLatin1(ThemeColors::Dark::SidebarText),
        QString::fromLatin1(ThemeColors::Dark::SidebarTextSelected),
        QString::fromLatin1(ThemeColors::Dark::Shadow),
        QString::fromLatin1(ThemeColors::Dark::TableStripe),
        QString::fromLatin1(ThemeColors::Dark::CodeBg),
        QString::fromLatin1(ThemeColors::Dark::StatusBarBg),
        QString::fromLatin1(ThemeColors::Dark::DialogBg),
        QString::fromLatin1(ThemeColors::Dark::SectionHeaderBg),
        QString::fromLatin1(ThemeColors::Dark::StatusPanelBg)
    };
}

ThemeMode ThemePalette::resolveMode(ThemeMode stored)
{
    if (stored == ThemeMode::Light) {
        return ThemeMode::Light;
    }
    if (stored == ThemeMode::Dark) {
        return ThemeMode::Dark;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance()); app != nullptr) {
        if (app->styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            return ThemeMode::Dark;
        }
    }
#endif
    return ThemeMode::Light;
}

ThemePalette ThemePalette::fromMode(ThemeMode mode)
{
    const ThemeMode effective = resolveMode(mode);
    if (effective == ThemeMode::Dark) {
        return dark();
    }
    return light();
}
