#pragma once

#include <QApplication>
#include <QString>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#include <QStyleHints>
#endif

enum class ThemeMode {
    Light,
    Dark,
    System
};

namespace ThemeColors {

// ── Light palette ──
namespace Light {
constexpr auto PageBg = "#F0F4F8";
constexpr auto Surface = "#FFFFFF";
constexpr auto Text = "#1A1A2E";
constexpr auto TextSecondary = "#374151";
constexpr auto TextMuted = "#6B7280";
constexpr auto Placeholder = "#9CA3AF";
constexpr auto Primary = "#2563EB";
constexpr auto PrimaryHover = "#1D4ED8";
constexpr auto PrimarySoft = "#EFF6FF";
constexpr auto PrimarySoftHover = "#DBEAFE";
constexpr auto Border = "#E5E9F0";
constexpr auto InputBorder = "#E5E9F0";
constexpr auto InputHover = "#93C5FD";
constexpr auto InputBg = "#F9FAFB";
constexpr auto BotBubble = "#F3F4F6";
constexpr auto Success = "#16A34A";
constexpr auto SuccessBg = "#D4F1DC";
constexpr auto Warning = "#F59E0B";
constexpr auto WarningBg = "#FFF3E8";
constexpr auto Danger = "#EF4444";
constexpr auto DangerHover = "#DC2626";
constexpr auto DangerBg = "#FFECE8";
constexpr auto SidebarBg = "#FFFFFF";
constexpr auto SidebarText = "#6B7280";
constexpr auto SidebarTextSelected = "#2563EB";
constexpr auto Shadow = "rgba(30, 64, 175, 0.09)";
constexpr auto TableStripe = "#F8FAFC";
constexpr auto CodeBg = "#F0F4F8";
constexpr auto StatusBarBg = "#FFFFFF";
constexpr auto DialogBg = "#FFFFFF";
constexpr auto SectionHeaderBg = "#F1F3FF";
constexpr auto StatusPanelBg = "#E1E8FD";
}

// ── Dark palette ──
namespace Dark {
constexpr auto PageBg = "#0F0F1A";
constexpr auto Surface = "#1A1A2E";
constexpr auto Text = "#E2E8F0";
constexpr auto TextSecondary = "#CBD5E1";
constexpr auto TextMuted = "#94A3B8";
constexpr auto Placeholder = "#64748B";
constexpr auto Primary = "#4E8EFA";
constexpr auto PrimaryHover = "#3B82F6";
constexpr auto PrimarySoft = "rgba(78, 142, 250, 0.12)";
constexpr auto PrimarySoftHover = "rgba(78, 142, 250, 0.20)";
constexpr auto Border = "#334155";
constexpr auto InputBorder = "#334155";
constexpr auto InputHover = "#4E8EFA";
constexpr auto InputBg = "#1E293B";
constexpr auto BotBubble = "#1E293B";
constexpr auto Success = "#22C55E";
constexpr auto SuccessBg = "rgba(34, 197, 94, 0.15)";
constexpr auto Warning = "#FBBF24";
constexpr auto WarningBg = "rgba(251, 191, 36, 0.15)";
constexpr auto Danger = "#F87171";
constexpr auto DangerHover = "#EF4444";
constexpr auto DangerBg = "rgba(248, 113, 113, 0.15)";
constexpr auto SidebarBg = "#141425";
constexpr auto SidebarText = "#94A3B8";
constexpr auto SidebarTextSelected = "#4E8EFA";
constexpr auto Shadow = "rgba(0, 0, 0, 0.30)";
constexpr auto TableStripe = "rgba(255, 255, 255, 0.03)";
constexpr auto CodeBg = "#1E293B";
constexpr auto StatusBarBg = "#141425";
constexpr auto DialogBg = "#1A1A2E";
constexpr auto SectionHeaderBg = "rgba(78, 142, 250, 0.08)";
constexpr auto StatusPanelBg = "rgba(78, 142, 250, 0.06)";
}

} // namespace ThemeColors

// ── Runtime theme palette ──
struct ThemePalette {
    QString pageBg, surface, text, textSecondary, textMuted, placeholder;
    QString primary, primaryHover, primarySoft, primarySoftHover;
    QString border, inputBorder, inputHover, inputBg, botBubble;
    QString success, successBg, warning, warningBg, danger, dangerHover, dangerBg;
    QString sidebarBg, sidebarText, sidebarTextSelected;
    QString shadow, tableStripe, codeBg, statusBarBg, dialogBg;
    QString sectionHeaderBg, statusPanelBg;

    static ThemePalette fromMode(ThemeMode mode);
    static ThemePalette light();
    static ThemePalette dark();

    static ThemeMode resolveMode(ThemeMode stored);
};

// ── Legacy compatibility (kept for existing call sites) ──
// New code should use ThemePalette::fromMode().
namespace ThemeColors {
constexpr auto PageBg = Light::PageBg;
constexpr auto Surface = Light::Surface;
constexpr auto Text = Light::Text;
constexpr auto TextSecondary = Light::TextSecondary;
constexpr auto TextMuted = Light::TextMuted;
constexpr auto Placeholder = Light::Placeholder;
constexpr auto Primary = Light::Primary;
constexpr auto PrimaryHover = Light::PrimaryHover;
constexpr auto PrimarySoft = Light::PrimarySoft;
constexpr auto PrimarySoftHover = Light::PrimarySoftHover;
constexpr auto Border = Light::Border;
constexpr auto InputBorder = Light::InputBorder;
constexpr auto InputHover = Light::InputHover;
constexpr auto InputBg = Light::InputBg;
constexpr auto BotBubble = Light::BotBubble;
constexpr auto Success = Light::Success;
constexpr auto Warning = Light::Warning;
constexpr auto Danger = Light::Danger;
constexpr auto DangerHover = Light::DangerHover;
constexpr auto SidebarBg = Light::SidebarBg;
constexpr auto SidebarText = Light::SidebarText;
constexpr auto SidebarTextSelected = Light::SidebarTextSelected;
} // namespace ThemeColors

namespace ThemeRadius {
constexpr int Small = 4;
constexpr int Medium = 8;
constexpr int Large = 10;
constexpr int Card = 12;
constexpr int Pill = 999;
}

namespace ThemeSpacing {
constexpr int Xs = 4;
constexpr int Sm = 8;
constexpr int Md = 16;
constexpr int Lg = 24;
}

namespace ThemeFonts {
constexpr int Title = 15;
constexpr int SectionTitle = 14;
constexpr int Body = 13;
constexpr int Caption = 12;
constexpr int Small = 11;
}
