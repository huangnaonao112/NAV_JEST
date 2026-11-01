// DomeScenePage 的 36 灯独立调节面板（左下角）。
// 通过 page 属性回访宿主的灯光参数数组与选中状态。
import QtQuick
import QtQuick.Controls

Item {
    id: panelRoot

    // 宿主 DomeScenePage 引用（提供灯光参数数组与选中状态）
    required property var page
    anchors.fill: parent

    // 灯光控制面板（左下角）- 支持36个灯独立调节
    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 18
        anchors.bottomMargin: 18
        width: 320
        height: 750
        radius: 10
        color: Qt.rgba(0, 0, 0, 0.85)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.5)
        z: 50
        visible: false  // 隐藏控制面板（保留代码）
        
        // 使面板可滚动
        ScrollView {
            anchors.fill: parent
            anchors.margins: 10
            contentHeight: controlsColumn.implicitHeight + 20
            
            Column {
                id: controlsColumn
                width: parent.width - 20
                spacing: 10
                
                // ========== 灯选择器 ==========
                Text {
                    text: "💡 选择灯光 (1-36)"
                    color: "#FFD700"
                    font.pixelSize: 16
                    font.bold: true
                }
                
                Row {
                    spacing: 8
                    Text { text: "灯号:"; color: "white"; font.pixelSize: 12; width: 40 }
                    SpinBox {
                        id: lightSelector
                        from: 1
                        to: 36
                        value: page.selectedLightIndex + 1
                        onValueModified: page.selectedLightIndex = value - 1
                    }
                    Text {
                        text: "当前: #" + (page.selectedLightIndex + 1)
                        color: "#FFD700"
                        font.pixelSize: 12
                    }
                }
                
                // 选中灯的原始坐标
                Rectangle {
                    width: parent.width
                    height: 40
                    color: "#1a1a1a"
                    radius: 4
                    Text {
                        anchors.centerIn: parent
                        text: page.selectedLightIndex >= 0 && page.selectedLightIndex < page.lightPositions.length ?
                            "原始: (" + page.lightPositions[page.selectedLightIndex].x.toFixed(1) + ", " + 
                            page.lightPositions[page.selectedLightIndex].y.toFixed(1) + ", " + 
                            page.lightPositions[page.selectedLightIndex].z.toFixed(1) + ")" : "N/A"
                        color: "#AAA"
                        font.family: "Consolas"
                        font.pixelSize: 10
                    }
                }
                
                // 分隔线
                Rectangle { width: parent.width; height: 1; color: "#444" }
                
                // ========== 全局调节 ==========
                Text {
                    text: "🌍 全局调节（所有灯）"
                    color: "#4ECDC4"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                // 全局偏移 X
                Row {
                    spacing: 6
                    Text { text: "偏移X:"; color: "#FF6B6B"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.globalOffsetX.toFixed(1)
                        onAccepted: page.globalOffsetX = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -100; to: 100
                        value: page.globalOffsetX
                        onValueChanged: page.globalOffsetX = value
                    }
                }
                
                // 全局偏移 Y
                Row {
                    spacing: 6
                    Text { text: "偏移Y:"; color: "#4ECDC4"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.globalOffsetY.toFixed(1)
                        onAccepted: page.globalOffsetY = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -100; to: 100
                        value: page.globalOffsetY
                        onValueChanged: page.globalOffsetY = value
                    }
                }
                
                // 全局偏移 Z
                Row {
                    spacing: 6
                    Text { text: "偏移Z:"; color: "#95E1D3"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.globalOffsetZ.toFixed(1)
                        onAccepted: page.globalOffsetZ = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -100; to: 100
                        value: page.globalOffsetZ
                        onValueChanged: page.globalOffsetZ = value
                    }
                }
                
                // 全局旋转 X
                Row {
                    spacing: 6
                    Text { text: "旋转X:"; color: "#FF6B6B"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.globalRotationX.toFixed(1)
                        onAccepted: page.globalRotationX = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.globalRotationX
                        onValueChanged: page.globalRotationX = value
                    }
                }
                
                // 全局旋转 Y
                Row {
                    spacing: 6
                    Text { text: "旋转Y:"; color: "#4ECDC4"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.globalRotationY.toFixed(1)
                        onAccepted: page.globalRotationY = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.globalRotationY
                        onValueChanged: page.globalRotationY = value
                    }
                }
                
                // 全局旋转 Z
                Row {
                    spacing: 6
                    Text { text: "旋转Z:"; color: "#95E1D3"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.globalRotationZ.toFixed(1)
                        onAccepted: page.globalRotationZ = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.globalRotationZ
                        onValueChanged: page.globalRotationZ = value
                    }
                }
                
                // 绕中心点旋转 (0-360°)
                Row {
                    spacing: 6
                    Text { text: "绕中心:"; color: "#FFD700"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.lightsOrbitAngle.toFixed(1)
                        onAccepted: page.lightsOrbitAngle = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: 0; to: 360
                        value: page.lightsOrbitAngle
                        onValueChanged: page.lightsOrbitAngle = value
                    }
                }
                
                // 分隔线
                Rectangle { width: parent.width; height: 1; color: "#444" }
                
                // ========== 模型旋转 ==========
                Text {
                    text: "🔄 模型旋转"
                    color: "#FF9500"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                // 模型旋转 X
                Row {
                    spacing: 6
                    Text { text: "旋转X:"; color: "#FF6B6B"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.modelRotationX.toFixed(1)
                        onAccepted: page.modelRotationX = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.modelRotationX
                        onValueChanged: page.modelRotationX = value
                    }
                }
                
                // 模型旋转 Y
                Row {
                    spacing: 6
                    Text { text: "旋转Y:"; color: "#4ECDC4"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.modelRotationY.toFixed(1)
                        onAccepted: page.modelRotationY = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.modelRotationY
                        onValueChanged: page.modelRotationY = value
                    }
                }
                
                // 模型旋转 Z
                Row {
                    spacing: 6
                    Text { text: "旋转Z:"; color: "#95E1D3"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 45
                        text: page.modelRotationZ.toFixed(1)
                        onAccepted: page.modelRotationZ = parseFloat(text) || 0
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.modelRotationZ
                        onValueChanged: page.modelRotationZ = value
                    }
                }
                
                // 分隔线
                Rectangle { width: parent.width; height: 1; color: "#444" }
                
                // ========== 独立调节（当前选中灯） ==========
                Text {
                    text: "🎯 独立调节（灯 #" + (page.selectedLightIndex + 1) + "）"
                    color: "#FF6B6B"
                    font.pixelSize: 14
                    font.bold: true
                }
                
                // 独立偏移 X
                Row {
                    spacing: 6
                    Text { text: "偏移X:"; color: "#FF6B6B"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 50
                        text: page.lightAdjustments[page.selectedLightIndex].offsetX.toFixed(2)
                        onAccepted: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].offsetX = parseFloat(text) || 0
                            page.lightAdjustments = adj  // 触发更新
                        }
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -50; to: 50
                        value: page.lightAdjustments[page.selectedLightIndex].offsetX
                        onValueChanged: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].offsetX = value
                            page.lightAdjustments = adj
                        }
                    }
                }
                
                // 独立偏移 Y
                Row {
                    spacing: 6
                    Text { text: "偏移Y:"; color: "#4ECDC4"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 50
                        text: page.lightAdjustments[page.selectedLightIndex].offsetY.toFixed(2)
                        onAccepted: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].offsetY = parseFloat(text) || 0
                            page.lightAdjustments = adj
                        }
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -50; to: 50
                        value: page.lightAdjustments[page.selectedLightIndex].offsetY
                        onValueChanged: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].offsetY = value
                            page.lightAdjustments = adj
                        }
                    }
                }
                
                // 独立偏移 Z
                Row {
                    spacing: 6
                    Text { text: "偏移Z:"; color: "#95E1D3"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 50
                        text: page.lightAdjustments[page.selectedLightIndex].offsetZ.toFixed(2)
                        onAccepted: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].offsetZ = parseFloat(text) || 0
                            page.lightAdjustments = adj
                        }
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -50; to: 50
                        value: page.lightAdjustments[page.selectedLightIndex].offsetZ
                        onValueChanged: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].offsetZ = value
                            page.lightAdjustments = adj
                        }
                    }
                }
                
                // 独立旋转 X
                Row {
                    spacing: 6
                    Text { text: "旋转X:"; color: "#FF6B6B"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 50
                        text: page.lightAdjustments[page.selectedLightIndex].rotationX.toFixed(2)
                        onAccepted: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].rotationX = parseFloat(text) || 0
                            page.lightAdjustments = adj
                        }
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.lightAdjustments[page.selectedLightIndex].rotationX
                        onValueChanged: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].rotationX = value
                            page.lightAdjustments = adj
                        }
                    }
                }
                
                // 独立旋转 Y
                Row {
                    spacing: 6
                    Text { text: "旋转Y:"; color: "#4ECDC4"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 50
                        text: page.lightAdjustments[page.selectedLightIndex].rotationY.toFixed(2)
                        onAccepted: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].rotationY = parseFloat(text) || 0
                            page.lightAdjustments = adj
                        }
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.lightAdjustments[page.selectedLightIndex].rotationY
                        onValueChanged: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].rotationY = value
                            page.lightAdjustments = adj
                        }
                    }
                }
                
                // 独立旋转 Z
                Row {
                    spacing: 6
                    Text { text: "旋转Z:"; color: "#95E1D3"; font.pixelSize: 11; width: 50 }
                    TextField {
                        width: 50
                        text: page.lightAdjustments[page.selectedLightIndex].rotationZ.toFixed(2)
                        onAccepted: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].rotationZ = parseFloat(text) || 0
                            page.lightAdjustments = adj
                        }
                        color: "white"
                        font.pixelSize: 10
                        background: Rectangle { color: "#333"; radius: 3 }
                    }
                    Slider {
                        width: 130
                        from: -180; to: 180
                        value: page.lightAdjustments[page.selectedLightIndex].rotationZ
                        onValueChanged: {
                            var adj = page.lightAdjustments
                            adj[page.selectedLightIndex].rotationZ = value
                            page.lightAdjustments = adj
                        }
                    }
                }
                
                // 分隔线
                Rectangle { width: parent.width; height: 1; color: "#444" }
                
                // ========== 按钮区域 ==========
                Row {
                    spacing: 10
                    
                    Button {
                        text: "打印所有参数"
                        onClicked: {
                            console.log("\n===== 36个灯的参数 =====")
                            console.log("全局偏移: (" + page.globalOffsetX + ", " + page.globalOffsetY + ", " + page.globalOffsetZ + ")")
                            console.log("全局旋转: (" + page.globalRotationX + ", " + page.globalRotationY + ", " + page.globalRotationZ + ")")
                            console.log("\n每个灯的独立参数:")
                            for (var i = 0; i < 36; i++) {
                                var adj = page.lightAdjustments[i]
                                console.log("  灯 #" + (i+1) + ": 偏移(" + 
                                    adj.offsetX.toFixed(2) + ", " + 
                                    adj.offsetY.toFixed(2) + ", " + 
                                    adj.offsetZ.toFixed(2) + ") 旋转(" +
                                    adj.rotationX.toFixed(2) + ", " +
                                    adj.rotationY.toFixed(2) + ", " +
                                    adj.rotationZ.toFixed(2) + ")")
                            }
                            console.log("===== 参数结束 =====\n")
                        }
                    }
                    
                    Button {
                        text: "复制到剪贴板"
                        onClicked: {
                            // 这里可以添加复制功能
                            console.log("参数已打印到控制台")
                        }
                    }
                }
            }
        }
    }

}
