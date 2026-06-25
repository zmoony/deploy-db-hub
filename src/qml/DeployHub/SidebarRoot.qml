import QtQuick
import DeployHub

DhSidebar {
    anchors.fill: parent
    model: AppShell.navLabels
    currentIndex: AppShell.settingsVisible ? -1 : AppShell.navIndex
    onItemClicked: function(index) { AppShell.navIndex = index }
    onSettingsClicked: AppShell.showSettings()
}
