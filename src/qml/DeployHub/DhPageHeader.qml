import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string title: ""
    property string subtitle: ""

    spacing: Theme.space8

    Text {
        text: root.title
        font.pixelSize: Theme.fontTitle
        font.family: Theme.fontFamily
        font.weight: Font.DemiBold
        renderType: Text.NativeRendering
        color: Theme.textPrimary
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    Text {
        visible: root.subtitle.length > 0
        text: root.subtitle
        font.pixelSize: Theme.fontSmall
        font.family: Theme.fontFamily
        renderType: Text.NativeRendering
        color: Theme.textRegular
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
