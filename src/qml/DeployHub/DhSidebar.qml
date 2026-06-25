import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    implicitWidth: Theme.sidebarWidth
    implicitHeight: column.implicitHeight

    property alias model: navList.model
    property int currentIndex: -1
    signal itemClicked(int index)
    signal settingsClicked()

    Rectangle {
        id: shell
        anchors.fill: parent
        radius: Theme.sidebarRadius
        border.color: Theme.sidebarBorder
        border.width: 1
        clip: false

        gradient: Gradient {
            GradientStop { position: 0.0; color: Visual.gradientTop }
            GradientStop { position: 1.0; color: Visual.gradientBottom }
        }

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: Visual.effectsEnabled ? Visual.shadowOffsetY : 0
            radius: parent.radius
            visible: Visual.effectsEnabled
            color: "#000000"
            opacity: Visual.shadowOpacity
            z: -1
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: Theme.space12
        anchors.topMargin: Theme.space16
        anchors.bottomMargin: Theme.space16
        spacing: Theme.space12

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space8
            Layout.preferredHeight: 48

            Rectangle {
                width: Theme.brandMarkSize
                height: Theme.brandMarkSize
                radius: 10
                gradient: Gradient {
                    GradientStop { position: 0.0; color: Theme.accent }
                    GradientStop { position: 1.0; color: Theme.accentStrong }
                }

                Text {
                    anchors.centerIn: parent
                    text: "D"
                    color: "#FFFFFF"
                    font.pixelSize: Theme.fontBrand
                    font.weight: Font.Bold
                    font.family: Theme.fontFamilyLatin
                    renderType: Text.NativeRendering
                }
            }

            Text {
                text: "Deploy Hub"
                font.pixelSize: Theme.fontBrand
                font.weight: Font.DemiBold
                font.family: Theme.fontFamilyLatin
                renderType: Text.NativeRendering
                color: Theme.sidebarBrandText
                Layout.fillWidth: true
            }
        }

        ListView {
            id: navList
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.space4
            clip: true
            cacheBuffer: 240
            reuseItems: true
            enabled: !AppShell.settingsVisible

            onMovementStarted: Visual.effectsEnabled = false
            onMovementEnded: Visual.effectsEnabled = true
            onFlickStarted: Visual.effectsEnabled = false
            onFlickEnded: Visual.effectsEnabled = true

            delegate: ItemDelegate {
                id: navItem
                width: navList.width
                height: 44
                hoverEnabled: true
                padding: 0

                background: Rectangle {
                    radius: Theme.navItemRadius
                    color: AppShell.settingsVisible ? "transparent"
                           : (index === root.currentIndex ? Theme.accentStrong
                              : (navItem.hovered ? Theme.sidebarHover : "transparent"))
                    Behavior on color {
                        enabled: Visual.effectsEnabled
                        ColorAnimation { duration: 120 }
                    }
                }

                contentItem: Text {
                    text: modelData
                    leftPadding: Theme.space16
                    rightPadding: Theme.space16
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    font.weight: (!AppShell.settingsVisible && index === root.currentIndex)
                                  ? Font.DemiBold : Font.Normal
                    renderType: Text.NativeRendering
                    color: AppShell.settingsVisible ? Theme.sidebarText
                           : (index === root.currentIndex ? "#FFFFFF" : Theme.sidebarText)
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.itemClicked(index)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.sidebarDivider
            opacity: 0.6
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space8

            Button {
                id: settingsButton
                Layout.fillWidth: true
                text: "\u2699  \u8BBE\u7F6E"
                flat: true
                checkable: true
                checked: AppShell.settingsVisible
                padding: 0
                implicitHeight: 40

                contentItem: Text {
                    text: settingsButton.text
                    leftPadding: Theme.space12
                    rightPadding: Theme.space12
                    font.pixelSize: Theme.fontBody
                    font.family: Theme.fontFamily
                    font.weight: Font.Medium
                    renderType: Text.NativeRendering
                    color: settingsButton.checked ? "#FFFFFF" : Theme.sidebarText
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: Theme.navItemRadius
                    color: settingsButton.checked ? Theme.accentStrong
                         : (settingsButton.hovered ? Theme.sidebarHover : "transparent")
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
                leftPadding: Theme.space12
                text: "Admin"
                font.pixelSize: Theme.fontCaption
                font.family: Theme.fontFamilyLatin
                renderType: Text.NativeRendering
                color: Theme.sidebarTextMuted
            }
        }
    }
}
