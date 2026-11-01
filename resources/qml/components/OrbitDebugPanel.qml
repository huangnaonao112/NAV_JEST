import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 轨道控制点悬浮调试面板
Rectangle {
    id: root
    width: 280
    height: 420
    color: Qt.rgba(0.05, 0.05, 0.1, 0.95)
    radius: 10
    border.color: "#4ECDC4"
    border.width: 2
    
    // 信号
    signal pointChanged(int index, real x, real y, real z)
    signal printAllClicked()
    signal pointSelected(int index)
    
    // 属性
    property int selectedIndex: 0
    property var pointNames: []
    property var pointColors: []
    property real pointX: 0
    property real pointY: 0
    property real pointZ: 0
    
    // 关闭按钮
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 5
        width: 24
        height: 24
        radius: 12
        color: closeMA.containsMouse ? "#FF4444" : "#333"
        
        Text {
            anchors.centerIn: parent
            text: "✕"
            color: "white"
            font.pixelSize: 12
        }
        
        MouseArea {
            id: closeMA
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.visible = false
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6
        
        Text {
            text: "轨道控制点调试"
            color: "#FFD700"
            font.pixelSize: 14
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 当前选中
        RowLayout {
            Layout.fillWidth: true
            Text { text: "选中:"; color: "#888" }
            Text { 
                text: pointNames[selectedIndex] || "-"
                color: pointColors[selectedIndex] || "white"
                font.bold: true
            }
            Text { text: "(" + (selectedIndex + 1) + "/72)"; color: "#666"; font.pixelSize: 10 }
        }
        
        // X轴
        RowLayout {
            Layout.fillWidth: true
            Text { text: "X:"; color: "#FF6B6B"; width: 20 }
            Slider {
                id: xSlider
                Layout.fillWidth: true
                from: -90000
                to: 90000
                value: root.pointX
                onMoved: {
                    root.pointX = value
                    root.pointChanged(selectedIndex, value, root.pointY, root.pointZ)
                }
            }
            Text { text: xSlider.value.toFixed(0); color: "white"; width: 45 }
        }
        
        // Y轴
        RowLayout {
            Layout.fillWidth: true
            Text { text: "Y:"; color: "#4ECDC4"; width: 20 }
            Slider {
                id: ySlider
                Layout.fillWidth: true
                from: -90000
                to: 90000
                value: root.pointY
                onMoved: {
                    root.pointY = value
                    root.pointChanged(selectedIndex, root.pointX, value, root.pointZ)
                }
            }
            Text { text: ySlider.value.toFixed(0); color: "white"; width: 45 }
        }
        
        // Z轴
        RowLayout {
            Layout.fillWidth: true
            Text { text: "Z:"; color: "#95E1D3"; width: 20 }
            Slider {
                id: zSlider
                Layout.fillWidth: true
                from: -90000
                to: 90000
                value: root.pointZ
                onMoved: {
                    root.pointZ = value
                    root.pointChanged(selectedIndex, root.pointX, root.pointY, value)
                }
            }
            Text { text: zSlider.value.toFixed(0); color: "white"; width: 45 }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }
        
        // 地球轨道
        Text { text: "地球轨道 (0-11)"; color: "#FF6B6B"; font.pixelSize: 10 }
        Grid {
            columns: 6
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: 12
                Button {
                    text: pointNames[index] || String(index)
                    width: 40
                    height: 22
                    font.pixelSize: 8
                    onClicked: selectPoint(index)
                }
            }
        }
        
        // 转移轨道
        Text { text: "转移 (12-35)"; color: "#4ECDC4"; font.pixelSize: 10 }
        Row {
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: 24
                Button {
                    text: pointNames[12 + index] || ("T" + (index+1))
                    width: 42
                    height: 22
                    font.pixelSize: 9
                    onClicked: selectPoint(12 + index)
                }
            }
        }
        
        // 月球轨道
        Text { text: "月球轨道 (36-71)"; color: "#45B7D1"; font.pixelSize: 10 }
        Grid {
            columns: 6
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: 36
                Button {
                    text: pointNames[36 + index] || String(index)
                    width: 40
                    height: 22
                    font.pixelSize: 8
                    onClicked: selectPoint(36 + index)
                }
            }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: "#333" }
        
        RowLayout {
            Layout.fillWidth: true
            Button { 
                text: "打印坐标"
                onClicked: root.printAllClicked()
                Layout.fillWidth: true 
            }
        }
    }
    
    function selectPoint(idx) {
        selectedIndex = idx
        root.pointSelected(idx)
    }
    
    function updateSliders(x, y, z) {
        pointX = x
        pointY = y
        pointZ = z
    }
}