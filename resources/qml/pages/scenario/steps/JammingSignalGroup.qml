// JammingSignalGroup.qml - 压制干扰源信号组组件
import QtQuick
import QtQuick.Controls

Column {
    id: groupRoot

    property string title: ""
    property int channelCount: 3
    property int channelHeight: 100
    property var detailFields: []
    property var signalData: ({})
    property bool isRefreshing: false

    signal channelDataChanged(int channelIndex, string key, var value)

    width: parent.width
    spacing: 10

    Text {
        text: groupRoot.title
        color: "#6B7280"
        font.pixelSize: 13
    }

    Grid {
        width: parent.width
        columns: 3
        columnSpacing: 10
        rowSpacing: 12

        Repeater {
            model: groupRoot.channelCount

            Rectangle {
                width: (parent.width - 20) / 3
                implicitHeight: channelColumn.implicitHeight + 16
                color: "white"
                radius: 6
                border.width: 1
                border.color: "#E5E7EB"

                property int channelIndex: index
                property var channelData: {
                    // 数组格式访问：signalData[0], signalData[1], signalData[2]
                    var data = groupRoot.signalData
                    if (data && data.length !== undefined && data.length > index && data[index]) {
                        return data[index]
                    }
                    return { paramType: "停止输出" }
                }

                Column {
                    id: channelColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    Text {
                        text: "通道" + (index + 1)
                        color: "#1677FF"
                        font.pixelSize: 12
                        font.bold: true
                        height: 20
                    }

                    Row {
                        width: parent.width
                        spacing: 4
                        Text {
                            text: "参数类型"
                            color: "#6B7280"
                            font.pixelSize: 11
                            width: 50
                            height: 22
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: paramTypeCombo
                            width: parent.width - 54
                            height: 22
                            model: ["信号设置", "停止输出"]
                            currentIndex: channelData.paramType === "信号设置" ? 0 : 1
                            font.pixelSize: 11
                            property bool __initialized: false
                            Component.onCompleted: __initialized = true
                            delegate: ItemDelegate {
                                width: paramTypeCombo.width
                                height: 22
                                contentItem: Text {
                                    text: modelData
                                    color: paramTypeCombo.highlightedIndex === index ? "white" : "#1F2937"
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 5
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: paramTypeCombo.highlightedIndex === index ? "#1677FF" : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 5
                                rightPadding: 18
                                text: paramTypeCombo.displayText
                                color: "#1F2937"
                                font.pixelSize: 11
                                verticalAlignment: Text.AlignVCenter
                            }
                            indicator: Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 4
                                anchors.verticalCenter: parent.verticalCenter
                                text: "▾"
                                color: "#6B7280"
                                font.pixelSize: 9
                            }
                            background: Rectangle {
                                color: "#F9FAFB"
                                border.color: "#D1D5DB"
                                border.width: 1
                                radius: 4
                            }
                            popup: Popup {
                                y: paramTypeCombo.height
                                width: paramTypeCombo.width
                                implicitHeight: contentItem.implicitHeight + 4
                                padding: 2
                                background: Rectangle {
                                    color: "white"
                                    radius: 4
                                    border.width: 1
                                    border.color: "#D1D5DB"
                                }
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: contentHeight
                                    model: paramTypeCombo.delegateModel
                                }
                            }
                            onCurrentTextChanged: {
                                if (groupRoot.isRefreshing || !paramTypeCombo.__initialized) return
                                groupRoot.channelDataChanged(index, "paramType", currentText)
                            }
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 4
                        enabled: paramTypeCombo.currentText === "信号设置"
                        opacity: enabled ? 1.0 : 0.5

                        Repeater {
                            model: groupRoot.detailFields

                            Row {
                                width: parent.width
                                spacing: 4

                                Text {
                                    text: modelData.label
                                    color: "#6B7280"
                                    font.pixelSize: 11
                                    width: 50
                                    height: 22
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Loader {
                                    width: parent.width - 54
                                    height: 22
                                    sourceComponent: modelData.type === "combo" ? comboComponent : textComponent

                                    property var fieldData: modelData
                                    property int fieldChannelIndex: channelIndex
                                    property var fieldChannelData: channelData

                                    Component {
                                        id: textComponent
                                        TextField {
                                            width: parent.width
                                            height: 22
                                            color: "#1F2937"
                                            font.pixelSize: 11
                                            text: {
                                                var d = fieldChannelData || {}
                                                var val = d[fieldData.key]
                                                return val !== undefined ? String(val) : (fieldData.placeholder || "-127.6")
                                            }
                                            background: Rectangle {
                                                color: "#F9FAFB"
                                                border.color: "#D1D5DB"
                                                border.width: 1
                                                radius: 4
                                            }
                                            onEditingFinished: {
                                                if (groupRoot.isRefreshing) return
                                                groupRoot.channelDataChanged(fieldChannelIndex, fieldData.key, text)
                                            }
                                        }
                                    }

                                    Component {
                                        id: comboComponent
                                        ComboBox {
                                            id: fieldCombo
                                            width: parent.width
                                            height: 22
                                            model: fieldData.options || []
                                            currentIndex: {
                                                var d = fieldChannelData || {}
                                                var val = d[fieldData.key]
                                                if (val !== undefined && fieldData.options) {
                                                    var idx = fieldData.options.indexOf(val)
                                                    return idx >= 0 ? idx : 0
                                                }
                                                return 0
                                            }
                                            font.pixelSize: 11
                                            property bool __initialized: false
                                            Component.onCompleted: __initialized = true
                                            delegate: ItemDelegate {
                                                width: fieldCombo.width
                                                height: 22
                                                contentItem: Text {
                                                    text: modelData
                                                    color: fieldCombo.highlightedIndex === index ? "white" : "#1F2937"
                                                    font.pixelSize: 11
                                                    verticalAlignment: Text.AlignVCenter
                                                    leftPadding: 5
                                                }
                                                background: Rectangle {
                                                    radius: 4
                                                    color: fieldCombo.highlightedIndex === index ? "#1677FF" : (hovered ? "#EEF2FF" : "transparent")
                                                }
                                            }
                                            contentItem: Text {
                                                leftPadding: 5
                                                rightPadding: 18
                                                text: fieldCombo.displayText
                                                color: "#1F2937"
                                                font.pixelSize: 11
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            indicator: Text {
                                                anchors.right: parent.right
                                                anchors.rightMargin: 4
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: "▾"
                                                color: "#6B7280"
                                                font.pixelSize: 9
                                            }
                                            background: Rectangle {
                                                color: "#F9FAFB"
                                                border.color: "#D1D5DB"
                                                border.width: 1
                                                radius: 4
                                            }
                                            popup: Popup {
                                                y: fieldCombo.height
                                                width: fieldCombo.width
                                                implicitHeight: contentItem.implicitHeight + 4
                                                padding: 2
                                                background: Rectangle {
                                                    color: "white"
                                                    radius: 4
                                                    border.width: 1
                                                    border.color: "#D1D5DB"
                                                }
                                                contentItem: ListView {
                                                    clip: true
                                                    implicitHeight: contentHeight
                                                    model: fieldCombo.delegateModel
                                                }
                                            }
                                            onCurrentTextChanged: {
                                                if (groupRoot.isRefreshing || !fieldCombo.__initialized) return
                                                groupRoot.channelDataChanged(fieldChannelIndex, fieldData.key, currentText)
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
