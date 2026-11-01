// Step2_DeviceConfig.qml - 步骤2：待测设备配置
import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var detailData: ({})

    // 信号：通知父组件数据已更新
    signal updateDetailData(string key, var value)

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 组件加载时不输出日志

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Flickable {
            id: flickable
            anchors.fill: parent
            clip: true
            contentHeight: contentColumn.height + 20
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                width: flickable.width - 16
                anchors.left: parent.left
                anchors.leftMargin: 8
                spacing: 16

                // --- 复位 ---
                Column {
                    width: parent.width
                    spacing: 10
                    Text { text: "——复位——"; color: root.text2; font.pixelSize: 13 }

                    Row {
                        spacing: 10
                        Text { text: "复位指令"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        Text {
                            text: "$CCRIS,*64<CR><LF>"
                            color: root.text1
                            font.pixelSize: 13
                            height: 32
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }

                // --- RMO ---
                Column {
                    width: parent.width
                    spacing: 10
                    Text { text: "——RMO——"; color: root.text2; font.pixelSize: 13 }

                    Row {
                        spacing: 10
                        Text { text: "目标语句"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        ComboBox {
                            id: targetSentenceCombo
                            width: 180
                            height: 32
                            implicitHeight: 32
                            font.pixelSize: 13
                            model: ["GGA", "DHV"]
                            currentIndex: root.detailData && root.detailData.rmoTargetStatement === "DHV" ? 1 : 0
                            property bool __initialized: false
                            Component.onCompleted: __initialized = true
                            onCurrentTextChanged: {
                                if (__initialized) root.updateData("rmoTargetStatement", currentText)
                            }
                            delegate: ItemDelegate {
                                width: targetSentenceCombo.width
                                contentItem: Text {
                                    text: modelData
                                    color: (targetSentenceCombo.highlightedIndex === index) ? "white" : root.text1
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 6
                                    color: (targetSentenceCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 10
                                rightPadding: 28
                                text: targetSentenceCombo.displayText
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
                                y: targetSentenceCombo.height + 4
                                width: targetSentenceCombo.width
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
                                    model: targetSentenceCombo.delegateModel
                                }
                            }
                        }
                    }

                    Row {
                        spacing: 10
                        Text { text: "打开模式"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        ComboBox {
                            id: openModeCombo
                            width: 180
                            height: 32
                            implicitHeight: 32
                            font.pixelSize: 13
                            model: ["关闭指令", "打开指令", "关闭全部", "打开全部"]
                            currentIndex: {
                                var mode = root.detailData ? root.detailData.rmoOpenMode : "CLOSE"
                                if (mode === "OPEN" || mode === "打开指令" || mode === "打开全部") {
                                    return mode === "打开全部" ? 3 : 1
                                }
                                return mode === "关闭全部" ? 2 : 0
                            }
                            property bool __initialized: false
                            Component.onCompleted: __initialized = true
                            onCurrentTextChanged: {
                                if (__initialized) {
                                    root.updateData("rmoOpenMode", currentText)
                                }
                            }
                            delegate: ItemDelegate {
                                width: openModeCombo.width
                                contentItem: Text {
                                    text: modelData
                                    color: (openModeCombo.highlightedIndex === index) ? "white" : root.text1
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 6
                                    color: (openModeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 10
                                rightPadding: 28
                                text: openModeCombo.displayText
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
                                y: openModeCombo.height + 4
                                width: openModeCombo.width
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
                                    model: openModeCombo.delegateModel
                                }
                            }
                        }
                    }

                    Row {
                        spacing: 10
                        Text { text: "输出频率"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            id: outputFreqField
                            width: 120
                            height: 32
                            color: root.text1
                            font.pixelSize: 13
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: (root.detailData && root.detailData.rmoOutputFreq !== undefined) ? String(root.detailData.rmoOutputFreq) : ""
                            placeholderText: "请输入"
                            background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
                            // 只在用户主动编辑时更新数据
                            onTextChanged: { if (activeFocus) root.updateData("rmoOutputFreq", parseInt(text) || 1) }
                        }
                        Text { text: "S"; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }
        }
    }

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }
}