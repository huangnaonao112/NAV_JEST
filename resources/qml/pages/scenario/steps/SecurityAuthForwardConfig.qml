// SecurityAuthForwardConfig.qml - 安全认证转发源配置
import QtQuick
import QtQuick.Controls
import "configs/security"

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
        text: "【安全认证转发源配置】"
        color: root.primary
        font.pixelSize: 15
        font.bold: true
        width: parent.width
    }

    // 欺骗参数
    SecurityDeceptionParamsConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }

    // 轨迹配置
    TrackConfig {
        width: parent.width
        detailData: root.detailData
        onUpdateDetailData: function(key, value) {
            root.updateDetailData(key, value)
        }
    }
}
