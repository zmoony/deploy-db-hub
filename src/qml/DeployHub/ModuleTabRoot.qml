import QtQuick
import DeployHub

DhModuleTabBar {
    anchors.fill: parent
    model: AppShell.moduleTitles
    currentIndex: AppShell.moduleIndex
    onTabChanged: function(index) { AppShell.moduleIndex = index }
}
