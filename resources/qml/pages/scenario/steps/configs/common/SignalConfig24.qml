// SignalConfig24.qml - 24通道信号设置配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    property string sceneType: ""
    property string channelsKey: ""      // 通道数据键名，由父组件传入
    property string freqKey: ""          // 频点数据键名，由父组件传入

    signal updateDetailData(string key, var value)

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"

    property var freqOptions: ["GPS_L1", "BDS_B1", "GAL_E1"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    function updateChannel(index, field, value) {
        if (!root.detailData) return
        var channels = root.detailData[root.channelsKey] ? JSON.parse(JSON.stringify(root.detailData[root.channelsKey])) : []
        var existingData = channels[index] || {}
        channels[index] = {
            switchType: existingData.switchType || "关闭",
            powerValue: existingData.powerValue !== undefined ? existingData.powerValue : -130,
            powerQ: existingData.powerQ !== undefined ? existingData.powerQ : -130,
            modulation: existingData.modulation || "QPSK"
        }
        channels[index][field] = value
        root.updateData(root.channelsKey, channels)
    }

    // 获取通道数据
    function getChannelsData() {
        if (!root.detailData) return []
        var data = root.detailData[root.channelsKey]
        if (data && data.length > 0) return data
        return root.detailData["channels"] || []
    }

    Text { text: "——信号设置——"; color: root.text2; font.pixelSize: 13 }

    // 频点选择
    Row {
        spacing: 10
        Text {
            text: "频点"
            color: root.text2
            font.pixelSize: 13
            width: 60
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        ComboBox {
            id: freqCombo
            width: 140
            height: 32
            font.pixelSize: 13
            model: root.freqOptions
            currentIndex: {
                var freqVal = root.detailData ? (root.detailData[root.freqKey] || root.detailData["globalFreq"]) : ""
                return freqVal ? Math.max(0, model.indexOf(freqVal)) : 0
            }
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized && visible) {
                    root.updateData(root.freqKey, currentText)
                }
            }
            delegate: ItemDelegate {
                width: freqCombo.width
                contentItem: Text {
                    text: modelData
                    color: (freqCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (freqCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: freqCombo.displayText
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
                y: freqCombo.height + 4
                width: freqCombo.width
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
                    model: freqCombo.delegateModel
                }
            }
        }
    }

    Text {
        text: "通道配置（24 通道）"
        color: root.text1
        font.pixelSize: 13
        font.bold: true
    }

    // 24通道网格
    Grid {
        width: parent.width
        columns: 6
        columnSpacing: 10
        rowSpacing: 10

        Repeater {
            model: 24

            delegate: Rectangle {
                width: (parent.width - 50) / 6
                height: 100
                color: "white"
                radius: 8
                border.width: 1
                border.color: "#E5E7EB"

                property int chIndex: index
                property var chData: {
                    var channels = root.getChannelsData()
                    var d = (channels && channels[index]) || {}
                    return {
                        switchType: d.switchType || "关闭",
                        powerValue: d.powerValue !== undefined ? d.powerValue : -130,
                        powerQ: d.powerQ !== undefined ? d.powerQ : -130,
                        modulation: d.modulation || "QPSK"
                    }
                }

                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 8

                    Text {
                        text: "通道 " + (index + 1)
                        color: root.primary
                        font.pixelSize: 13
                        font.bold: true
                    }

                    // 通道开关
                    Row {
                        width: parent.width
                        spacing: 4
                        Text {
                            text: "开关"
                            color: root.text2
                            font.pixelSize: 11
                            width: 32
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: switchCombo
                            width: parent.width - 36
                            height: 26
                            model: ["开启", "关闭"]
                            currentIndex: chData.switchType === "开启" ? 0 : 1
                            font.pixelSize: 11
                            property bool __initialized: false
                            Component.onCompleted: __initialized = true
                            onCurrentTextChanged: {
                                if (!__initialized || !root.detailData || !visible) return
                                root.updateChannel(index, "switchType", currentText)
                            }
                            delegate: ItemDelegate {
                                width: switchCombo.width
                                height: 24
                                contentItem: Text {
                                    text: modelData
                                    color: switchCombo.highlightedIndex === index ? "white" : root.text1
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 6
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: switchCombo.highlightedIndex === index ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 6
                                rightPadding: 22
                                text: switchCombo.displayText
                                color: root.text1
                                font.pixelSize: 11
                                verticalAlignment: Text.AlignVCenter
                            }
                            indicator: Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                                text: "▾"
                                color: root.text2
                                font.pixelSize: 10
                            }
                            background: Rectangle {
                                color: "#F9FAFB"
                                border.color: "#D1D5DB"
                                border.width: 1
                                radius: 4
                            }
                            popup: Popup {
                                y: switchCombo.height
                                width: switchCombo.width
                                implicitHeight: contentItem.implicitHeight + 6
                                padding: 3
                                background: Rectangle {
                                    color: "white"
                                    radius: 6
                                    border.width: 1
                                    border.color: "#D1D5DB"
                                }
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: switchCombo.delegateModel
                                }
                            }
                        }
                    }

                    // 功率值
                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: switchCombo.currentText === "开启"
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "功率"
                            color: root.text2
                            font.pixelSize: 11
                            width: 32
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            id: powerField
                            width: parent.width - 36
                            height: 26
                            color: root.text1
                            font.pixelSize: 11
                            text: chData.powerValue.toFixed(1)
                            validator: DoubleValidator { bottom: -500; top: 500; decimals: 1 }
                            maximumLength: 50
                            property bool isOutOfRange: {
                                var val = parseFloat(text)
                                return !isNaN(val) && Math.abs(val) > 500
                            }
                            background: Rectangle {
                                color: "#F9FAFB"
                                border.color: powerField.isOutOfRange ? "#EF4444" : "#D1D5DB"
                                border.width: powerField.isOutOfRange ? 2 : 1
                                radius: 4
                            }
                            onEditingFinished: {
                                if (!root.detailData) return
                                var val = parseFloat(text)
                                if (isOutOfRange) {
                                    // 恢复为有效值
                                    text = chData.powerValue.toFixed(1)
                                    return
                                }
                                root.updateChannel(index, "powerValue", val || -130)
                            }
                        }
                    }
                }
            }
        }
    }
}
