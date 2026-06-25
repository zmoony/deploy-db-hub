pragma Singleton
import QtQuick

QtObject {
    id: root

    // Scroll / drag: disable expensive effects temporarily.
    property bool effectsEnabled: true

    readonly property int shadowOffsetY: 4
    readonly property real shadowOpacity: 0.12
    readonly property int cardRadius: Theme.contentRadius
    readonly property int blurRadius: 16

    readonly property color gradientTop: "#243044"
    readonly property color gradientBottom: Theme.sidebarBg
    readonly property color moduleGradientStart: "#FFFFFF"
    readonly property color moduleGradientEnd: "#F1F5F9"
}
