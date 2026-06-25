import QtQuick
import QtQuick.Controls

TextArea {
    id: editor

    padding: 10
    font.pixelSize: Theme.fontBody
    font.family: Theme.fontFamily
    renderType: Text.NativeRendering
    color: Theme.textPrimary
    selectionColor: "#E0E7FF"
    selectedTextColor: Theme.textPrimary
    wrapMode: TextArea.NoWrap
    placeholderTextColor: Theme.textSecondary

    background: Rectangle {
        radius: Theme.buttonRadius
        color: "#FFFFFF"
        border.color: editor.activeFocus ? Theme.accent : Theme.border
        border.width: 1
    }
}
