// TrackOutputConfig.qml - 轨迹输出配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    property string dataKey: "trackOutput"

    signal updateDetailData(string key, var value)

    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color primary: "#1677FF"

    property var trackOutputOptions: ["停止输出", "开始输出"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——轨迹输出——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text {
            text: "轨迹输出"
            color: root.text2
            font.pixelSize: 13
            width: 60
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        ComboBox {
            id: trackOutputCombo
            width: 140
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: root.trackOutputOptions
            currentIndex: {
                var val = root.detailData ? root.detailData[root.dataKey] : ""
                var idx = root.trackOutputOptions.indexOf(val)
                return idx >= 0 ? idx : 0
            }
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized && visible) {
                    root.updateData(root.dataKey, currentText)
                }
            }
            delegate: ItemDelegate {
                width: trackOutputCombo.width
                contentItem: Text {
                    text: modelData
                    color: (trackOutputCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (trackOutputCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: trackOutputCombo.displayText
                color: root.text1
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
            indicator: Text {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: "▾"
                color: root.text2
                font.pixelSize: 14
            }
            background: Rectangle {
                color: "white"
                border.color: "#BBDEFB"
                border.width: 1
                radius: 6
            }
            popup: Popup {
                y: trackOutputCombo.height + 4
                width: trackOutputCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 6
                background: Rectangle {
                    color: "white"
                    radius: 8
                    border.width: 1
                    border.color: "#BBDEFB"
                }
                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: trackOutputCombo.delegateModel
                }
            }
        }
    }
}
