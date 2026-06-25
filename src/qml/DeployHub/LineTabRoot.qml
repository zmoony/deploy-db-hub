import QtQuick
import DeployHub

DhLineTabBar {
    anchors.fill: parent
    model: TabBar.labels
    currentIndex: TabBar.currentIndex
    onTabChanged: function(index) { TabBar.setCurrentIndex(index) }
}
