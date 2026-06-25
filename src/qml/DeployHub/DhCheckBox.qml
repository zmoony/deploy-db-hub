import QtQuick
import QtQuick.Controls

CheckBox {
    id: control

    spacing: Theme.space8
    implicitHeight: Theme.fieldHeight

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: 4
        color: control.checked ? Theme.accent : "#FFFFFF"
        border.color: control.checked ? Theme.accent : Theme.border
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "\u2713"
            font.pixelSize: 12
            font.family: Theme.fontFamily
            renderType: Text.NativeRendering
            color: "#FFFFFF"
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font.pixelSize: Theme.fontBody
        font.family: Theme.fontFamily
        renderType: Text.NativeRendering
        color: Theme.textPrimary
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
