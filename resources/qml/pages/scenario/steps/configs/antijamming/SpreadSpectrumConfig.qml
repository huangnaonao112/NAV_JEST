// SpreadSpectrumConfig.qml - 扩频信号设置配置
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

    // 获取数组数据
    function getSignalArray() {
        if (!root.detailData) return []
        var arr = root.detailData["spreadSpectrum"]
        return (arr !== undefined && arr !== null && arr.length !== undefined) ? arr : []
    }

    // 更新通道数据 - 使用数组格式
    function updateChannelData(channelIndex, key, value) {
        if (!root.detailData) return
        var signalArray = getSignalArray()

        // 确保数组足够长
        while (signalArray.length <= channelIndex) {
            signalArray.push({ channelIndex: signalArray.length, paramType: "停止输出" })
        }

        // 更新指定索引的数据
        if (!signalArray[channelIndex]) {
            signalArray[channelIndex] = { channelIndex: channelIndex, paramType: "停止输出" }
        }
        signalArray[channelIndex][key] = value

        // 确保有 channelIndex
        signalArray[channelIndex].channelIndex = channelIndex

        root.updateData("spreadSpectrum", signalArray)
    }

    // 获取通道数据
    function getChannelData(index) {
        var arr = getSignalArray()
        for (var i = 0; i < arr.length; i++) {
            if (arr[i] && arr[i].channelIndex === index) {
                return arr[i]
            }
        }
        return { channelIndex: index, paramType: "停止输出" }
    }

    Text { text: "——扩频信号设置——"; color: root.text2; font.pixelSize: 13 }

    Grid {
        width: parent.width
        columns: 3
        columnSpacing: 10
        rowSpacing: 12

        Repeater {
            model: 3

            Rectangle {
                width: (parent.width - 20) / 3
                implicitHeight: channelColumn.implicitHeight + 16
                color: "white"
                radius: 6
                border.width: 1
                border.color: "#E5E7EB"

                property int channelIndex: index
                property var channelData: getChannelData(index)

                Column {
                    id: channelColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Text { text: "通道" + (index + 1); color: root.primary; font.pixelSize: 12; font.bold: true; height: 20 }

                    Row {
                        width: parent.width; spacing: 4
                        Text { text: "参数类型"; color: root.text2; font.pixelSize: 11; width: 50; height: 22; verticalAlignment: Text.AlignVCenter }
                        ComboBox {
                            id: paramTypeCombo
                            width: parent.width - 54; height: 22
                            model: ["信号设置", "停止输出"]
                            currentIndex: channelData.paramType === "信号设置" ? 0 : 1
                            font.pixelSize: 11
                            delegate: ItemDelegate {
                                width: paramTypeCombo.width; height: 22
                                contentItem: Text { text: modelData; color: paramTypeCombo.highlightedIndex === index ? "white" : root.text1; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; leftPadding: 5 }
                                background: Rectangle { radius: 4; color: paramTypeCombo.highlightedIndex === index ? root.primary : (hovered ? "#EEF2FF" : "transparent") }
                            }
                            contentItem: Text { leftPadding: 5; rightPadding: 18; text: paramTypeCombo.displayText; color: root.text1; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter }
                            indicator: Text { anchors.right: parent.right; anchors.rightMargin: 4; anchors.verticalCenter: parent.verticalCenter; text: "▾"; color: root.text2; font.pixelSize: 9 }
                            background: Rectangle { color: "#F9FAFB"; border.color: "#D1D5DB"; border.width: 1; radius: 4 }
                            popup: Popup {
                                y: paramTypeCombo.height
                                width: paramTypeCombo.width
                                implicitHeight: contentItem.implicitHeight + 4
                                padding: 2
                                background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#D1D5DB" }
                                contentItem: ListView { clip: true; implicitHeight: contentHeight; model: paramTypeCombo.delegateModel }
                            }
                            onCurrentTextChanged: root.updateChannelData(channelIndex, "paramType", currentText)
                        }
                    }

                    Row {
                        width: parent.width; spacing: 4
                        enabled: paramTypeCombo.currentText === "信号设置"
                        opacity: enabled ? 1.0 : 0.5
                        Text { text: "中心频率"; color: root.text2; font.pixelSize: 11; width: 50; height: 22; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            width: parent.width - 54; height: 22
                            color: root.text1; font.pixelSize: 11
                            text: channelData.centerFreq !== undefined ? String(channelData.centerFreq) : "-127.6"
                            background: Rectangle { color: "#F9FAFB"; border.color: "#D1D5DB"; border.width: 1; radius: 4 }
                            onEditingFinished: root.updateChannelData(channelIndex, "centerFreq", text)
                        }
                    }

                    Row {
                        width: parent.width; spacing: 4
                        enabled: paramTypeCombo.currentText === "信号设置"
                        opacity: enabled ? 1.0 : 0.5
                        Text { text: "信号功率"; color: root.text2; font.pixelSize: 11; width: 50; height: 22; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            id: signalPowerField
                            width: parent.width - 54; height: 22
                            color: root.text1; font.pixelSize: 11
                            text: channelData.signalPower !== undefined ? String(channelData.signalPower) : "-127.6"
                            validator: DoubleValidator { bottom: -500; top: 500; decimals: 1 }
                            maximumLength: 50
                            property bool isOutOfRange: {
                                var val = parseFloat(text)
                                return !isNaN(val) && Math.abs(val) > 500
                            }
                            background: Rectangle { color: "#F9FAFB"; border.color: signalPowerField.isOutOfRange ? "#EF4444" : "#D1D5DB"; border.width: signalPowerField.isOutOfRange ? 2 : 1; radius: 4 }
                            onEditingFinished: {
                                if (isOutOfRange) {
                                    text = channelData.signalPower !== undefined ? String(channelData.signalPower) : "-127.6"
                                    return
                                }
                                root.updateChannelData(channelIndex, "signalPower", text)
                            }
                        }
                    }

                    Row {
                        width: parent.width; spacing: 4
                        enabled: paramTypeCombo.currentText === "信号设置"
                        opacity: enabled ? 1.0 : 0.5
                        Text { text: "信号带宽"; color: root.text2; font.pixelSize: 11; width: 50; height: 22; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            width: parent.width - 54; height: 22
                            color: root.text1; font.pixelSize: 11
                            text: channelData.signalBandwidth !== undefined ? String(channelData.signalBandwidth) : "-127.6"
                            background: Rectangle { color: "#F9FAFB"; border.color: "#D1D5DB"; border.width: 1; radius: 4 }
                            onEditingFinished: root.updateChannelData(channelIndex, "signalBandwidth", text)
                        }
                    }
                }
            }
        }
    }
}
