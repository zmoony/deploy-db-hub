pragma Singleton
import QtQuick

QtObject {
    // Match AppStyle.cpp tokens
    readonly property color pageBg: "#F8FAFC"
    readonly property color cardBg: "#FFFFFF"
    readonly property color textPrimary: "#0F172A"
    readonly property color textRegular: "#64748B"
    readonly property color textSecondary: "#94A3B8"
    readonly property color accent: "#6366F1"
    readonly property color accentStrong: "#4F46E5"
    readonly property color accentSoft: "#EEF2FF"
    readonly property color accentHoverBg: "#F1F5F9"
    readonly property color border: "#E2E8F0"
    readonly property color lineTabActive: "#409EFF"
    readonly property color lineTabText: "#303133"

    readonly property color sidebarBg: "#1E293B"
    readonly property color sidebarBorder: "#1E293B"
    readonly property color sidebarText: "#E2E8F0"
    readonly property color sidebarTextMuted: "#CBD5E1"
    readonly property color sidebarHover: "#334155"
    readonly property color sidebarDivider: "#334155"
    readonly property color sidebarBrandText: "#F8FAFC"

    readonly property int space4: 4
    readonly property int space6: 6
    readonly property int space8: 8
    readonly property int space12: 12
    readonly property int space16: 16
    readonly property int space24: 24

    readonly property int moduleTabSpacing: 4
    readonly property int lineTabSpacing: 12

    readonly property int pagePadding: 16
    readonly property int sidebarWidth: 180
    readonly property int sidebarRadius: 24
    readonly property int contentRadius: 16
    readonly property int navItemRadius: 10
    readonly property int moduleTabRadius: 8
    readonly property int brandMarkSize: 32

    readonly property int fontCaption: 12
    readonly property int fontBody: 14
    readonly property int fontSmall: 13
    readonly property int fontBrand: 15
    readonly property int fontTitle: 18

    readonly property int fieldHeight: 36
    readonly property int buttonRadius: 10

    // Segoe UI for Latin chrome; YaHei for CJK body text on Windows.
    readonly property string fontFamily: Qt.platform.os === "windows"
                                       ? "Microsoft YaHei UI"
                                       : "Segoe UI"
    readonly property string fontFamilyLatin: "Segoe UI"
}
