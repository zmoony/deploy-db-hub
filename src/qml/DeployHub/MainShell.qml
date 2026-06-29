import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DeployHub

Rectangle {
    id: root
    color: "#F0F4F8"

    RowLayout {
        anchors.fill: parent
        spacing: 0

        DhSidebar {
            id: sidebar
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            model: AppShell.navLabels
            currentIndex: AppShell.settingsVisible ? -1 : AppShell.navIndex
            onItemClicked: function(index) { AppShell.navIndex = index }
            onSettingsClicked: AppShell.showSettings()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            DhModuleTabBar {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                model: AppShell.moduleTitles
                currentIndex: AppShell.moduleIndex
                onTabChanged: function(index) { AppShell.moduleIndex = index }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                WidgetHost {
                    widget: AppShell.currentWidget
                }
            }
        }
    }
}
