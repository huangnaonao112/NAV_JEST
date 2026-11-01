// Step1_SimulatorConfig.qml - 步骤 1：模拟器配置
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Rectangle {
    id: root
    color: "white"

    property var detailData: ({})

    // 场景类型属性 - 用于判断场景类型
    property string sceneType: ""

    // 信号：通知父组件数据已更新
    signal updateDetailData(string key, var value)

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 判断是否为中高轨场景
    property bool isMediumOrbit: root.sceneType === "中高轨导航终端性能测试"

    // 判断是否为抗干扰场景
    property bool isAntiJamming: root.sceneType === "导航用户装备抗干扰性能试验"

    // 判断是否为安全认证场景
    property bool isSecurityAuth: root.sceneType === "安全认证测试终端试验"

    // 默认场景（低轨、地月空间等）显示通用配置
    property bool isDefaultScene: !root.isMediumOrbit && !root.isAntiJamming && !root.isSecurityAuth

    // 组件加载和属性变化时不输出日志

    Flickable {
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: contentColumn.implicitHeight + 20
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            width: 8
            policy: ScrollBar.AsNeeded
        }

        Column {
            id: contentColumn
            width: parent.width - 16
            x: 8
            spacing: 16

            // 卫星导航差分信号模拟配置 - 中高轨场景不显示
            SatelliteNavDifferentialConfig {
                width: parent.width
                visible: !root.isMediumOrbit
                detailData: root.detailData
                sceneType: root.sceneType
                onUpdateDetailData: function(key, value) {
                    root.updateDetailData(key, value)
                }
            }

            // 安全认证转发源配置 - 仅安全认证场景显示
            SecurityAuthForwardConfig {
                width: parent.width
                visible: root.isSecurityAuth
                detailData: root.detailData
                onUpdateDetailData: function(key, value) {
                    root.updateDetailData(key, value)
                }
            }

            // 压制干扰源配置 - 仅抗干扰场景显示
            JammingSourceConfig {
                width: parent.width
                visible: root.isAntiJamming
                detailData: root.detailData
                onUpdateDetailData: function(key, value) {
                    root.updateDetailData(key, value)
                }
            }

            // 诱骗干扰源配置 - 仅抗干扰场景显示
            DeceptionJammingSourceConfig {
                width: parent.width
                visible: root.isAntiJamming
                detailData: root.detailData
                onUpdateDetailData: function(key, value) {
                    root.updateDetailData(key, value)
                }
            }

            // 满天星配置 - 仅中高轨场景显示
            MediumOrbitConfig {
                width: parent.width
                visible: root.isMediumOrbit
                detailData: root.detailData
                sceneType: root.sceneType
                onUpdateDetailData: function(key, value) {
                    root.updateDetailData(key, value)
                }
            }
        }
    }
}