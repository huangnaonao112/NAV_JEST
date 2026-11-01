// Step3_Evaluation.qml - 步骤3：评估配置
import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var detailData: ({})
    property string sceneType: ""

    // 信号：通知父组件数据已更新
    signal updateDetailData(string key, var value)

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 测试类型判断
    readonly property bool isPositionTest: {
        var t = root.detailData ? root.detailData.testType : ""
        return t === "定位精度" || t === "干扰定位" || t === "抗干扰性能测试" || t === "安全认证性能测试" || t === "地月空间定位能力测试"
    }
    readonly property bool isVelocityTest: {
        var t = root.detailData ? root.detailData.testType : ""
        return t === "测速精度" || t === "干扰测速"
    }
    readonly property bool isTimeToFirstFixTest: {
        var t = root.detailData ? root.detailData.testType : ""
        return t === "首次定位时间" || t === "干扰首次定位时间"
    }

    // 组件加载时不输出日志

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Flickable {
            id: flickable
            anchors.fill: parent
            clip: true
            contentHeight: contentColumn.height + 20
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                width: flickable.width - 16
                anchors.left: parent.left
                anchors.leftMargin: 8
                spacing: 16

                // ===== 评估类型 (仅定位和测速测试显示) =====
                Column {
                    width: parent.width
                    spacing: 10
                    visible: !root.isTimeToFirstFixTest

                    Text { text: "——评估类型——"; color: root.text2; font.pixelSize: 13 }

                    Row {
                        width: parent.width
                        spacing: 26

                        ButtonGroup { id: evalMethodGroup }

                        RadioButton {
                            id: evalRmsRadio
                            text: "RMS统计"
                            ButtonGroup.group: evalMethodGroup
                            checked: {
                                var method = root.detailData ? root.detailData.evalMethod : "RMS统计"
                                return method === "RMS统计"
                            }
                            onCheckedChanged: {
                                if (checked) root.updateData("evalMethod", "RMS统计")
                            }
                            indicator: Rectangle {
                                implicitWidth: 16
                                implicitHeight: 16
                                radius: 8
                                border.width: 1
                                border.color: evalRmsRadio.checked ? root.primary : "#BBDEFB"
                                color: "white"
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: root.primary
                                    visible: evalRmsRadio.checked
                                }
                            }
                            contentItem: Text {
                                text: evalRmsRadio.text
                                color: root.text1
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: evalRmsRadio.indicator.width + evalRmsRadio.spacing
                            }
                        }

                        RadioButton {
                            id: evalCpeRadio
                            text: "CPE统计"
                            ButtonGroup.group: evalMethodGroup
                            checked: {
                                var method = root.detailData ? root.detailData.evalMethod : "RMS统计"
                                return method === "CPE统计"
                            }
                            onCheckedChanged: {
                                if (checked) root.updateData("evalMethod", "CPE统计")
                            }
                            indicator: Rectangle {
                                implicitWidth: 16
                                implicitHeight: 16
                                radius: 8
                                border.width: 1
                                border.color: evalCpeRadio.checked ? root.primary : "#BBDEFB"
                                color: "white"
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: root.primary
                                    visible: evalCpeRadio.checked
                                }
                            }
                            contentItem: Text {
                                text: evalCpeRadio.text
                                color: root.text1
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: evalCpeRadio.indicator.width + evalCpeRadio.spacing
                            }
                        }

                        RadioButton {
                            id: eval95Radio
                            text: "95%统计"
                            ButtonGroup.group: evalMethodGroup
                            checked: {
                                var method = root.detailData ? root.detailData.evalMethod : "RMS统计"
                                return method === "95%统计"
                            }
                            onCheckedChanged: {
                                if (checked) root.updateData("evalMethod", "95%统计")
                            }
                            indicator: Rectangle {
                                implicitWidth: 16
                                implicitHeight: 16
                                radius: 8
                                border.width: 1
                                border.color: eval95Radio.checked ? root.primary : "#BBDEFB"
                                color: "white"
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: root.primary
                                    visible: eval95Radio.checked
                                }
                            }
                            contentItem: Text {
                                text: eval95Radio.text
                                color: root.text1
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                leftPadding: eval95Radio.indicator.width + eval95Radio.spacing
                            }
                        }
                    }
                }

                // ===== 阈值配置 (定位测试: cm, 测速测试: m/s) =====
                Column {
                    width: parent.width
                    spacing: 10
                    visible: !root.isTimeToFirstFixTest

                    Text { text: "——阈值配置——"; color: root.text2; font.pixelSize: 13 }

                    Grid {
                        width: parent.width
                        columns: 3
                        columnSpacing: 10
                        rowSpacing: 12

                        // 单位根据测试类型变化
                        property string unit: root.isVelocityTest ? "m/s" : "cm"

                        Text { text: "东方向"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            id: thEastField
                            width: 120
                            height: 32
                            color: root.text1
                            font.pixelSize: 13
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: (root.detailData && root.detailData.thEast !== undefined) ? String(root.detailData.thEast) : "10"
                            background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
                            onTextChanged: { if (activeFocus) root.updateData("thEast", parseFloat(text) || 10) }
                        }
                        Text { text: parent.unit; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }

                        Text { text: "北方向"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            id: thNorthField
                            width: 120
                            height: 32
                            color: root.text1
                            font.pixelSize: 13
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: (root.detailData && root.detailData.thNorth !== undefined) ? String(root.detailData.thNorth) : "10"
                            background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
                            onTextChanged: { if (activeFocus) root.updateData("thNorth", parseFloat(text) || 10) }
                        }
                        Text { text: parent.unit; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }

                        Text { text: "垂直方向"; color: root.text2; font.pixelSize: 13; width: 80; height: 32; verticalAlignment: Text.AlignVCenter }
                        TextField {
                            id: thUpField
                            width: 120
                            height: 32
                            color: root.text1
                            font.pixelSize: 13
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: (root.detailData && root.detailData.thUp !== undefined) ? String(root.detailData.thUp) : "10"
                            background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
                            onTextChanged: { if (activeFocus) root.updateData("thUp", parseFloat(text) || 10) }
                        }
                        Text { text: parent.unit; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    }
                }

                // ===== 时间阈值 (仅首次定位时间测试显示) =====
                Column {
                    width: parent.width
                    spacing: 10
                    visible: root.isTimeToFirstFixTest

                    Text { text: "——时间阈值——"; color: root.text2; font.pixelSize: 13 }

                    Row {
                        spacing: 10
                        Text {
                            text: "时间阈值"
                            color: root.text2
                            font.pixelSize: 13
                            width: 80
                            height: 32
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            id: timeThresholdField
                            width: 120
                            height: 32
                            color: root.text1
                            font.pixelSize: 13
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: (root.detailData && root.detailData.timeThreshold !== undefined) ? String(root.detailData.timeThreshold) : "30"
                            background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
                            onTextChanged: { if (activeFocus) root.updateData("timeThreshold", parseFloat(text) || 30) }
                        }
                        Text { text: "秒"; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    }
                }

                // ===== 数据采集 =====
                Column {
                    width: parent.width
                    spacing: 10

                    Text { text: "——数据采集——"; color: root.text2; font.pixelSize: 13 }

                    Row {
                        spacing: 10
                        Text {
                            text: "采集数量"
                            color: root.text2
                            font.pixelSize: 13
                            width: 80
                            height: 32
                            verticalAlignment: Text.AlignVCenter
                        }
                        TextField {
                            id: dataCollectionCountField
                            width: 120
                            height: 32
                            color: root.text1
                            font.pixelSize: 13
                            inputMethodHints: Qt.ImhDigitsOnly
                            text: (root.detailData && root.detailData.dataCollectionCount !== undefined) ? String(root.detailData.dataCollectionCount) : "100"
                            background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
                            onTextChanged: { if (activeFocus) root.updateData("dataCollectionCount", parseInt(text) || 100) }
                        }
                        Text { text: "个"; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    }
                }
            }
        }
    }

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }
}
