import QtQuick
import QtQuick.Controls

Item {
    id: root
    implicitWidth: tabRow.implicitWidth
    implicitHeight: 41

    property var model: []
    property int currentIndex: 0
    signal tabChanged(int index)

    readonly property int lineTabHeight: 40

    Row {
        id: tabRow
        anchors.left: parent.left
        anchors.top: parent.top
        spacing: Theme.space24

        Repeater {
            model: root.model

            delegate: TabButton {
                id: tabButton
                text: modelData
                height: root.lineTabHeight
                leftPadding: Theme.space4
                rightPadding: Theme.space4
                topPadding: Theme.space12
                bottomPadding: Theme.space12
                checked: index === root.currentIndex

                contentItem: Text {
                    text: tabButton.text
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    font.weight: tabButton.checked ? Font.Medium : Font.Normal
                    renderType: Text.NativeRendering
                    color: tabButton.checked ? Theme.lineTabActive : Theme.lineTabText
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Item {
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 2
                        visible: tabButton.checked
                        color: Theme.lineTabActive
                    }
                }

                onClicked: {
                    if (root.currentIndex !== index) {
                        root.tabChanged(index)
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#E4E7ED"
    }
}
