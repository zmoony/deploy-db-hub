import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DeployHub

ApplicationWindow {
    id: root
    color: Theme.pageBg
    visible: true

    MainShell {
        anchors.fill: parent
    }

    footer: Label {
        leftPadding: Theme.space16
        rightPadding: Theme.space16
        topPadding: Theme.space4
        bottomPadding: Theme.space4
        text: qsTr("配置目录：%1").arg(AppShell.configDir)
        font.pixelSize: Theme.fontCaption
        font.family: Theme.fontFamily
        renderType: Text.NativeRendering
        color: Theme.textSecondary
        elide: Text.ElideMiddle
    }
}
