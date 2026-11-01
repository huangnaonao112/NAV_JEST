// MediumOrbitConfig.qml - 满天星配置（中高轨场景）
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "configs/common"

Column {
    id: root

    property var detailData: ({})
    property string sceneType: ""
    signal updateDetailData(string key, var value)

    property bool isRefreshing: false

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"
    property string text3: "#9CA3AF"

    spacing: 16

    onSceneTypeChanged: Qt.callLater(forceRefreshAllUI)
    onVisibleChanged: if (visible) Qt.callLater(forceRefreshAllUI)
    onDetailDataChanged: Qt.callLater(forceRefreshAllUI)

    function forceRefreshAllUI() {
        root.isRefreshing = true
        var trackOut = root.detailData.trackOutput || "停止输出"
        var trackIdx = trackOutputOptions.indexOf(trackOut)
        if (trackIdx < 0) trackIdx = 0
        trackOutputCombo.currentIndex = trackIdx
        root.isRefreshing = false
    }

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    property var trackOutputOptions: ["停止输出", "开始输出"]

    // 模块标题
    Text {
        text: "【满天星配置】"
        color: root.primary
        font.pixelSize: 15
        font.bold: true
        width: parent.width
    }

    // 场景配置
    Column {
        width: parent.width
        spacing: 10
        Text { text: "——场景配置——"; color: root.text2; font.pixelSize: 13 }

        Row {
            spacing: 10
            Text { text: "场景路径"; color: root.text2; font.pixelSize: 13; width: 60; height: 32; verticalAlignment: Text.AlignVCenter }
            TextField {
                id: scenePathField
                width: 300
                height: 32
                color: root.text1
                font.pixelSize: 13
                placeholderText: ""
                text: root.detailData.scenePath || ""
                readOnly: true
                background: Rectangle { color: "#F5F5F5"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
            }
        }
    }

    // 停止
    StopConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 信号设置 (36通道)
    SignalConfig36 {
        width: parent.width
        detailData: root.detailData
        sceneType: root.sceneType
        channelsKey: "channels"
        freqKey: "freqSelect"
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 轨迹输出
    Column {
        width: parent.width
        spacing: 10
        Text { text: "——轨迹输出——"; color: root.text2; font.pixelSize: 13 }

        Row {
            spacing: 10
            Text { text: "轨迹输出"; color: root.text2; font.pixelSize: 13; width: 60; height: 32; verticalAlignment: Text.AlignVCenter }
            ComboBox {
                id: trackOutputCombo
                width: 140
                height: 32
                implicitHeight: 32
                font.pixelSize: 13
                model: root.trackOutputOptions
                currentIndex: 0
                onCurrentTextChanged: {
                    if (root.isRefreshing) return
                    root.updateData("trackOutput", currentText)
                }
                delegate: ItemDelegate {
                    width: trackOutputCombo.width
                    contentItem: Text {
                        text: modelData
                        color: (trackOutputCombo.highlightedIndex === index) ? "white" : root.text1
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 6
                        color: (trackOutputCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                    }
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 28
                    text: trackOutputCombo.displayText
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
                    y: trackOutputCombo.height + 4
                    width: trackOutputCombo.width
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
                        model: trackOutputCombo.delegateModel
                    }
                }
            }
        }
    }

    // 仿真误差参数
    SimulationErrorConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 目标轨迹参数
    TargetTrackConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }
}
