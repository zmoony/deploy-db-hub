import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitHeight: 56

    property var model: []
    property int currentIndex: 0
    signal tabChanged(int index)

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#E5E9F0"
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 8

        Repeater {
            model: root.model

            delegate: Button {
                id: tabButton
                text: modelData
                checkable: true
                checked: index === root.currentIndex
                flat: true
                implicitHeight: 36
                leftPadding: 16
                rightPadding: 16
                topPadding: 0
                bottomPadding: 0

                contentItem: Text {
                    text: tabButton.text
                    font.pixelSize: 14
                    font.weight: tabButton.checked ? Font.Medium : Font.Normal
                    color: tabButton.checked ? "#FFFFFF" : "#6B7280"
                    renderType: Text.NativeRendering
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 8
                    color: tabButton.checked ? "#2563EB"
                           : (tabButton.hovered ? "#F3F4F6" : "transparent")
                }

                onClicked: {
                    if (root.currentIndex !== index) {
                        root.tabChanged(index)
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
