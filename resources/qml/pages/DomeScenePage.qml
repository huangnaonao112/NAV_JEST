import QtQuick
import QtQuick3D
import QtQuick.Layouts
import QtQuick.Controls
import models 1.0
import NavModels

Item {
    id: root
    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    // 灯光控制属性
    property bool lightsEnabled: true
    property real lightBrightness: 5.0
    
    // 当前选中的灯（0-35）
    property int selectedLightIndex: 0  // 0-based index
    
    // 36个灯的独立调节参数（偏移和旋转）
    property var lightAdjustments: [
        // 每个元素: { offsetX, offsetY, offsetZ, rotationX, rotationY, rotationZ }
        { offsetX: 0.00, offsetY: -0.85, offsetZ: 0.00, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #1
        { offsetX: -2.91, offsetY: -0.17, offsetZ: -0.17, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #2
        { offsetX: 8.72, offsetY: 3.93, offsetZ: 0.51, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #3
        { offsetX: -10.43, offsetY: 1.20, offsetZ: 0.51, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #4
        { offsetX: 2.56, offsetY: -0.17, offsetZ: -0.85, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #5
        { offsetX: -0.85, offsetY: -1.54, offsetZ: 5.98, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #6
        { offsetX: 3.25, offsetY: 0.51, offsetZ: 8.72, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #7
        { offsetX: -2.22, offsetY: -3.59, offsetZ: 0.00, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #8
        { offsetX: 0.51, offsetY: -0.85, offsetZ: -0.85, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #9
        { offsetX: 1.88, offsetY: -0.17, offsetZ: -2.22, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #10
        { offsetX: 3.93, offsetY: -0.85, offsetZ: -0.85, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #11
        { offsetX: 8.72, offsetY: -1.54, offsetZ: 5.98, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #12
        { offsetX: 2.56, offsetY: -0.17, offsetZ: 4.62, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #13
        { offsetX: 2.56, offsetY: -0.85, offsetZ: 5.30, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #14
        { offsetX: 4.62, offsetY: -2.22, offsetZ: 4.62, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #15
        { offsetX: -1.54, offsetY: -4.27, offsetZ: -0.17, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #16
        { offsetX: -0.17, offsetY: -2.91, offsetZ: -1.54, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #17
        { offsetX: 1.20, offsetY: -1.54, offsetZ: -2.22, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #18
        { offsetX: 1.88, offsetY: -0.17, offsetZ: -1.54, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #19
        { offsetX: 2.56, offsetY: -0.85, offsetZ: -1.54, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #20
        { offsetX: 2.56, offsetY: 6.67, offsetZ: 5.30, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #21
        { offsetX: 1.20, offsetY: 5.30, offsetZ: -2.91, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #22
        { offsetX: 0.51, offsetY: -0.17, offsetZ: -1.54, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #23
        { offsetX: -1.54, offsetY: -1.54, offsetZ: -2.22, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #24
        { offsetX: -0.17, offsetY: 3.93, offsetZ: -4.96, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #25
        { offsetX: 3.25, offsetY: -0.17, offsetZ: 0.51, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #26
        { offsetX: 1.88, offsetY: 1.20, offsetZ: 0.51, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #27
        { offsetX: 1.20, offsetY: 2.56, offsetZ: 0.51, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #28
        { offsetX: 1.20, offsetY: 3.93, offsetZ: -0.17, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #29
        { offsetX: 0.51, offsetY: 3.25, offsetZ: -0.60, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #30
        { offsetX: -0.85, offsetY: 10.09, offsetZ: -1.54, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #31
        { offsetX: 1.20, offsetY: 12.82, offsetZ: 0.00, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #32
        { offsetX: 1.88, offsetY: 6.67, offsetZ: 1.20, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #33
        { offsetX: 5.30, offsetY: 10.77, offsetZ: -0.17, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #34
        { offsetX: 3.25, offsetY: 8.03, offsetZ: 1.88, rotationX: 0, rotationY: 0, rotationZ: 0 },  // #35
        { offsetX: -0.17, offsetY: 10.09, offsetZ: 1.88, rotationX: 0, rotationY: 0, rotationZ: 0 }   // #36
    ]
    
    // 全局偏移和旋转（整体调节）
    property real globalOffsetX: 0
    property real globalOffsetY: -97.09401709401706
    property real globalOffsetZ: 57.44
    property real globalRotationX: 18.3
    property real globalRotationY: 97
    property real globalRotationZ: 88.61538461538464
    
    // 灯组绕中心点旋转角度 (0-360°)
    property real lightsOrbitAngle: 352.3
    
    // 穹顶顶点位置（用于旋转中心）
    property vector3d domeTopPoint: Qt.vector3d(0, 96.47, 0)  // GLB原始顶点坐标
    
    // 36个灯的位置（原始GLB坐标）
    property var lightPositions: [
        Qt.vector3d(0.00, 96.47, -0.00),    // #1 经0° 纬90° 顶点
        Qt.vector3d(24.97, 93.18, -0.00),   // #2 经0° 纬75°
        Qt.vector3d(-18.93, 87.35, 19.22),  // #3 经120° 纬75°
        Qt.vector3d(0.47, 91.25, -20.19),   // #4 经240° 纬75°
        Qt.vector3d(48.23, 83.54, -0.00),   // #5 经0° 纬60°
        Qt.vector3d(27.24, 83.84, 36.29),   // #6 经60° 纬60°
        Qt.vector3d(-26.55, 80.37, 34.64),  // #7 经120° 纬60°
        Qt.vector3d(-46.49, 84.22, 0.00),   // #8 经180° 纬60°
        Qt.vector3d(-24.14, 83.56, -41.81), // #9 经240° 纬60°
        Qt.vector3d(24.14, 83.56, -41.81),  // #10 经300° 纬60°
        Qt.vector3d(68.21, 68.21, 0.00),    // #11 经0° 纬45°
        Qt.vector3d(49.87, 68.27, 34.64),   // #12 经36° 纬45°
        Qt.vector3d(21.19, 65.21, 61.04),   // #13 经72° 纬45°
        Qt.vector3d(-21.63, 65.21, 61.04),  // #14 经108° 纬45°
        Qt.vector3d(-59.68, 65.36, 36.29),  // #15 经144° 纬45°
        Qt.vector3d(-67.09, 68.45, -0.07),  // #16 经180° 纬45°
        Qt.vector3d(-55.23, 68.21, -40.13), // #17 经216° 纬45°
        Qt.vector3d(-21.10, 68.21, -64.93), // #18 经252° 纬45°
        Qt.vector3d(21.10, 68.21, -64.93),  // #19 经288° 纬45°
        Qt.vector3d(55.23, 68.21, -40.13),  // #20 经324° 纬45°
        Qt.vector3d(77.71, 44.32, 19.22),   // #21 经18° 纬30°
        Qt.vector3d(49.41, 44.01, 68.45),   // #22 经54° 纬30°
        Qt.vector3d(1.88, 48.23, 83.54),    // #23 经90° 纬30°
        Qt.vector3d(-44.90, 48.95, 68.45),  // #24 经126° 纬30°
        Qt.vector3d(-75.74, 42.93, 30.27),  // #25 经162° 纬30°
        Qt.vector3d(-79.52, 48.21, -25.84), // #26 经198° 纬30°
        Qt.vector3d(-49.15, 48.21, -67.65), // #27 经234° 纬30°
        Qt.vector3d(0.48, 48.01, -84.22),   // #28 经270° 纬30°
        Qt.vector3d(49.15, 48.21, -67.65),  // #29 经306° 纬30°
        Qt.vector3d(79.52, 48.21, -25.84),  // #30 经342° 纬30°
        Qt.vector3d(93.18, 24.97, 0.00),    // #31 经0° 纬15°
        Qt.vector3d(46.50, 20.12, 76.67),   // #32 经60° 纬15°
        Qt.vector3d(-44.51, 24.89, 76.67),  // #33 经120° 纬15°
        Qt.vector3d(-94.03, 20.21, -0.10),  // #34 经180° 纬15°
        Qt.vector3d(-46.63, 24.93, -80.77), // #35 经240° 纬15°
        Qt.vector3d(46.63, 24.93, -80.77)   // #36 经300° 纬15°
    ]
    
    // 单个灯调试（可删除）
    property real lightX: 24.97
    property real lightY: 1.00
    property real lightZ: -0.00
    property real lightSize: 2.0
    
    // 相机参数
    property real cameraRotationX: 78
    property real cameraRotationY: -0.2
    property real cameraDistance: 220.4
    
    // 模型旋转属性
    property real modelRotationX: 2.5
    property real modelRotationY: 0.1
    property real modelRotationZ: 17.2
    
    // 打印灯光位置坐标
    function printLightPositions() {
        console.log("\n===== 灯光坐标 =====")
        console.log("Qt.vector3d(" + root.lightX.toFixed(2) + ", " + root.lightY.toFixed(2) + ", " + root.lightZ.toFixed(2) + ")")
        console.log("大小: " + root.lightSize.toFixed(2))
    }

    // 背景 - 与机柜页面保持一致
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0b1220" }
            GradientStop { position: 0.55; color: "#0a2a4a" }
            GradientStop { position: 1.0; color: "#07111f" }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.18
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 0.6; color: "#00000000" }
            GradientStop { position: 1.0; color: "#AA000000" }
        }
    }

    // View3D
    View3D {
        id: view3d
        anchors.fill: parent

        environment: SceneEnvironment {
            backgroundMode: SceneEnvironment.Transparent
            antialiasingMode: SceneEnvironment.MSAA
            antialiasingQuality: SceneEnvironment.High
        }

        Node {
            id: cameraPivot
            eulerRotation.x: root.cameraRotationX
            eulerRotation.y: root.cameraRotationY

            PerspectiveCamera {
                id: camera
                z: root.cameraDistance
                clipNear: 1
                clipFar: Math.max(5000, root.cameraDistance * 2)
                fieldOfView: 60
            }
        }

        DirectionalLight {
            eulerRotation.x: -45
            eulerRotation.y: 45
            brightness: 4.0
        }

        DirectionalLight {
            eulerRotation.x: 45
            eulerRotation.y: -45
            brightness: 3.0
            color: "#E0E0FF"
        }

        DirectionalLight {
            eulerRotation.x: 0
            eulerRotation.y: 0
            brightness: 2.0
        }

        // 穹顶模型（包含灯光）
        DomeWithLights {
            id: domeWithLights
            domeRadius: 12.0
            meridians: 12
            parallels: 5
            tubeRadius: 0.12
            lightSphereRadius: 0.2
            scale: Qt.vector3d(1, 1, 1)
            position: Qt.vector3d(0, 0, 0)
            modelRotationX: root.modelRotationX
            modelRotationY: root.modelRotationY
            modelRotationZ: root.modelRotationZ
            
            // 传入灯光参数
            lightPositions: root.lightPositions
            lightAdjustments: root.lightAdjustments
            selectedLightIndex: root.selectedLightIndex
            lightBrightness: root.lightBrightness
            lightsEnabled: root.lightsEnabled
            
            // 传入全局参数
            globalOffsetX: root.globalOffsetX
            globalOffsetY: root.globalOffsetY
            globalOffsetZ: root.globalOffsetZ
            globalRotationX: root.globalRotationX
            globalRotationY: root.globalRotationY
            globalRotationZ: root.globalRotationZ
            lightsOrbitAngle: root.lightsOrbitAngle
            domeTopPoint: root.domeTopPoint
        }
    }

// 鼠标交互区域
    MouseArea {
        anchors.fill: parent
        z: 10
        property real lastX: 0
        property real lastY: 0

        onPressed: (mouse) => { lastX = mouse.x; lastY = mouse.y; }
        onPositionChanged: (mouse) => {
            if (!pressed) return;
            const dx = mouse.x - lastX;
            const dy = mouse.y - lastY;
            
            // 垂直拖动：X轴旋转（上下转）
            root.modelRotationX += dy * 0.3;
            // 水平拖动：Z轴旋转（翻滚）
            root.modelRotationZ += dx * 0.3;
            
            lastX = mouse.x;
            lastY = mouse.y;
        }

        onWheel: (wheel) => {
            const step = root.cameraDistance * 0.08;
            root.cameraDistance += (wheel.angleDelta.y > 0 ? -step : step);
            root.cameraDistance = Math.max(5, Math.min(1000, root.cameraDistance));
        }
    }

    // 右上角参数面板
    Rectangle {
        id: turntablePanel
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 72
        anchors.rightMargin: 18
        width: 240
        height: contentCol.implicitHeight + 20
        radius: 10
        color: Qt.rgba(0, 0, 0, 0.5)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.7)
        z: 50

        Column {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 18

            Text {
                text: "转台参数"
                color: "#FFFFFF"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }

            GridLayout {
                width: parent.width
                columns: 3
                columnSpacing: 10
                rowSpacing: 8

                Item { Layout.row: 0; Layout.column: 0; Layout.preferredHeight: 18 }
                Text { Layout.row: 0; Layout.column: 1; text: "角度值"; color: "#FFFFFF"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 0; Layout.column: 2; text: "速度值"; color: "#FFFFFF"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                Text { Layout.row: 1; Layout.column: 0; text: "翻滚"; color: "#FFFFFF"; font.pixelSize: 12 }
                Text { Layout.row: 1; Layout.column: 1; text: (PntDataModel ? PntDataModel.turntableRoll.toFixed(1) : "0.0") + "°"; color: "#22C55E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 1; Layout.column: 2; text: (PntDataModel ? PntDataModel.turntableRollSpeed.toFixed(1) : "0.0") + "°/s"; color: "#22C55E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                Text { Layout.row: 2; Layout.column: 0; text: "俯仰"; color: "#FFFFFF"; font.pixelSize: 12 }
                Text { Layout.row: 2; Layout.column: 1; text: (PntDataModel ? PntDataModel.turntablePitch.toFixed(1) : "0.0") + "°"; color: "#22C55E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 2; Layout.column: 2; text: (PntDataModel ? PntDataModel.turntablePitchSpeed.toFixed(1) : "0.0") + "°/s"; color: "#22C55E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                Text { Layout.row: 3; Layout.column: 0; text: "偏航"; color: "#FFFFFF"; font.pixelSize: 12 }
                Text { Layout.row: 3; Layout.column: 1; text: (PntDataModel ? PntDataModel.turntableYaw.toFixed(1) : "0.0") + "°"; color: "#22C55E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 3; Layout.column: 2; text: (PntDataModel ? PntDataModel.turntableYawSpeed.toFixed(1) : "0.0") + "°/s"; color: "#22C55E"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
            }

            Rectangle { width: parent.width; height: 1; color: Qt.rgba(255/255, 255/255, 255/255, 0.5) }

            Text {
                text: "待测设备参数"
                color: "#FFFFFF"
                font.pixelSize: 14
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                width: parent.width
            }

            GridLayout {
                width: parent.width
                columns: 3
                columnSpacing: 10
                rowSpacing: 8

                Item { Layout.row: 0; Layout.column: 0; Layout.preferredHeight: 18 }
                Text { Layout.row: 0; Layout.column: 1; text: "角度值"; color: "#FFFFFF"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 0; Layout.column: 2; text: "速度值"; color: "#FFFFFF"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                Text { Layout.row: 1; Layout.column: 0; text: "翻滚"; color: "#FFFFFF"; font.pixelSize: 12 }
                Text { Layout.row: 1; Layout.column: 1; text: (PntDataModel ? PntDataModel.dutRoll.toFixed(1) : "0.0") + "°"; color: "#3B82F6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 1; Layout.column: 2; text: (PntDataModel ? PntDataModel.dutRollSpeed.toFixed(1) : "0.0") + "°/s"; color: "#3B82F6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                Text { Layout.row: 2; Layout.column: 0; text: "俯仰"; color: "#FFFFFF"; font.pixelSize: 12 }
                Text { Layout.row: 2; Layout.column: 1; text: (PntDataModel ? PntDataModel.dutPitch.toFixed(1) : "0.0") + "°"; color: "#3B82F6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 2; Layout.column: 2; text: (PntDataModel ? PntDataModel.dutPitchSpeed.toFixed(1) : "0.0") + "°/s"; color: "#3B82F6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }

                Text { Layout.row: 3; Layout.column: 0; text: "偏航"; color: "#FFFFFF"; font.pixelSize: 12 }
                Text { Layout.row: 3; Layout.column: 1; text: (PntDataModel ? PntDataModel.dutYaw.toFixed(1) : "0.0") + "°"; color: "#3B82F6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
                Text { Layout.row: 3; Layout.column: 2; text: (PntDataModel ? PntDataModel.dutYawSpeed.toFixed(1) : "0.0") + "°/s"; color: "#3B82F6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; Layout.fillWidth: true }
            }
        }
    }
    
    // 灯光控制面板（左下角）- 36灯独立调节（已拆分至 DomeLightPanel.qml）
    DomeLightPanel {
        id: lightPanel
        page: root
    }
}
