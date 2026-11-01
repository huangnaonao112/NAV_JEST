// SignalConfig36.qml - 36通道信号设置配置（中高轨专用）
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    property string sceneType: ""
    property string channelsKey: "channels"
    property string freqKey: "freqSelect"

    signal updateDetailData(string key, var value)

    property bool isRefreshing: false

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"

    property var freqOptions: ["GPS_L1", "GPS_L2", "GPS_L5", "BD2_B1", "BD2_B2", "BD2_B3", "BD2_B1C", "BD2_B2A", "BD2_B2B", "GLO_G1"]
    property var modulationOptions: ["0-关闭", "1-BPSK-Q/导频", "2-BPSK-I/数据", "3-QPSK/数据导频", "15-单载波"]
    property var switchOptions: ["0-关闭", "1-打开"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    // 强制刷新所有UI
    function forceRefreshAllUI() {
        root.isRefreshing = true

        // 设置频点
        var freq = root.detailData[root.freqKey] || "GPS_L1"
        var freqIdx = root.freqOptions.indexOf(freq)
        if (freqIdx < 0) freqIdx = 0
        freqCombo.currentIndex = freqIdx

        // 设置所有通道
        var channels = root.detailData[root.channelsKey] || []
        for (var i = 0; i < 36; i++) {
            var ch = channels[i] || {}

            var chSw = ch.chSwitch || "0-关闭"
            var chSwIdx = root.switchOptions.indexOf(chSw)
            if (chSwIdx < 0) chSwIdx = 0

            var mod = ch.modulation || "0-关闭"
            var modIdx = root.modulationOptions.indexOf(mod)
            if (modIdx < 0) modIdx = 0

            var pI = ch.powerI !== undefined ? ch.powerI : -127.6
            var pQ = ch.powerQ !== undefined ? ch.powerQ : -127.6

            var freqSw = ch.freqSwitch || "0-关闭"
            var freqSwIdx = root.switchOptions.indexOf(freqSw)
            if (freqSwIdx < 0) freqSwIdx = 0

            if (repeaterItems.itemAt(i)) {
                var item = repeaterItems.itemAt(i)
                if (item.chSwitchCombo) item.chSwitchCombo.currentIndex = chSwIdx
                if (item.modulationCombo) item.modulationCombo.currentIndex = modIdx
                if (item.powerIField) item.powerIField.text = pI.toString()
                if (item.powerQField) item.powerQField.text = pQ.toString()
                if (item.freqSwitchCombo) item.freqSwitchCombo.currentIndex = freqSwIdx
            }
        }

        root.isRefreshing = false
    }

    onDetailDataChanged: {
        if (visible) Qt.callLater(forceRefreshAllUI)
    }

    onVisibleChanged: {
        if (visible) Qt.callLater(forceRefreshAllUI)
    }

    Text { text: "——信号设置——"; color: root.text2; font.pixelSize: 13 }

    // 频点选择
    Row {
        spacing: 10
        Text {
            text: "频点选择"
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
            implicitHeight: 32
            font.pixelSize: 13
            model: root.freqOptions
            currentIndex: 0
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (root.isRefreshing || !__initialized || !visible) return
                root.updateData(root.freqKey, currentText)
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
        text: "通道配置（36 通道）"
        color: root.text1
        font.pixelSize: 13
        font.bold: true
    }

    // 36通道网格
    Grid {
        id: channelsGrid
        width: parent.width
        columns: 6
        columnSpacing: 10
        rowSpacing: 10

        Repeater {
            id: repeaterItems
            model: 36

            Rectangle {
                id: channelRect
                width: (parent.width - 50) / 6
                height: 182
                color: "white"
                radius: 8
                border.width: 1
                border.color: "#E5E7EB"

                property int chIndex: index
                property alias chSwitchCombo: chSwitchCombo
                property alias modulationCombo: modulationCombo
                property alias powerIField: powerIField
                property alias powerQField: powerQField
                property alias freqSwitchCombo: freqSwitchCombo

                property bool isChannelOn: chSwitchCombo.currentIndex === 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

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
                            text: "通道开关"
                            color: root.text2
                            font.pixelSize: 11
                            width: 59
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: chSwitchCombo
                            width: parent.width - 63
                            height: 26
                            model: root.switchOptions
                            currentIndex: 0
                            font.pixelSize: 11
                            delegate: ItemDelegate {
                                width: chSwitchCombo.width
                                height: 24
                                contentItem: Text {
                                    text: modelData
                                    color: chSwitchCombo.highlightedIndex === index ? "white" : root.text1
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 6
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: chSwitchCombo.highlightedIndex === index ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 6
                                rightPadding: 22
                                text: chSwitchCombo.displayText
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
                                y: chSwitchCombo.height
                                width: chSwitchCombo.width
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
                                    model: chSwitchCombo.delegateModel
                                    boundsBehavior: Flickable.StopAtBounds
                                }
                            }
                            onCurrentTextChanged: {
                                if (root.isRefreshing) return
                                if (!root.detailData) return
                                var arr = root.detailData[root.channelsKey] ? JSON.parse(JSON.stringify(root.detailData[root.channelsKey])) : []
                                var ch = arr[index] || {}
                                ch.chSwitch = currentText
                                arr[index] = ch
                                root.updateData(root.channelsKey, arr)
                            }
                        }
                    }

                    // 调制类型
                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: channelRect.isChannelOn
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "调制类型"
                            color: root.text2
                            font.pixelSize: 11
                            width: 59
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: modulationCombo
                            width: parent.width - 63
                            height: 26
                            model: root.modulationOptions
                            currentIndex: 0
                            font.pixelSize: 11
                            delegate: ItemDelegate {
                                width: modulationCombo.width
                                height: 24
                                contentItem: Text {
                                    text: modelData
                                    color: modulationCombo.highlightedIndex === index ? "white" : root.text1
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 6
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: modulationCombo.highlightedIndex === index ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 6
                                rightPadding: 22
                                text: modulationCombo.displayText
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
                                color: parent.parent.enabled ? "#F9FAFB" : "#F3F4F6"
                                border.color: "#D1D5DB"
                                border.width: 1
                                radius: 4
                            }
                            popup: Popup {
                                y: modulationCombo.height
                                width: modulationCombo.width
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
                                    model: modulationCombo.delegateModel
                                    boundsBehavior: Flickable.StopAtBounds
                                }
                            }
                            onCurrentTextChanged: {
                                if (root.isRefreshing) return
                                if (!root.detailData) return
                                var arr = root.detailData[root.channelsKey] ? JSON.parse(JSON.stringify(root.detailData[root.channelsKey])) : []
                                var ch = arr[index] || {}
                                ch.modulation = currentText
                                arr[index] = ch
                                root.updateData(root.channelsKey, arr)
                            }
                        }
                    }

                    // I 功率
                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: channelRect.isChannelOn
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "I 功率"
                            color: root.text2
                            font.pixelSize: 11
                            width: 59
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            id: powerIField
                            width: parent.width - 63
                            height: 26
                            color: root.text1
                            font.pixelSize: 11
                            text: "-127.6"
                            validator: DoubleValidator { decimals: 1; bottom: -500; top: 500 }
                            maximumLength: 50
                            property bool isOutOfRange: {
                                var val = parseFloat(text)
                                return !isNaN(val) && Math.abs(val) > 500
                            }
                            background: Rectangle {
                                color: parent.parent.enabled ? "white" : "#F3F4F6"
                                border.color: powerIField.isOutOfRange ? "#EF4444" : "#D1D5DB"
                                border.width: powerIField.isOutOfRange ? 2 : 1
                                radius: 4
                            }
                            onTextChanged: {
                                if (root.isRefreshing) return
                                if (!root.detailData || !activeFocus) return
                                var val = parseFloat(text)
                                if (isOutOfRange) return  // 绝对值超过500，不保存
                                if (!acceptableInput) return
                                var arr = root.detailData[root.channelsKey] ? JSON.parse(JSON.stringify(root.detailData[root.channelsKey])) : []
                                var ch = arr[index] || {}
                                if (!isNaN(val)) {
                                    ch.powerI = val
                                    arr[index] = ch
                                    root.updateData(root.channelsKey, arr)
                                }
                            }
                        }
                    }

                    // Q 功率
                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: channelRect.isChannelOn
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "Q 功率"
                            color: root.text2
                            font.pixelSize: 11
                            width: 59
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            id: powerQField
                            width: parent.width - 63
                            height: 26
                            color: root.text1
                            font.pixelSize: 11
                            text: "-127.6"
                            validator: DoubleValidator { decimals: 1; bottom: -500; top: 500 }
                            maximumLength: 50
                            property bool isOutOfRange: {
                                var val = parseFloat(text)
                                return !isNaN(val) && Math.abs(val) > 500
                            }
                            background: Rectangle {
                                color: parent.parent.enabled ? "white" : "#F3F4F6"
                                border.color: powerQField.isOutOfRange ? "#EF4444" : "#D1D5DB"
                                border.width: powerQField.isOutOfRange ? 2 : 1
                                radius: 4
                            }
                            onTextChanged: {
                                if (root.isRefreshing) return
                                if (!root.detailData || !activeFocus) return
                                var val = parseFloat(text)
                                if (isOutOfRange) return  // 绝对值超过500，不保存
                                if (!acceptableInput) return
                                var arr = root.detailData[root.channelsKey] ? JSON.parse(JSON.stringify(root.detailData[root.channelsKey])) : []
                                var ch = arr[index] || {}
                                if (!isNaN(val)) {
                                    ch.powerQ = val
                                    arr[index] = ch
                                    root.updateData(root.channelsKey, arr)
                                }
                            }
                        }
                    }

                    // 频点开关
                    Row {
                        width: parent.width
                        spacing: 4
                        enabled: channelRect.isChannelOn
                        opacity: enabled ? 1.0 : 0.5
                        Text {
                            text: "频点开关"
                            color: root.text2
                            font.pixelSize: 11
                            width: 59
                            height: 26
                            verticalAlignment: Text.AlignVCenter
                        }
                        ComboBox {
                            id: freqSwitchCombo
                            width: parent.width - 63
                            height: 26
                            model: root.switchOptions
                            currentIndex: 0
                            font.pixelSize: 11
                            delegate: ItemDelegate {
                                width: freqSwitchCombo.width
                                height: 24
                                contentItem: Text {
                                    text: modelData
                                    color: freqSwitchCombo.highlightedIndex === index ? "white" : root.text1
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 6
                                }
                                background: Rectangle {
                                    radius: 4
                                    color: freqSwitchCombo.highlightedIndex === index ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                                }
                            }
                            contentItem: Text {
                                leftPadding: 6
                                rightPadding: 22
                                text: freqSwitchCombo.displayText
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
                                color: parent.parent.enabled ? "#F9FAFB" : "#F3F4F6"
                                border.color: "#D1D5DB"
                                border.width: 1
                                radius: 4
                            }
                            popup: Popup {
                                y: freqSwitchCombo.height
                                width: freqSwitchCombo.width
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
                                    model: freqSwitchCombo.delegateModel
                                    boundsBehavior: Flickable.StopAtBounds
                                }
                            }
                            onCurrentTextChanged: {
                                if (root.isRefreshing) return
                                if (!root.detailData) return
                                var arr = root.detailData[root.channelsKey] ? JSON.parse(JSON.stringify(root.detailData[root.channelsKey])) : []
                                var ch = arr[index] || {}
                                ch.freqSwitch = currentText
                                arr[index] = ch
                                root.updateData(root.channelsKey, arr)
                            }
                        }
                    }
                }
            }
        }
    }
}
