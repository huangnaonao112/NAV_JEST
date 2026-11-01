// DeleteConfirmDialog.qml - 删除确认弹窗
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    parent: Overlay.overlay
    width: 360
    height: 180
    
    signal confirmed()
    
    readonly property color danger: "#EF4444"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"

    onOpened: {
        x = Math.round((parent.width - width) / 2)
        y = Math.round((parent.height - height) / 2)
    }

    background: Rectangle {
        color: "#E8F0FE"
        radius: 14
        border.width: 1
        border.color: "#BBDEFB"
    }

    contentItem: Column {
        width: parent.width
        spacing: 0

        // 标题栏
        Rectangle {
            width: parent.width
            height: 48
            color: root.danger
            radius: 14
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 14
                color: root.danger
            }
            Text {
                anchors.centerIn: parent
                text: "\u786E\u8BA4\u5220\u9664"
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }
        }

        // 内容
        Column {
            width: parent.width
            leftPadding: 20
            rightPadding: 20
            topPadding: 20
            bottomPadding: 20
            spacing: 16

            Text {
                width: parent.width - 40
                text: "\u786E\u5B9A\u8981\u5220\u9664\u5F53\u524D\u9009\u4E2D\u7684\u573A\u666F\u5417\uFF1F\u6B64\u64CD\u4F5C\u4E0D\u53EF\u64A4\u9500\u3002"
                color: root.text1
                wrapMode: Text.WordWrap
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }

            RowLayout {
                width: parent.width - 40
                spacing: 12

                Button {
                    text: "\u53D6\u6D88"
                    flat: true
                    implicitWidth: 72
                    implicitHeight: 32
                    background: Rectangle {
                        color: parent.pressed ? "#E5E7EB" : (parent.hovered ? "#F6F7F9" : "white")
                        radius: 6
                        border.width: 1
                        border.color: "#E5E7EB"
                    }
                    contentItem: Text {
                        text: parent.text
                        color: root.text2
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }
                    onClicked: root.close()
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "\u5220\u9664"
                    implicitWidth: 72
                    implicitHeight: 32
                    background: Rectangle {
                        color: parent.pressed ? "#B91C1C" : (parent.hovered ? "#DC2626" : root.danger)
                        radius: 6
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }
                    onClicked: {
                        root.confirmed()
                        root.close()
                    }
                }
            }
        }
    }
}