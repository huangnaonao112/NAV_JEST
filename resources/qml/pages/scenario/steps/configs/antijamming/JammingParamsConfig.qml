// JammingParamsConfig.qml - 干扰参数配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    property var jammingModeOptions: ["固定模式", "随机模式"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——干扰参数——"; color: root.text2; font.pixelSize: 13 }

    Grid {
        width: parent.width
        columns: 2
        columnSpacing: 10
        rowSpacing: 8

        // 介入时间
        Text {
            text: "介入时间"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            id: interventionTimeField
            width: 140
            height: 32
            color: root.text1
            font.pixelSize: 13
            text: (root.detailData && root.detailData.interventionTime !== undefined) ? String(root.detailData.interventionTime) : ""
            placeholderText: "请输入介入时间"
            background: Rectangle {
                color: "white"
                radius: 6
                border.width: 1
                border.color: "#BBDEFB"
            }
            onEditingFinished: {
                root.updateData("interventionTime", text)
            }
        }

        // 持续时间
        Text {
            text: "持续时间"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            id: durationField
            width: 140
            height: 32
            color: root.text1
            font.pixelSize: 13
            text: (root.detailData && root.detailData.jammingDuration !== undefined) ? String(root.detailData.jammingDuration) : ""
            placeholderText: "请输入持续时间"
            background: Rectangle {
                color: "white"
                radius: 6
                border.width: 1
                border.color: "#BBDEFB"
            }
            onEditingFinished: {
                root.updateData("jammingDuration", text)
            }
        }

        // 干扰方向
        Text {
            text: "干扰方向"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            id: jammingDirectionField
            width: 140
            height: 32
            color: root.text1
            font.pixelSize: 13
            text: (root.detailData && root.detailData.jammingDirection !== undefined) ? String(root.detailData.jammingDirection) : ""
            placeholderText: "请输入干扰方向"
            background: Rectangle {
                color: "white"
                radius: 6
                border.width: 1
                border.color: "#BBDEFB"
            }
            onEditingFinished: {
                root.updateData("jammingDirection", text)
            }
        }

        // 干扰方式
        Text {
            text: "干扰方式"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        ComboBox {
            id: jammingModeCombo
            width: 140
            height: 32
            font.pixelSize: 13
            model: root.jammingModeOptions
            currentIndex: {
                var val = root.detailData ? root.detailData.jammingMode : ""
                return val ? Math.max(0, root.jammingModeOptions.indexOf(val)) : 0
            }
            delegate: ItemDelegate {
                width: jammingModeCombo.width
                contentItem: Text {
                    text: modelData
                    color: (jammingModeCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (jammingModeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: jammingModeCombo.displayText
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
                y: jammingModeCombo.height + 4
                width: jammingModeCombo.width
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
                    model: jammingModeCombo.delegateModel
                }
            }
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized) {
                    root.updateData("jammingMode", currentText)
                }
            }
        }

        // 干扰功率
        Text {
            text: "干扰功率"
            color: root.text2
            font.pixelSize: 13
            width: 80
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            id: jammingPowerField
            width: 140
            height: 32
            color: root.text1
            font.pixelSize: 13
            text: (root.detailData && root.detailData.jammingPower !== undefined) ? String(root.detailData.jammingPower) : ""
            placeholderText: "请输入干扰功率"
            validator: DoubleValidator { bottom: -500; top: 500; decimals: 1 }
            maximumLength: 50
            property bool isOutOfRange: {
                var val = parseFloat(text)
                return !isNaN(val) && Math.abs(val) > 500
            }
            background: Rectangle {
                color: "white"
                radius: 6
                border.width: jammingPowerField.isOutOfRange ? 2 : 1
                border.color: jammingPowerField.isOutOfRange ? "#EF4444" : "#BBDEFB"
            }
            onEditingFinished: {
                if (isOutOfRange) {
                    text = (root.detailData && root.detailData.jammingPower !== undefined) ? String(root.detailData.jammingPower) : ""
                    return
                }
                root.updateData("jammingPower", text)
            }
        }
    }
}
