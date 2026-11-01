// SimulationErrorConfig.qml - 仿真误差参数配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    property var errorTypeOptions: ["大气误差", "轨道误差", "全部误差"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——仿真误差参数——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text {
            text: "误差类型"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        ComboBox {
            id: errorTypeCombo
            width: 140
            height: 32
            font.pixelSize: 13
            model: root.errorTypeOptions
            currentIndex: {
                var val = root.detailData ? root.detailData.simulationErrorType : ""
                return val ? Math.max(0, root.errorTypeOptions.indexOf(val)) : 0
            }
            delegate: ItemDelegate {
                width: errorTypeCombo.width
                contentItem: Text {
                    text: modelData
                    color: (errorTypeCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (errorTypeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: errorTypeCombo.displayText
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
                y: errorTypeCombo.height + 4
                width: errorTypeCombo.width
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
                    model: errorTypeCombo.delegateModel
                }
            }
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized) {
                    root.updateData("simulationErrorType", currentText)
                }
            }
        }
    }
}
