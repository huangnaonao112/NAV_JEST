// ScenePathConfig.qml - 场景路径配置
import QtQuick
import QtQuick.Controls
import NavModels

Column {
    id: root

    property var detailData: ({})
    property string dataKey: "scenePath"  // 数据键名，可覆盖

    signal updateDetailData(string key, var value)

    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color primary: "#1677FF"

    spacing: 10

    Text { text: "——场景配置——"; color: root.text2; font.pixelSize: 13 }

    Row {
        spacing: 10
        Text {
            text: "场景路径"
            color: root.text2
            font.pixelSize: 13
            width: 60
            height: 32
            verticalAlignment: Text.AlignVCenter
        }
        TextField {
            id: scenePathField
            width: 300
            height: 32
            color: root.text1
            font.pixelSize: 13
            placeholderText: ""
            text: (root.detailData && root.detailData[root.dataKey]) ? root.detailData[root.dataKey] : ""
            readOnly: true
            background: Rectangle {
                color: "#F5F5F5"
                radius: 6
                border.width: 1
                border.color: "#BBDEFB"
            }
        }
    }

    // ===== 内置航路生成（navcore：切线圆弧转弯 + 运动约束速度剖面） =====
    Column {
        width: parent.width
        spacing: 10

        Text { text: "——内置航路生成 (navcore)——"; color: root.text2; font.pixelSize: 13 }

        Row {
            spacing: 10

            Text { text: "转弯半径"; color: root.text2; font.pixelSize: 13; width: 70; height: 32; verticalAlignment: Text.AlignVCenter }
            TextField {
                id: turnRadiusField
                width: 90
                height: 32
                color: root.text1
                font.pixelSize: 13
                text: "2500"
                inputMethodHints: Qt.ImhDigitsOnly
                background: Rectangle { color: "white"; radius: 6; border.width: 1; border.color: "#BBDEFB" }
            }
            Text { text: "m"; color: root.text2; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }

            Button {
                id: generateButton
                text: "生成演示航路"
                onClicked: {
                    var r = RouteGeneratorModel.generateDemoRoute(
                        39.9042, 116.4074, 50.0,
                        parseFloat(turnRadiusField.text) || 2500)
                    if (r.valid) {
                        root.updateDetailData(root.dataKey,
                            "navcore-route://demo?pts=" + r.pointCount +
                            "&len=" + Math.round(r.lengthMeters) +
                            "&dur=" + Math.round(r.durationSeconds))
                    }
                }
            }
        }

        Grid {
            columns: 3
            columnSpacing: 20
            rowSpacing: 6
            visible: RouteGeneratorModel.lastStats.valid === true

            Repeater {
                model: [
                    { label: "采样点数", value: RouteGeneratorModel.lastStats.pointCount },
                    { label: "航路总长", value: (RouteGeneratorModel.lastStats.lengthMeters / 1000).toFixed(1) + " km" },
                    { label: "航路时长", value: (RouteGeneratorModel.lastStats.durationSeconds / 60).toFixed(1) + " min" },
                    { label: "圆弧转弯数", value: RouteGeneratorModel.lastStats.roundedTurnCount },
                    { label: "受限转弯数", value: RouteGeneratorModel.lastStats.radiusLimitedTurnCount },
                    { label: "最小有效半径", value: RouteGeneratorModel.lastStats.minimumEffectiveTurnRadiusMeters.toFixed(0) + " m" }
                ]
                delegate: Column {
                    spacing: 1
                    Text { text: modelData.label; color: "#9CA3AF"; font.pixelSize: 11 }
                    Text { text: modelData.value; color: root.text1; font.pixelSize: 13; font.bold: true }
                }
            }
        }

        Text {
            width: parent.width
            text: "演示航路：北京原点 7 航路点环形航线，圆弧转弯 + ACC/DEC/LAT 速度剖面"
            color: root.text2
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            visible: RouteGeneratorModel.lastStats.valid !== true
        }
    }
}
