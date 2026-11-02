// OrbitInsightPanel - navcore 真实轨道力学实测面板。
// 显示选中卫星的真实星下点（GMST/ECI-ECEF/WGS-84）、轨道参数
// 与地面站（北京）观测（方位/仰角/斜距/可见性），500ms 实时刷新。
// 数据来自 SatelliteModel 的 navcore 接口，与视觉传播管线无关。
import QtQuick
import QtQuick.Controls
import NavModels

Rectangle {
    id: root

    width: 250
    height: insightColumn.height + 20
    radius: 8
    color: "#CC1A1A2E"
    border.color: "#66ffffff"
    border.width: 1

    property int satelliteIndex: 0
    property var stateData: null
    property var obsData: null

    function refresh() {
        var t = SatelliteModel.currentSimTime()
        root.stateData = SatelliteModel.navcoreState(root.satelliteIndex, t)
        root.obsData = SatelliteModel.navcoreObservation(
            root.satelliteIndex, 39.9042, 116.4074, 50.0, t)
    }

    Timer {
        interval: 500
        repeat: true
        running: root.visible
        triggeredOnStart: true
        onTriggered: root.refresh()
    }

    onVisibleChanged: if (visible) root.refresh()
    onSatelliteIndexChanged: if (root.visible) root.refresh()

    Column {
        id: insightColumn
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 6

        Text {
            width: parent.width
            text: "轨道实测 (navcore)"
            color: "white"
            font.pixelSize: 13
            font.bold: true
        }

        ComboBox {
            id: satSelector
            width: parent.width
            height: 26
            font.pixelSize: 12
            model: SatelliteModel.satelliteCount
            currentIndex: 0
            onCurrentIndexChanged: {
                if (currentIndex >= 0) root.satelliteIndex = currentIndex
            }
            delegate: ItemDelegate {
                width: parent.width
                text: "卫星 " + modelData
                font.pixelSize: 12
            }
        }

        Grid {
            columns: 2
            columnSpacing: 14
            rowSpacing: 5
            width: parent.width

            Repeater {
                model: {
                    var s = root.stateData
                    var o = root.obsData
                    if (!s || !s.valid || !o || !o.valid) return []
                    return [
                        { label: "真实星下点纬度", value: s.subLatitudeDeg.toFixed(2) + "°" },
                        { label: "真实星下点经度", value: s.subLongitudeDeg.toFixed(2) + "°" },
                        { label: "轨道高度", value: s.altitudeKm.toFixed(1) + " km" },
                        { label: "轨道速度", value: s.speedKmPerSecond.toFixed(3) + " km/s" },
                        { label: "轨道周期", value: s.periodMinutes.toFixed(1) + " min" },
                        { label: "格林尼治恒星时角", value: s.gmstDeg.toFixed(2) + "°" },
                        { label: "地面站方位角", value: o.azimuthDeg.toFixed(1) + "°" },
                        { label: "地面站仰角", value: o.elevationDeg.toFixed(1) + "°" },
                        { label: "地面站斜距", value: o.rangeKm.toFixed(0) + " km" },
                        { label: "站内可见 (仰角>0°)", value: o.visible ? "是" : "否" }
                    ]
                }
                delegate: Column {
                    spacing: 1
                    Text { text: modelData.label; color: "#99BBDD"; font.pixelSize: 11 }
                    Text { text: modelData.value; color: "white"; font.pixelSize: 13; font.bold: true }
                }
            }
        }

        Text {
            width: parent.width
            text: (root.stateData && root.stateData.valid)
                      ? "地面站: 北京 39.90°N 116.41°E"
                      : "无有效椭圆轨道（星下点不可用）"
            color: (root.stateData && root.stateData.valid) ? "#99BBDD" : "#FF9999"
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }
}
