import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: 220
    implicitHeight: column.implicitHeight

    property alias model: navList.model
    property int currentIndex: -1
    signal itemClicked(int index)
    signal settingsClicked()

    Rectangle {
        anchors.fill: parent
        color: "#FFFFFF"
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Layout.preferredHeight: 48

            Rectangle {
                width: 32
                height: 32
                radius: 8
                color: "#2563EB"

                Text {
                    anchors.centerIn: parent
                    text: "D"
                    color: "#FFFFFF"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    renderType: Text.NativeRendering
                }
            }

            Text {
                text: "Deploy Hub"
                font.pixelSize: 16
                font.weight: Font.Medium
                color: "#1A1A2E"
                renderType: Text.NativeRendering
                Layout.fillWidth: true
            }
        }

        ListView {
            id: navList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 2
            clip: true

            delegate: ItemDelegate {
                id: navItem
                width: navList.width
                height: 40
                hoverEnabled: true
                padding: 0

                readonly property bool isSelected: index === root.currentIndex && !AppShell.settingsVisible

                background: Rectangle {
                    radius: 8
                    color: navItem.isSelected ? "#EFF6FF"
                           : (navItem.hovered ? "#F3F4F6" : "transparent")
                }

                contentItem: RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        radius: 4
                        color: navItem.isSelected ? "#2563EB" : "#D1D5DB"
                    }

                    Text {
                        text: modelData
                        font.pixelSize: 14
                        font.weight: navItem.isSelected ? Font.Medium : Font.Normal
                        color: navItem.isSelected ? "#2563EB" : "#6B7280"
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                        Layout.fillWidth: true
                    }
                }

                onClicked: root.itemClicked(index)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#E5E9F0"
            opacity: 0.6
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                id: settingsButton
                Layout.fillWidth: true
                text: "设置"
                flat: true
                checkable: true
                checked: AppShell.settingsVisible
                padding: 0
                implicitHeight: 40

                contentItem: RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Rectangle {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        radius: 4
                        color: settingsButton.checked ? "#2563EB" : "#D1D5DB"
                    }

                    Text {
                        text: settingsButton.text
                        font.pixelSize: 14
                        font.weight: settingsButton.checked ? Font.Medium : Font.Normal
                        color: settingsButton.checked ? "#2563EB" : "#6B7280"
                        renderType: Text.NativeRendering
                        verticalAlignment: Text.AlignVCenter
                        Layout.fillWidth: true
                    }
                }

                background: Rectangle {
                    radius: 8
                    color: settingsButton.checked ? "#EFF6FF"
                         : (settingsButton.hovered ? "#F3F4F6" : "transparent")
                }

                onClicked: {
                    if (AppShell.settingsVisible) {
                        AppShell.showMainContent()
                    } else {
                        root.settingsClicked()
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                leftPadding: 12
                text: "Admin"
                font.pixelSize: 12
                color: "#9CA3AF"
                renderType: Text.NativeRendering
            }
        }
    }
}
