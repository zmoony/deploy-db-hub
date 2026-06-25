import QtQuick

Item {
    id: root

    default property alias content: contentSlot.data
    property alias color: surface.color
    property alias radius: surface.radius
    property alias borderColor: surface.border.color
    property bool shadowEnabled: Visual.effectsEnabled
    property int shadowOffset: Visual.shadowOffsetY

    implicitWidth: surface.implicitWidth
    implicitHeight: surface.implicitHeight

    Rectangle {
        id: shadow
        anchors.fill: surface
        anchors.topMargin: root.shadowEnabled ? root.shadowOffset : 0
        visible: root.shadowEnabled
        radius: surface.radius
        color: "#000000"
        opacity: Visual.shadowOpacity
        z: -1
    }

    Rectangle {
        id: surface
        radius: Visual.cardRadius
        color: Theme.cardBg
        border.color: Theme.border
        border.width: 1

        Item {
            id: contentSlot
            anchors.fill: parent
        }
    }
}
