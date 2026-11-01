// DeceptionPositionConfig.qml - 诱骗位置设置配置
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

    Text { text: "——诱骗位置——"; color: root.text2; font.pixelSize: 13 }

    Grid {
        width: parent.width
        columns: 2
        columnSpacing: 10
        rowSpacing: 8

        Text { text: "经度"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
        TextField {
            width: 140; height: 28
            color: root.text1
            font.pixelSize: 12
            text: (root.detailData && root.detailData.deceptionLon !== undefined) ? String(root.detailData.deceptionLon) : "-127.6"
            validator: DoubleValidator { bottom: -180; top: 180; decimals: 1 }
            background: Rectangle { color: "#F9FAFB"; radius: 6; border.width: 1; border.color: "#D1D5DB" }
            onEditingFinished: root.updateData("deceptionLon", parseFloat(text) || -127.6)
        }

        Text { text: "纬度"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
        TextField {
            width: 140; height: 28
            color: root.text1
            font.pixelSize: 12
            text: (root.detailData && root.detailData.deceptionLat !== undefined) ? String(root.detailData.deceptionLat) : "-127.6"
            validator: DoubleValidator { bottom: -90; top: 90; decimals: 1 }
            background: Rectangle { color: "#F9FAFB"; radius: 6; border.width: 1; border.color: "#D1D5DB" }
            onEditingFinished: root.updateData("deceptionLat", parseFloat(text) || -127.6)
        }

        Text { text: "高程"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
        TextField {
            width: 140; height: 28
            color: root.text1
            font.pixelSize: 12
            text: (root.detailData && root.detailData.deceptionAlt !== undefined) ? String(root.detailData.deceptionAlt) : "-127.6"
            validator: DoubleValidator { bottom: -10000; top: 100000; decimals: 1 }
            background: Rectangle { color: "#F9FAFB"; radius: 6; border.width: 1; border.color: "#D1D5DB" }
            onEditingFinished: root.updateData("deceptionAlt", parseFloat(text) || -127.6)
        }
    }
}
