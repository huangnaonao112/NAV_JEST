// SceneViewItem 的交互覆盖层：视角切换、回放控制、轨道调试入口。
// 通过 scene 属性回访宿主 SceneViewItem 的状态与方法（只在此处耦合）。
import QtQuick
import QtQuick.Controls
import NavModels

Item {
    id: overlayRoot

    required property var scene

    Timer {
        id: drawTimer
        interval: scene.drawIntervalMs
        repeat: true
        running: false
        onTriggered: {
            if (!scene.useTrajectoryFile) {
                stop()
                return
            }
            var total = scene.controlPointX.length
            if (total <= 1) {
                stop()
                return
            }
            if (scene.drawProgress < total) {
                scene.drawProgress = Math.min(total, scene.drawProgress + scene.drawStep)
            } else {
                if (PntDataModel) PntDataModel.setTaskStatusInvokable("任务结束")
                stop()
            }
        }
    }

    // 4. Interactive UI Components
    
    // 左侧视角切换按钮
    Rectangle {
        id: viewSwitchPanel
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 15
        width: 50
        height: 120
        radius: 10
        color: "#CC1A1A2E"
        border.color: "#66ffffff"
        border.width: 1
        
        Column {
            anchors.centerIn: parent
            spacing: 10
            
            // 地球视角按钮
            Rectangle {
                width: 36
                height: 36
                radius: 18
                color: scene.viewMode === "earth" ? "#3B82F6" : "#2A2A4A"
                border.color: scene.viewMode === "earth" ? "#60A5FA" : "#555577"
                border.width: 2
                
                Text {
                    anchors.centerIn: parent
                    text: "地"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: scene.switchToEarthView()
                }
            }
            
            // 分隔线
            Rectangle {
                width: 30
                height: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#555577"
            }
            
            // 月球视角按钮
            Rectangle {
                width: 36
                height: 36
                radius: 18
                color: scene.viewMode === "moon" ? "#F59E0B" : "#2A2A4A"
                border.color: scene.viewMode === "moon" ? "#FBBF24" : "#555577"
                border.width: 2
                
                Text {
                    anchors.centerIn: parent
                    text: "月"
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: scene.switchToMoonView()
                }
            }
        }
    }
    
    // 动画指示器
    BusyIndicator {
        anchors.left: viewSwitchPanel.right
        anchors.verticalCenter: viewSwitchPanel.verticalCenter
        anchors.leftMargin: 10
        width: 24
        height: 24
        running: scene.animating
        visible: scene.animating
    }

    FloatingBall {
        id: floatBall
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.rightMargin: 30
        onClicked: configPanel.visible = !configPanel.visible
    }

    OrbitConfigPanel {
        id: configPanel
        anchors.right: floatBall.left
        anchors.rightMargin: 20
        anchors.verticalCenter: floatBall.verticalCenter
        visible: false
    }

    // 慢慢绘制控制按钮
    Rectangle {
        anchors.right: configPanel.left
        anchors.rightMargin: 20
        anchors.verticalCenter: floatBall.verticalCenter
        width: 180
        height: 150
        radius: 8
        color: "#2A000000"
        border.color: "#66ffffff"
        border.width: 1
        visible: configPanel.visible && scene.useTrajectoryFile

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Text {
                width: parent.width
                text: "轨迹回放"
                color: "white"
                font.pixelSize: 13
                font.bold: true
            }

            Rectangle {
                width: parent.width
                height: 30
                radius: 6
                color: "#22000000"
                border.color: "#66ffffff"
                border.width: 1
                Text { anchors.centerIn: parent; text: drawTimer.running ? "暂停" : "开始"; color: "white"; font.pixelSize: 12 }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (!scene.useTrajectoryFile) return
                        if (drawTimer.running) {
                            drawTimer.stop()
                        } else {
                            if (scene.drawProgress <= 0)
                                scene.drawProgress = 1
                            if (PntDataModel) PntDataModel.setTaskStatusInvokable("任务中")
                            drawTimer.start()
                        }
                    }
                }
            }

            Row {
                width: parent.width
                height: 30
                spacing: 10

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 30
                    radius: 6
                    color: "#22000000"
                    border.color: "#66ffffff"
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "重播"; color: "white"; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (!scene.useTrajectoryFile) return
                            drawTimer.stop()
                            scene.drawProgress = 1
                            if (PntDataModel) PntDataModel.setTaskStatusInvokable("任务中")
                            drawTimer.start()
                        }
                    }
                }

                Rectangle {
                    width: (parent.width - 10) / 2
                    height: 30
                    radius: 6
                    color: "#22000000"
                    border.color: "#66ffffff"
                    border.width: 1
                    Text { anchors.centerIn: parent; text: "清除"; color: "white"; font.pixelSize: 12 }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            drawTimer.stop()
                            scene.drawProgress = 0
                            if (PntDataModel) PntDataModel.setTaskStatusInvokable("空闲")
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 6
                radius: 3
                color: "#22FFFFFF"
                Rectangle {
                    height: parent.height
                    radius: 3
                    width: parent.width * (scene.controlPointX.length > 0 ? (scene.drawProgress / scene.controlPointX.length) : 0)
                    color: "#88FFFFFF"
                }
            }
        }
    }
    
    // 轨道调试按钮 (左侧中间)
    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 20
        width: 40
        height: 40
        radius: 20
        visible: false
        color: orbitDebugBtn.containsMouse ? "#4ECDC4" : "#333"
        border.color: "#4ECDC4"
        border.width: 2
        
        Text {
            anchors.centerIn: parent
            text: "轨"
            color: "white"
            font.pixelSize: 16
            font.bold: true
        }
        
        MouseArea {
            id: orbitDebugBtn
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
            }
        }
    }
    
    // 轨道控制点调试面板
    OrbitDebugPanel {
        id: orbitDebugPanel
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 70
        visible: false
        
        selectedIndex: scene.selectedPointIndex
        pointNames: scene.controlPointNames
        pointColors: scene.controlPointColors
        pointX: scene.controlPointX[scene.selectedPointIndex] || 0
        pointY: scene.controlPointY[scene.selectedPointIndex] || 0
        pointZ: scene.controlPointZ[scene.selectedPointIndex] || 0
        
        onPointChanged: function(idx, x, y, z) {
            var xs = scene.controlPointX
            var ys = scene.controlPointY
            var zs = scene.controlPointZ
            xs[idx] = x
            ys[idx] = y
            zs[idx] = z
            scene.controlPointX = xs
            scene.controlPointY = ys
            scene.controlPointZ = zs
        }
        
        onPrintAllClicked: {
            var output = "// 轨道控制点坐标\n\n"
            output += "// 地球轨道\n"
            for (var i = 0; i < 12; i++) {
                output += "// " + controlPointNames[i] + ": (" + 
                          controlPointX[i].toFixed(1) + ", " + 
                          controlPointY[i].toFixed(1) + ", " + 
                          controlPointZ[i].toFixed(1) + ")\n"
            }
            output += "\n// 转移轨道\n"
            for (var i = 12; i < 36; i++) {
                output += "// " + controlPointNames[i] + ": (" + 
                          controlPointX[i].toFixed(1) + ", " + 
                          controlPointY[i].toFixed(1) + ", " + 
                          controlPointZ[i].toFixed(1) + ")\n"
            }
            output += "\n// 月球轨道\n"
            for (var i = 36; i < 72; i++) {
                output += "// " + controlPointNames[i] + ": (" + 
                          controlPointX[i].toFixed(1) + ", " + 
                          controlPointY[i].toFixed(1) + ", " + 
                          controlPointZ[i].toFixed(1) + ")\n"
            }
            console.log("\n" + output)
        }
        
        onPointSelected: function(idx) {
            scene.selectedPointIndex = idx
        }
    }
    
    // ========== 月球轨迹测试面板（已禁用） ==========
    // MoonTrajectoryTestPanel {
    //     visible: scene.viewMode === "moon"
    //     anchors.right: parent.right
    //     anchors.top: parent.top
    //     anchors.margins: 10
    //     width: 220
    //     height: 140
    //     moonModelRef: scene.moonModelRef
    // }
}
