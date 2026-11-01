import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NavModels

Rectangle {
    id: root
    width: 240
    height: mainLayout.implicitHeight + 24 // 24 is top + bottom margins
    color: "#CC222222"
    radius: 10
    border.color: "#00AAFF"
    border.width: 1
    clip: true

    // Prevent mouse events from passing through to the 3D scene
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        enabled: false
        onWheel: (wheel) => wheel.accepted = true
    }

    property bool showLEO: false
    property bool showMEOHEO: false
    property bool showGEO: false

    // 欺骗卫星独立开关
    property bool showMEODeception: false
    property bool showGEODeception: false

    // 干扰信号开关状态
    property bool signalMEO: false
    property bool signalGEO: false

    // 使用批量设置，只触发一次 orbitPathsChanged
    function applyGroupVisibility() {
        if (!SatelliteModel) return;
        var total = SatelliteModel.satelliteCount

        var indices = []
        var visibles = []

        for (var idx = 0; idx < total; ++idx) {
            var v = false;
            if (idx < 5) v = root.showLEO;              // LEO: 0-4
            else if (idx < 10) v = root.showMEOHEO;     // MEO: 5-9
            else if (idx < 15) v = root.showGEO;        // GEO: 10-14
            else if (idx === 15) v = root.showMEODeception;  // 中轨欺骗: 15
            else if (idx === 16) v = root.showGEODeception;  // 高轨欺骗: 16

            indices.push(idx)
            visibles.push(v)
        }

        // 批量设置，只触发一次信号
        SatelliteModel.setVisibilityBatch(indices, visibles)
    }

    Component.onCompleted: applyGroupVisibility()
    Connections {
        target: SatelliteModel
        function onSatelliteCountChanged() { root.applyGroupVisibility() }
    }

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: "轨道显示"
                color: "white"
                font.bold: true
                font.pixelSize: 18
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "×"
                flat: true
                onClicked: root.visible = false
                font.pixelSize: 20
                palette.buttonText: "white"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text { text: "低轨"; color: "white"; font.pixelSize: 14; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.showLEO
                onToggled: {
                    root.showLEO = checked
                    root.applyGroupVisibility()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: "中高轨"; color: "white"; font.pixelSize: 14; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.showMEOHEO
                onToggled: {
                    root.showMEOHEO = checked
                    root.applyGroupVisibility()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: "高轨"; color: "white"; font.pixelSize: 14; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.showGEO
                onToggled: {
                    root.showGEO = checked
                    root.applyGroupVisibility()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: "中轨欺骗"; color: "#FF3333"; font.pixelSize: 14; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.showMEODeception
                onToggled: {
                    root.showMEODeception = checked
                    root.applyGroupVisibility()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: "高轨欺骗"; color: "#FF3333"; font.pixelSize: 14; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.showGEODeception
                onToggled: {
                    root.showGEODeception = checked
                    root.applyGroupVisibility()
                }
            }
        }

        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#555577"
        }

        Text {
            text: "干扰信号"
            color: "#FF6666"
            font.bold: true
            font.pixelSize: 14
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: "中轨干扰"; color: "#FF9999"; font.pixelSize: 13; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.signalMEO
                onToggled: {
                    root.signalMEO = checked
                    if (SatelliteModel) {
                        SatelliteModel.setSignalActive(15, checked)  // 中轨欺骗卫星索引15
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text { text: "高轨干扰"; color: "#FF9999"; font.pixelSize: 13; Layout.preferredWidth: 70 }
            Item { Layout.fillWidth: true }
            Switch {
                checked: root.signalGEO
                onToggled: {
                    root.signalGEO = checked
                    if (SatelliteModel) {
                        SatelliteModel.setSignalActive(16, checked)  // 高轨欺骗卫星索引16
                    }
                }
            }
        }
    }
}
