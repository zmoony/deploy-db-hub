import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitHeight: 36

    property var model: []
    property int currentIndex: 0
    signal tabChanged(int index)

    Rectangle {
        anchors.fill: parent
        radius: Theme.moduleTabRadius + 4
        gradient: Gradient {
            GradientStop { position: 0.0; color: Visual.moduleGradientStart }
            GradientStop { position: 1.0; color: Visual.moduleGradientEnd }
        }
        border.color: Theme.border
        border.width: 1
        opacity: 0.95
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.space4
        spacing: Theme.moduleTabSpacing

        Repeater {
            model: root.model

            delegate: Button {
                id: tabButton
                text: modelData
                checkable: true
                checked: index === root.currentIndex
                flat: true
                implicitHeight: 32
                leftPadding: Theme.space8
                rightPadding: Theme.space8
                topPadding: Theme.space6
                bottomPadding: Theme.space6

                contentItem: Text {
                    text: tabButton.text
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    font.weight: tabButton.checked ? Font.DemiBold : Font.Medium
                    renderType: Text.NativeRendering
                    color: tabButton.checked ? "#FFFFFF"
                           : (tabButton.hovered ? Theme.accent : Theme.textRegular)
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: Theme.moduleTabRadius
                    color: tabButton.checked
                           ? (tabButton.hovered ? Theme.accentStrong : Theme.accent)
                           : (tabButton.hovered ? Theme.accentHoverBg : "transparent")
                    Behavior on color {
                        enabled: Visual.effectsEnabled
                        ColorAnimation { duration: 100 }
                    }
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
