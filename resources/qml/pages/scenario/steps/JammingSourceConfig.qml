// JammingSourceConfig.qml - 压制干扰源配置
import QtQuick
import QtQuick.Controls
import "configs/antijamming"

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    spacing: 16

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    // 模块标题
    Text {
        text: "【压制干扰源配置】"
        color: root.primary
        font.pixelSize: 15
        font.bold: true
        width: parent.width
    }

    // 连续波设置
    ContinuousWaveConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 脉冲信号设置
    PulseSignalConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 白噪声设置
    WhiteNoiseConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 调幅信号设置
    AmSignalConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 扩频信号设置
    SpreadSpectrumConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 扫频信号设置
    SweepFreqConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 调频信号设置
    FmSignalConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 干扰机数量
    JammerCountConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 干扰参数
    JammingParamsConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }
}
