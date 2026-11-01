// StepX_JammingSource.qml - 压制干扰源模块
// 用于导航用户装备抗干扰性能试验场景类型
import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var detailData: ({})
    property bool isRefreshing: false

    signal updateDetailData(string key, var value)

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 监听 detailData 变化，设置刷新标志
    onDetailDataChanged: {
        root.isRefreshing = true
        refreshTimer.start()
    }

    Timer {
        id: refreshTimer
        interval: 50
        onTriggered: root.isRefreshing = false
    }

    Flickable {
        anchors.fill: parent
        clip: true
        contentHeight: contentColumn.height
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            width: 8
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 6
                radius: 3
                color: "#888888"
            }
        }

        Column {
            id: contentColumn
            width: parent.width
            spacing: 20

            // ========== 连续波设置 ==========
            JammingSignalGroup {
                title: "——连续波设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "中心频率", key: "centerFreq", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" }
                ]
                signalData: root.detailData.continuousWave || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("continuousWave", channelIndex, key, value)
                }
            }

            // ========== 脉冲信号设置 ==========
            JammingSignalGroup {
                title: "——脉冲信号设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "中心频率", key: "centerFreq", placeholder: "-127.6" },
                    { label: "周期", key: "period", placeholder: "-127.6" },
                    { label: "脉宽", key: "pulseWidth", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" }
                ]
                signalData: root.detailData.pulseSignal || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("pulseSignal", channelIndex, key, value)
                }
            }

            // ========== 白噪声设置 ==========
            JammingSignalGroup {
                title: "——白噪声设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "中心频率", key: "centerFreq", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" },
                    { label: "信号带宽", key: "signalBandwidth", placeholder: "-127.6" }
                ]
                signalData: root.detailData.whiteNoise || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("whiteNoise", channelIndex, key, value)
                }
            }

            // ========== 调幅信号设置 ==========
            JammingSignalGroup {
                title: "——调幅信号设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "中心频率", key: "centerFreq", placeholder: "-127.6" },
                    { label: "信号类型", key: "signalType", placeholder: "正弦波" },
                    { label: "调制频率", key: "modulationFreq", placeholder: "-127.6" },
                    { label: "调制度", key: "modulationDegree", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" }
                ]
                signalData: root.detailData.amSignal || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("amSignal", channelIndex, key, value)
                }
            }

            // ========== 扩频信号设置 ==========
            JammingSignalGroup {
                title: "——扩频信号设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "中心频率", key: "centerFreq", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" },
                    { label: "信号带宽", key: "signalBandwidth", placeholder: "-127.6" }
                ]
                signalData: root.detailData.spreadSpectrum || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("spreadSpectrum", channelIndex, key, value)
                }
            }

            // ========== 扫频信号设置 ==========
            JammingSignalGroup {
                title: "——扫频信号设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "起始频率", key: "startFreq", placeholder: "-127.6" },
                    { label: "终止频率", key: "endFreq", placeholder: "-127.6" },
                    { label: "频率步进", key: "freqStep", placeholder: "-127.6" },
                    { label: "驻留时间", key: "dwellTime", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" }
                ]
                signalData: root.detailData.sweepFreq || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("sweepFreq", channelIndex, key, value)
                }
            }

            // ========== 调频信号设置 ==========
            JammingSignalGroup {
                title: "——调频信号设置——"
                isRefreshing: root.isRefreshing
                detailFields: [
                    { label: "中心频率", key: "centerFreq", placeholder: "-127.6" },
                    { label: "视频噪声带宽", key: "videoNoiseBandwidth", placeholder: "1K" },
                    { label: "噪声带宽", key: "noiseBandwidth", placeholder: "-127.6" },
                    { label: "信号功率", key: "signalPower", placeholder: "-127.6" }
                ]
                signalData: root.detailData.fmSignal || {}
                onChannelDataChanged: function(channelIndex, key, value) {
                    root.updateJammingData("fmSignal", channelIndex, key, value)
                }
            }
        }
    }

    // 更新压制干扰源数据 - 使用数组格式 [{...}, {...}, {...}]
    function updateJammingData(signalType, channelIndex, key, value) {
        if (!root.detailData || root.isRefreshing) return

        var existingData = root.detailData[signalType]
        var signalGroup = []

        if (existingData) {
            // 直接遍历复制，不依赖JSON转换
            if (existingData.length !== undefined && existingData.length > 0) {
                for (var i = 0; i < existingData.length; i++) {
                    if (existingData[i]) {
                        signalGroup.push(JSON.parse(JSON.stringify(existingData[i])))
                    } else {
                        signalGroup.push({ paramType: "停止输出" })
                    }
                }
            }
        }

        // 确保有3个通道
        while (signalGroup.length < 3) {
            signalGroup.push({ paramType: "停止输出" })
        }

        if (!signalGroup[channelIndex]) {
            signalGroup[channelIndex] = { paramType: "停止输出" }
        }
        signalGroup[channelIndex][key] = value
        root.updateDetailData(signalType, signalGroup)
    }
}
