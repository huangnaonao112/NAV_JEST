// StartConfig.qml - 启动配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    property string dataKey: "startType"

    signal updateDetailData(string key, var value)

    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    spacing: 10

    Text { text: "——启动——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text {
            text: "启动类型"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            text: "0xFFFFFF03"
            enabled: false
            width: 140
            height: 32
            color: root.text3
            font.pixelSize: 13
            background: Rectangle {
                color: "#F3F4F6"
                radius: 6
                border.width: 1
                border.color: "#D1D5DB"
            }
        }
    }
}
