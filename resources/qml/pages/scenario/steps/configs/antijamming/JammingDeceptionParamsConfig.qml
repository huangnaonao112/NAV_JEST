// JammingDeceptionParamsConfig.qml - 诱骗参数设置配置 (13频点)
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    property var freqOptions: [
        "GPS_L1", "GLO_G2", "GPS_L5", "BDS_B2I", "BDS_B3I",
        "BDS_B1C", "GAL_E5A", "GAL_E5B", "BDS_B2A", "BDS_B1I",
        "GLO_G1", "GAL_E1", "GPS_L2"
    ]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——诱骗参数——"; color: root.text2; font.pixelSize: 13 }

    // 13 个频点，每行 5 个
    Grid {
        width: parent.width
        columns: 5
        columnSpacing: 10
        rowSpacing: 10

        Repeater {
            model: 13

            Rectangle {
                width: (parent.width - 40) / 5
                implicitHeight: freqColumn.implicitHeight + 16
                color: "#F9FAFB"
                radius: 8
                border.width: 1
                border.color: "#D1D5DB"

                property string freqName: freqOptions[index]
                property var freqDataArray: root.detailData && root.detailData.deceptionFreq ? root.detailData.deceptionFreq : []
                property var freqData: {
                    var d = (freqDataArray && freqDataArray[index]) || {}
                    return {
                        switch: d.switch || "打开",
                        power: d.power !== undefined ? d.power : -127.6,
                        delay: d.delay !== undefined ? d.delay : -127.6
                    }
                }

                Column {
                    id: freqColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Text {
                        text: parent.parent.freqName
                        color: root.primary
                        font.pixelSize: 12
                        font.bold: true
                        width: parent.width
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Row {
                        width: parent.width
                        spacing: 4
                        Text {
                            text: "发射开关"
                            color: root.text2
                            font.pixelSize: 11
                            width: 55
                            height: 24
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: switchCombo
                            width: parent.width - 59
                            height: 24
                            font.pixelSize: 11
                            model: ["打开", "关闭"]
                            currentIndex: freqData.switch === "打开" ? 0 : 1
                            delegate: ItemDelegate {
                                width: switchCombo.width
                                height: 22
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
                                rightPadding: 20
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
                                    boundsBehavior: Flickable.StopAtBounds
                                }
                            }
                            onCurrentTextChanged: {
                                if (!root.detailData) return
                                var arr = root.detailData.deceptionFreq ? JSON.parse(JSON.stringify(root.detailData.deceptionFreq)) : []
                                var ch = arr[index] || {}
                                ch.switch = currentText
                                arr[index] = ch
                                root.updateData("deceptionFreq", arr)
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: switchCombo.currentText === "打开"
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "发射功率"
                            color: root.text2
                            font.pixelSize: 11
                            width: 55
                            height: 24
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            id: powerField
                            width: parent.width - 59
                            height: 24
                            color: root.text1
                            font.pixelSize: 11
                            text: freqData.power.toFixed(1)
                            validator: DoubleValidator { bottom: -500; top: 500; decimals: 1 }
                            maximumLength: 50
                            property bool isOutOfRange: {
                                var val = parseFloat(text)
                                return !isNaN(val) && Math.abs(val) > 500
                            }
                            background: Rectangle { color: "white"; border.color: powerField.isOutOfRange ? "#EF4444" : "#D1D5DB"; border.width: powerField.isOutOfRange ? 2 : 1; radius: 4 }
                            onEditingFinished: {
                                if (isOutOfRange) {
                                    text = freqData.power.toFixed(1)
                                    return
                                }
                                var arr = root.detailData.deceptionFreq ? JSON.parse(JSON.stringify(root.detailData.deceptionFreq)) : []
                                var d = arr[index] || {}
                                d.power = parseFloat(text) || -127.6
                                arr[index] = d
                                root.updateData("deceptionFreq", arr)
                            }
                        }
                    }

                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: switchCombo.currentText === "打开"
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "发射延时"
                            color: root.text2
                            font.pixelSize: 11
                            width: 55
                            height: 24
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            width: parent.width - 59
                            height: 24
                            color: root.text1
                            font.pixelSize: 11
                            text: freqData.delay.toFixed(1)
                            validator: DoubleValidator { bottom: -10000; top: 10000; decimals: 1 }
                            background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#D1D5DB" }
                            onEditingFinished: {
                                var arr = root.detailData.deceptionFreq ? JSON.parse(JSON.stringify(root.detailData.deceptionFreq)) : []
                                var d = arr[index] || {}
                                d.delay = parseFloat(text) || -127.6
                                arr[index] = d
                                root.updateData("deceptionFreq", arr)
                            }
                        }
                    }
                }
            }
        }
    }
}
