import QtQuick
import QtQuick.Layouts
import NavModels

Rectangle {
    id: root
    width: 300
    height: 200
    color: "#80000000"
    radius: 10
    clip: true

    // Prevent mouse events from passing through to the 3D scene
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        enabled: false
        onWheel: (wheel) => wheel.accepted = true
    }

    // Phase names array - 与 ProbeMissionModel::Phase 枚举对应
    readonly property var phaseNames: ["发射入轨", "近地轨道", "地月转移", "环月轨道", "动力下降", "任务完成"]

    // Format elapsed time as HH:MM:SS
    function formatTime(seconds) {
        var h = Math.floor(seconds / 3600);
        var m = Math.floor((seconds % 3600) / 60);
        var s = Math.floor(seconds % 60);
        return String(h).padStart(2, '0') + ":" + 
               String(m).padStart(2, '0') + ":" + 
               String(s).padStart(2, '0');
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // Header
        Text {
            text: "任务状态"
            color: "white"
            font.bold: true
            font.pixelSize: 18
        }

        // Current Phase
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "当前阶段:"
                color: "#AAAAAA"
                font.pixelSize: 14
                Layout.preferredWidth: 80
            }
            Text {
                text: ProbeMissionModel ? (phaseNames[ProbeMissionModel.currentPhase] || "未知") : "未知"
                color: "#00FF88"
                font.bold: true
                font.pixelSize: 14
            }
        }

        // Altitude
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "高度:"
                color: "#AAAAAA"
                font.pixelSize: 14
                Layout.preferredWidth: 80
            }
            Text {
                text: ProbeMissionModel ? ProbeMissionModel.altitude.toFixed(0) + " km" : "-- km"
                color: "white"
                font.pixelSize: 14
            }
        }

        // Velocity
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "速度:"
                color: "#AAAAAA"
                font.pixelSize: 14
                Layout.preferredWidth: 80
            }
            Text {
                text: ProbeMissionModel ? ProbeMissionModel.velocity.toFixed(2) + " km/s" : "-- km/s"
                color: "white"
                font.pixelSize: 14
            }
        }

        // Mission Time
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "任务时间:"
                color: "#AAAAAA"
                font.pixelSize: 14
                Layout.preferredWidth: 80
            }
            Text {
                text: ProbeMissionModel ? formatTime(ProbeMissionModel.elapsedTime) : "--:--:--"
                color: "white"
                font.pixelSize: 14
            }
        }

        Item { Layout.fillHeight: true }
    }
}