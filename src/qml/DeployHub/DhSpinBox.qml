import QtQuick
import QtQuick.Controls

SpinBox {
    id: control

    implicitHeight: Theme.fieldHeight
    implicitWidth: 120

    contentItem: TextInput {
        z: 2
        text: control.textFromValue(control.value, control.locale)
        font.pixelSize: Theme.fontBody
        font.family: Theme.fontFamily
        renderType: Text.NativeRendering
        color: Theme.textPrimary
        selectionColor: "#E0E7FF"
        selectedTextColor: Theme.textPrimary
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    background: Rectangle {
        implicitHeight: Theme.fieldHeight
        color: "#FFFFFF"
        border.color: control.activeFocus ? Theme.accent : Theme.border
        border.width: 1
        radius: Theme.buttonRadius
    }

    up.indicator: Rectangle {
        x: control.mirrored ? 0 : parent.width - width
        height: parent.height
        width: 28
        color: up.pressed ? Theme.accentSoft : "transparent"
        radius: Theme.buttonRadius

        Text {
            anchors.centerIn: parent
            text: "+"
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            renderType: Text.NativeRendering
            color: Theme.textPrimary
        }
    }

    down.indicator: Rectangle {
        x: control.mirrored ? parent.width - width : 0
        height: parent.height
        width: 28
        color: down.pressed ? Theme.accentSoft : "transparent"
        radius: Theme.buttonRadius

        Text {
            anchors.centerIn: parent
            text: "\u2212"
            font.pixelSize: Theme.fontBody
            font.family: Theme.fontFamily
            renderType: Text.NativeRendering
            color: Theme.textPrimary
        }
    }
}
