// JammerCountConfig.qml - 干扰机数量配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——干扰机数量——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text {
            text: "数量"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        ComboBox {
            id: jammerCountCombo
            width: 140
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: ["1"]
            currentIndex: 0
            onCurrentTextChanged: {
                root.updateData("jammerCount", currentText)
            }
            delegate: ItemDelegate {
                width: jammerCountCombo.width
                contentItem: Text {
                    text: modelData
                    color: (jammerCountCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (jammerCountCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: jammerCountCombo.displayText
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
                border.color: "#D1D5DB"
                border.width: 1
                radius: 6
            }
            popup: Popup {
                y: jammerCountCombo.height + 4
                width: jammerCountCombo.width
                implicitHeight: contentItem.implicitHeight
                padding: 6
                background: Rectangle {
                    color: "white"
                    radius: 8
                    border.width: 1
                    border.color: "#D1D5DB"
                }
                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: jammerCountCombo.delegateModel
                }
            }
        }
    }
}
