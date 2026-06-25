import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import DeployHub

Rectangle {
    id: root
    color: "#FFFFFF"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.space12
        spacing: Theme.space12

        DhPageHeader {
            title: "UUID 生成"
            subtitle: "批量生成 UUID v4，可选大写、去横线。"
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Theme.space8
            Layout.fillWidth: true

            Text {
                text: "数量"
                font.pixelSize: Theme.fontBody
                font.family: Theme.fontFamily
                renderType: Text.NativeRendering
                color: Theme.textPrimary
            }

            DhSpinBox {
                id: countSpin
                from: 1
                to: 1000
                value: Tool.count
                editable: true
                onValueModified: Tool.count = value
            }

            DhCheckBox {
                text: "大写"
                checked: Tool.uppercase
                onToggled: Tool.uppercase = checked
            }

            DhCheckBox {
                text: "去横线"
                checked: Tool.withoutDashes
                onToggled: Tool.withoutDashes = checked
            }

            DhButton {
                text: "生成"
                onClicked: Tool.generate()
            }

            DhButton {
                text: "复制全部"
                onClicked: Tool.copyAll()
            }

            Item { Layout.fillWidth: true }
        }

        DhTextArea {
            readOnly: true
            placeholderText: "输出"
            text: Tool.output
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 220
        }
    }
}
