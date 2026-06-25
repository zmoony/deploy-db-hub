import QtQuick
import QtQuick.Effects

Item {
    id: root

    property var source
    property real blurAmount: 0.35
    property bool active: Visual.effectsEnabled

    visible: active && source !== undefined && source !== null
    anchors.fill: parent

    MultiEffect {
        anchors.fill: parent
        source: root.source
        autoPaddingEnabled: true
        blurEnabled: root.active
        blur: root.blurAmount
        blurMax: Visual.blurRadius
        saturation: 0.9
    }
}
