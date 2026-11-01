// SecurityDeceptionParamsConfig.qml - 欺骗参数配置（安全认证专用）
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    property var signalTypeList: ["BDS_B1I", "BDS_B1Q", "BDS_B1C", "BDS_B1A", "BDS_B2a", "BDS_B2b",
        "BDS_B3I", "BDS_B3A", "BDS_B3AE", "GPS_L1C/A", "GPS_L1C", "GPS_L1P",
        "GPS_L2C", "GPS_L2P", "GPS_L5"]
    property var powerModeList: ["手动模式", "自动模式"]
    property var ephemerisModeList: ["对天接收", "自主星历"]
    property var deceptionModeList: ["轨迹欺骗", "手动拉偏"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——欺骗参数——"; color: root.text2; font.pixelSize: 13; font.bold: true }

    // 欺骗信号
    Row {
        spacing: 10
        Text { text: "欺骗信号"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
        ComboBox {
            id: deceptionSignalCombo
            width: 180
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: root.signalTypeList
            currentIndex: root.detailData && root.detailData.deceptionSignal ? Math.max(0, model.indexOf(root.detailData.deceptionSignal)) : 0
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized) root.updateData("deceptionSignal", currentText)
            }
            delegate: ItemDelegate {
                width: deceptionSignalCombo.width
                contentItem: Text {
                    text: modelData
                    color: (deceptionSignalCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (deceptionSignalCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: deceptionSignalCombo.displayText
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
                y: deceptionSignalCombo.height + 4
                width: deceptionSignalCombo.width
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
                    model: deceptionSignalCombo.delegateModel
                }
            }
        }
    }

    // 功率模式
    Row {
        spacing: 10
        Text { text: "功率模式"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
        ComboBox {
            id: powerModeCombo
            width: 140
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: root.powerModeList
            currentIndex: root.detailData && root.detailData.powerMode === "自动模式" ? 1 : 0
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized) root.updateData("powerMode", currentText)
            }
            delegate: ItemDelegate {
                width: powerModeCombo.width
                contentItem: Text {
                    text: modelData
                    color: (powerModeCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (powerModeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: powerModeCombo.displayText
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
                y: powerModeCombo.height + 4
                width: powerModeCombo.width
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
                    model: powerModeCombo.delegateModel
                }
            }
        }
    }

    // 初始功率
    Row {
        spacing: 10
        Text { text: "初始功率"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
        TextField {
            id: initPowerField
            width: 120
            height: 32
            color: root.text1
            font.pixelSize: 13
            text: (root.detailData && root.detailData.initPower !== undefined) ? String(root.detailData.initPower) : "-130"
            validator: DoubleValidator { bottom: -500; top: 500; decimals: 1 }
            maximumLength: 50
            property bool isOutOfRange: {
                var val = parseFloat(text)
                return !isNaN(val) && Math.abs(val) > 500
            }
            background: Rectangle { color: "white"; radius: 6; border.color: initPowerField.isOutOfRange ? "#EF4444" : "#BBDEFB"; border.width: initPowerField.isOutOfRange ? 2 : 1 }
            onTextChanged: {
                if (activeFocus && !isOutOfRange) {
                    root.updateData("initPower", parseFloat(text) || -130)
                }
            }
        }
        Text { text: "dB"; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
    }

    // 星历模式
    Row {
        spacing: 10
        Text { text: "星历模式"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
        ComboBox {
            id: ephemerisModeCombo
            width: 140
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: root.ephemerisModeList
            currentIndex: root.detailData && root.detailData.ephemerisMode === "自主星历" ? 1 : 0
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized) root.updateData("ephemerisMode", currentText)
            }
            delegate: ItemDelegate {
                width: ephemerisModeCombo.width
                contentItem: Text {
                    text: modelData
                    color: (ephemerisModeCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (ephemerisModeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: ephemerisModeCombo.displayText
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
                y: ephemerisModeCombo.height + 4
                width: ephemerisModeCombo.width
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
                    model: ephemerisModeCombo.delegateModel
                }
            }
        }
    }

    // 欺骗模式
    Row {
        spacing: 10
        Text { text: "欺骗模式"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
        ComboBox {
            id: deceptionModeCombo
            width: 140
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: root.deceptionModeList
            currentIndex: root.detailData && root.detailData.deceptionMode === "手动拉偏" ? 1 : 0
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized) root.updateData("deceptionMode", currentText)
            }
            delegate: ItemDelegate {
                width: deceptionModeCombo.width
                contentItem: Text {
                    text: modelData
                    color: (deceptionModeCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (deceptionModeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: deceptionModeCombo.displayText
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
                y: deceptionModeCombo.height + 4
                width: deceptionModeCombo.width
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
                    model: deceptionModeCombo.delegateModel
                }
            }
        }
    }
}
