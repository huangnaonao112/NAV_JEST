// DeceptionRebootConfig.qml - 诱骗重启设置配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——诱骗重启——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text { text: "重启参数"; color: root.text2; font.pixelSize: 12; width: 80; height: 28; verticalAlignment: Text.AlignVCenter }
        TextField {
            width: 200; height: 28
            color: root.text1
            font.pixelSize: 12
            text: root.detailData && root.detailData.deceptionReboot ? String(root.detailData.deceptionReboot) : "REBOOT"
            maximumLength: 50
            background: Rectangle { color: "#F9FAFB"; radius: 6; border.width: 1; border.color: "#D1D5DB" }
            onEditingFinished: root.updateData("deceptionReboot", text)
        }
    }
}
