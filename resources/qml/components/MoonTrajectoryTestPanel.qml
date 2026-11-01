import QtQuick
import QtQuick.Controls
import NavModels

/**
 * 月球轨迹动态更新测试面板
 * 模拟1秒1次的报文数据源
 */
Rectangle {
    id: root
    width: 300
    height: 200
    color: "#2C3E50"
    radius: 10
    border.color: "#34495E"
    border.width: 2
    
    // 外部传入的moonModel引用
    property var moonModelRef: null
    
    Column {
        anchors.centerIn: parent
        spacing: 15
        
        Text {
            text: "月球轨迹动态测试"
            font.pixelSize: 18
            font.bold: true
            color: "#ECF0F1"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Text {
            id: statusText
            text: "轨迹点: " + (MoonTrajectoryModel ? MoonTrajectoryModel.trajectoryCount : 0)
            font.pixelSize: 14
            color: "#BDC3C7"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Row {
            spacing: 10
            anchors.horizontalCenter: parent.horizontalCenter
            
            Button {
                text: "开始模拟"
                onClicked: {
                    trajectoryTimer.running = true
                    statusText.text = "运行中..."
                }
            }
            
            Button {
                text: "停止"
                onClicked: {
                    trajectoryTimer.running = false
                    statusText.text = "已停止"
                }
            }
            
            Button {
                text: "清空"
                width: 60
                onClicked: {
                    trajectoryTimer.running = false
                    if (moonModelRef && typeof moonModelRef.clearTrajectory === 'function') {
                        moonModelRef.clearTrajectory()
                    }
                    statusText.text = "已清空"
                    trajectoryTimer.angle = 0
                }
            }
        }
    }
    
    // 1秒定时器：模拟报文数据源
    Timer {
        id: trajectoryTimer
        interval: 1000  // 1秒
        running: false
        repeat: true
        property int angle: 0
        
        onTriggered: {
            console.log("[测试面板] Timer触发, angle=" + angle)
            
            // 模拟轨迹运动（螺旋路径）
            var lat = 20 + 30 * Math.sin(angle * 0.05)  // 纬度：-10° ~ 50°
            var lon = (angle * 5) % 360 - 180  // 经度：-180° ~ 180°
            
            console.log("[测试面板] 调用addTrajectoryPoint: lat=" + lat + ", lon=" + lon)
            
            // 调用 Geometry 接口
            if (moonModelRef && typeof moonModelRef.addTrajectoryPoint === 'function') {
                moonModelRef.addTrajectoryPoint(lat, lon)
            } else {
                console.error("[测试面板] moonModelRef未设置或没有addTrajectoryPoint方法!")
            }
            
            // 更新状态
            statusText.text = "轨迹点: " + (angle + 1) + 
                             " | 位置: " + lat.toFixed(1) + "°, " + lon.toFixed(1) + "°"
            
            angle++
        }
    }
}