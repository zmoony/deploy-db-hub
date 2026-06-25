import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DeployHub

Item {
    id: root
    anchors.fill: parent

    Rectangle {
        anchors.fill: parent
        color: Theme.pageBg
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.pagePadding
        spacing: Theme.space8

        DhSidebar {
            id: sidebar
            Layout.preferredWidth: Theme.sidebarWidth
            Layout.minimumWidth: Theme.sidebarWidth
            Layout.maximumWidth: Theme.sidebarWidth
            Layout.fillHeight: true
            model: AppShell.navLabels
            currentIndex: AppShell.settingsVisible ? -1 : AppShell.navIndex
            onItemClicked: function(index) { AppShell.navIndex = index }
            onSettingsClicked: AppShell.showSettings()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 320
            spacing: Theme.space8

            DhModuleTabBar {
                Layout.fillWidth: true
                model: AppShell.moduleTitles
                currentIndex: AppShell.moduleIndex
                onTabChanged: function(index) { AppShell.moduleIndex = index }
            }

            WidgetHost {
                Layout.fillWidth: true
                Layout.fillHeight: true
                widget: AppShell.currentWidget
            }
        }
    }
}
