// ScenePathConfig.qml - 场景路径配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    property string dataKey: "scenePath"  // 数据键名，可覆盖

    signal updateDetailData(string key, var value)

    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color primary: "#1677FF"

    spacing: 10

    Text { text: "——场景配置——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text {
            text: "场景路径"
            color: root.text2
            font.pixelSize: 13
            width: 60
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            id: scenePathField
            width: 300
            height: 32
            color: root.text1
            font.pixelSize: 13
            placeholderText: ""
            text: (root.detailData && root.detailData[root.dataKey]) ? root.detailData[root.dataKey] : ""
            readOnly: true
            background: Rectangle {
                color: "#F5F5F5"
                radius: 6
                border.width: 1
                border.color: "#BBDEFB"
            }
        }
    }
}
