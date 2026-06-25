import QtQuick
import QtQuick.Controls

Button {
    id: control

    property bool primary: false

    implicitHeight: Theme.fieldHeight
    leftPadding: Theme.space16
    rightPadding: Theme.space16
    topPadding: 10
    bottomPadding: 10

    font.pixelSize: Theme.fontBody
    font.family: Theme.fontFamily
    font.weight: primary ? Font.DemiBold : Font.Normal

    contentItem: Text {
        text: control.text
        font: control.font
        renderType: Text.NativeRendering
        color: {
            if (!control.enabled)
                return Theme.textSecondary
            return control.primary ? "#FFFFFF" : Theme.textPrimary
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: Theme.buttonRadius
        color: {
            if (!control.enabled)
                return "#F1F5F9"
            if (control.down)
                return control.primary ? Theme.accentStrong : Theme.accentSoft
            if (control.hovered)
                return control.primary ? Theme.accentStrong : "#F8FAFC"
            return control.primary ? Theme.accent : "#FFFFFF"
        }
        border.color: {
            if (!control.enabled)
                return Theme.border
            if (control.primary)
                return "transparent"
            return control.hovered ? "#CBD5E1" : Theme.border
        }
        border.width: control.primary ? 0 : 1
    }
}
