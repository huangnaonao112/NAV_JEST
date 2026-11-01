// 卫星导航差分信号模拟配置.qml
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "configs/common"

Column {
    id: root

    property var detailData: ({})
    property string sceneType: ""

    // 信号：通知父组件数据已更新
    signal updateDetailData(string key, var value)

    // 场景类型判断
    readonly property bool isLowOrbit: root.sceneType === "低轨导航终端性能测试"
    readonly property bool isLunarSpace: root.sceneType === "地月空间终端设备性能测试"
    readonly property bool isSecurityAuth: root.sceneType === "安全认证测试终端试验"
    readonly property bool isAntiJamming: root.sceneType === "导航用户装备抗干扰性能试验"

    // 根据场景类型获取独立的 channels 数据键名，确保不同场景类型的数据严格隔离
    readonly property string channelsKey: {
        if (isLowOrbit) return "lowOrbitChannels"
        if (isLunarSpace) return "lunarSpaceChannels"
        if (isSecurityAuth) return "securityAuthChannels"
        if (isAntiJamming) return "antiJammingChannels"
        return "channels" // 兜底
    }

    // 根据场景类型获取独立的频点数据键名
    readonly property string freqKey: {
        if (isLowOrbit) return "lowOrbitFreq"
        if (isLunarSpace) return "lunarSpaceFreq"
        if (isSecurityAuth) return "securityAuthFreq"
        if (isAntiJamming) return "antiJammingFreq"
        return "globalFreq" // 兜底
    }

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    spacing: 16

    // 模块标题
    Text {
        text: "【卫星导航差分信号模拟配置】"
        color: root.primary
        font.pixelSize: 15
        font.bold: true
    }

    // 场景配置
    ScenePathConfig {
        width: parent.width
        detailData: root.detailData
        dataKey: "scenePath"
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 初始化
    InitConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 启动
    StartConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
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

    // 信号设置 (24通道)
    SignalConfig24 {
        width: parent.width
        detailData: root.detailData
        sceneType: root.sceneType
        channelsKey: root.channelsKey
        freqKey: root.freqKey
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 仿真误差参数 (仅地月空间场景)
    SimulationErrorConfig {
        width: parent.width
        visible: root.isLunarSpace
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 目标轨迹参数 (仅地月空间场景)
    TargetTrackConfig {
        width: parent.width
        visible: root.isLunarSpace
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }
}
