// TargetTrackConfig.qml - 目标轨迹参数配置
import QtQuick
import QtQuick.Controls

Column {
    id: root

    property var detailData: ({})
    signal updateDetailData(string key, var value)

    property string primary: "#1677FF"
    property string text1: "#1F2937"
    property string text2: "#6B7280"

    property var trackTypeList: ["静态轨迹", "动态轨迹"]
    property var dynamicTrackTypeList: ["初始状态", "匀速直线", "匀加速直线", "拐弯", "俯冲", "制动"]

    spacing: 10

    function updateData(key, value) {
        root.updateDetailData(key, value)
    }

    Text { text: "——目标轨迹参数——"; color: root.text2; font.pixelSize: 13; font.bold: true }

    // 轨迹类型
    Row {
        spacing: 10
        Text { text: "轨迹类型"; color: root.text2; font.pixelSize: 13; width: 100; height: 32; verticalAlignment: Text.AlignVCenter }
        ComboBox {
            id: trackTypeCombo
            width: 140
            height: 32
            implicitHeight: 32
            font.pixelSize: 13
            model: root.trackTypeList
            currentIndex: {
                if (!root.detailData || !root.detailData.trackType) return 0
                return root.detailData.trackType === "动态轨迹" ? 1 : 0
            }
            property bool __initialized: false
            Component.onCompleted: __initialized = true
            onCurrentTextChanged: {
                if (__initialized && root.detailData) {
                    root.updateData("trackType", currentText)
                }
            }
            delegate: ItemDelegate {
                width: trackTypeCombo.width
                contentItem: Text {
                    text: modelData
                    color: (trackTypeCombo.highlightedIndex === index) ? "white" : root.text1
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: (trackTypeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                }
            }
            contentItem: Text {
                leftPadding: 10
                rightPadding: 28
                text: trackTypeCombo.displayText
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
                y: trackTypeCombo.height + 4
                width: trackTypeCombo.width
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
                    model: trackTypeCombo.delegateModel
                }
            }
        }
    }

    // 动态轨迹子类型
    Rectangle {
        width: parent.width
        height: dynamicTrackTypeLayout.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹"

        Column {
            id: dynamicTrackTypeLayout
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "动态轨迹子类型"; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
            ComboBox {
                id: dynamicTrackTypeCombo
                width: 140
                height: 32
                implicitHeight: 32
                font.pixelSize: 13
                model: root.dynamicTrackTypeList
                currentIndex: {
                    if (!root.detailData || !root.detailData.dynamicTrackType) return 0
                    var t = root.detailData.dynamicTrackType
                    var i = root.dynamicTrackTypeList.indexOf(t)
                    return i >= 0 ? i : 0
                }
                property bool __initialized: false
                Component.onCompleted: __initialized = true
                onCurrentTextChanged: {
                    if (__initialized && root.detailData) {
                        root.updateData("dynamicTrackType", currentText)
                    }
                }
                delegate: ItemDelegate {
                    width: dynamicTrackTypeCombo.width
                    contentItem: Text {
                        text: modelData
                        color: (dynamicTrackTypeCombo.highlightedIndex === index) ? "white" : root.text1
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        radius: 6
                        color: (dynamicTrackTypeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                    }
                }
                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 28
                    text: dynamicTrackTypeCombo.displayText
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
                    y: dynamicTrackTypeCombo.height + 4
                    width: dynamicTrackTypeCombo.width
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
                        model: dynamicTrackTypeCombo.delegateModel
                    }
                }
            }
        }
    }

    // 静态轨迹参数
    Rectangle {
        width: parent.width
        height: staticTrackParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "静态轨迹"

        Column {
            id: staticTrackParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——静态位置——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "经度 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.longitude !== undefined) ? String(root.detailData.longitude) : "116.38456"
                    validator: DoubleValidator { bottom: -180; top: 180; decimals: 5 }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                    onEditingFinished: root.updateData("longitude", parseFloat(text) || 0.0)
                }
                Text { text: "纬度 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.latitude !== undefined) ? String(root.detailData.latitude) : "39.69394"
                    validator: DoubleValidator { bottom: -90; top: 90; decimals: 5 }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                    onEditingFinished: root.updateData("latitude", parseFloat(text) || 0.0)
                }
                Text { text: "高度 (m)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.altitude !== undefined) ? String(root.detailData.altitude) : "50.00"
                    validator: DoubleValidator { bottom: -1000; top: 100000; decimals: 2 }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                    onEditingFinished: root.updateData("altitude", parseFloat(text) || 0.0)
                }
            }
        }
    }

    // 初始状态参数
    Rectangle {
        width: parent.width
        height: initialStateParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹" && dynamicTrackTypeCombo.currentText === "初始状态"

        Column {
            id: initialStateParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——初始状态参数——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "经度 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.initialLon !== undefined) ? String(root.detailData.initialLon) : "116.407400"
                    onTextChanged: { if (activeFocus) root.updateData("initialLon", parseFloat(text) || 116.407400) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "纬度 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.initialLat !== undefined) ? String(root.detailData.initialLat) : "39.904200"
                    onTextChanged: { if (activeFocus) root.updateData("initialLat", parseFloat(text) || 39.904200) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "高程 (m)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.initialAlt !== undefined) ? String(root.detailData.initialAlt) : "50.00"
                    onTextChanged: { if (activeFocus) root.updateData("initialAlt", parseFloat(text) || 50.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "速度 (m/s)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.initialSpeed !== undefined) ? String(root.detailData.initialSpeed) : "0.00"
                    onTextChanged: { if (activeFocus) root.updateData("initialSpeed", parseFloat(text) || 0.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "初始方位角 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.initialAzimuth !== undefined) ? String(root.detailData.initialAzimuth) : "0.00"
                    onTextChanged: { if (activeFocus) root.updateData("initialAzimuth", parseFloat(text) || 0.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
            }
        }
    }

    // 匀速直线运动参数
    Rectangle {
        width: parent.width
        height: uniformLinearParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹" && dynamicTrackTypeCombo.currentText === "匀速直线"

        Column {
            id: uniformLinearParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——匀速直线运动——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "运动时长 (s)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.uniformDuration !== undefined) ? String(root.detailData.uniformDuration) : "10.00"
                    onTextChanged: { if (activeFocus) root.updateData("uniformDuration", parseFloat(text) || 10.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
            }
        }
    }

    // 匀加速直线运动参数
    Rectangle {
        width: parent.width
        height: acceleratedLinearParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹" && dynamicTrackTypeCombo.currentText === "匀加速直线"

        Column {
            id: acceleratedLinearParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——匀加速直线运动——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "运动时长 (s)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.accelDuration !== undefined) ? String(root.detailData.accelDuration) : "10.00"
                    onTextChanged: { if (activeFocus) root.updateData("accelDuration", parseFloat(text) || 10.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "匀加速末速度 (m/s)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.accelEndSpeed !== undefined) ? String(root.detailData.accelEndSpeed) : "50.00"
                    onTextChanged: { if (activeFocus) root.updateData("accelEndSpeed", parseFloat(text) || 50.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
            }
        }
    }

    // 拐弯参数
    Rectangle {
        width: parent.width
        height: turningParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹" && dynamicTrackTypeCombo.currentText === "拐弯"

        Column {
            id: turningParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——拐弯参数——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "方向改变量 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.turnDirectionChange !== undefined) ? String(root.detailData.turnDirectionChange) : "90.00"
                    onTextChanged: { if (activeFocus) root.updateData("turnDirectionChange", parseFloat(text) || 90.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "转弯角速度 (°/s)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.turnAngularSpeed !== undefined) ? String(root.detailData.turnAngularSpeed) : "10.00"
                    onTextChanged: { if (activeFocus) root.updateData("turnAngularSpeed", parseFloat(text) || 10.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
            }
        }
    }

    // 俯冲参数
    Rectangle {
        width: parent.width
        height: divingParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹" && dynamicTrackTypeCombo.currentText === "俯冲"

        Column {
            id: divingParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——俯冲参数——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "俯仰角 (°)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.divePitchAngle !== undefined) ? String(root.detailData.divePitchAngle) : "-30.00"
                    onTextChanged: { if (activeFocus) root.updateData("divePitchAngle", parseFloat(text) || -30.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
                Text { text: "高度改变量 (m)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.diveAltChange !== undefined) ? String(root.detailData.diveAltChange) : "-100.00"
                    onTextChanged: { if (activeFocus) root.updateData("diveAltChange", parseFloat(text) || -100.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
            }
        }
    }

    // 制动参数
    Rectangle {
        width: parent.width
        height: brakingParams.implicitHeight + 20
        color: "white"
        radius: 6
        border.width: 1
        border.color: "#BBDEFB"
        visible: trackTypeCombo.currentText === "动态轨迹" && dynamicTrackTypeCombo.currentText === "制动"

        Column {
            id: brakingParams
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Text { text: "——制动参数——"; color: root.text2; font.pixelSize: 12 }

            Grid {
                width: parent.width
                columns: 2
                columnSpacing: 10
                rowSpacing: 8

                Text { text: "制动时长 (s)"; color: root.text2; font.pixelSize: 12; height: 28; verticalAlignment: Text.AlignVCenter }
                TextField {
                    width: 120; height: 28; color: root.text1; font.pixelSize: 12
                    text: (root.detailData && root.detailData.brakeDuration !== undefined) ? String(root.detailData.brakeDuration) : "5.00"
                    onTextChanged: { if (activeFocus) root.updateData("brakeDuration", parseFloat(text) || 5.0) }
                    background: Rectangle { color: "white"; radius: 4; border.width: 1; border.color: "#BBDEFB" }
                }
            }
        }
    }
}
