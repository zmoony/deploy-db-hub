// FluentUI theme + standard window (avoids FluWindow -> FluAcrylic -> Qt5Compat.GraphicalEffects).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import FluentUI
import DeployHub

ApplicationWindow {
    id: root
    color: Theme.pageBg
    visible: true

    Component.onCompleted: {
        FluApp.init(root)
        FluTheme.primaryColor = Theme.accent
        FluTheme.darkMode = FluThemeType.Light
        FluTheme.blurBehindWindowEnabled = false
    }

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
