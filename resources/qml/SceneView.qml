import QtQuick
import QtQuick3D
import QtQuick3D.Helpers
import "components"
import NavModels

View3D {
    id: view
    anchors.fill: parent

    readonly property real panSpeed: 0.6
    // 顶栏高度（含 topMargin），此区域禁用旋转/平移
    readonly property real topBarHeight: 80

    function resetOrbitCenter() {
        orbitOrigin.x = earthCenter.x
        orbitOrigin.y = earthCenter.y
        orbitOrigin.z = earthCenter.z
    }

    onVisibleChanged: if (visible) resetOrbitCenter()

    environment: SceneEnvironment {
        // Change background to White as requested
        clearColor: "#FFFFFF" 
        backgroundMode: SceneEnvironment.Color
        antialiasingMode: SceneEnvironment.MSAA
        antialiasingQuality: SceneEnvironment.High
        // Qt 6.11+ Order Independent Transparency - 解决轨迹线深度顺序问题
        oitMethod: SceneEnvironment.OITWeightedBlended
    }

    PerspectiveCamera {
        id: camera
        z: 450 
    }
    
    OrbitCameraController {
        id: orbitController
        anchors.fill: parent
        camera: camera
        origin: orbitOrigin
        // 当鼠标在顶栏区域或正在平移时禁用
        enabled: !panArea.panning && !panArea.inTopBar
    }

    // 顶栏区域双击切换最大化/还原（z 值更高，优先于 panArea）
    MouseArea {
        id: topBarDoubleClickArea
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.right: parent.right
        height: view.topBarHeight
        z: 10
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        propagateComposedEvents: true
        onPressed: {
            mouse.accepted = true
        }
        onDoubleClicked: {
            const win = Window.window
            if (win) {
                if (win.visibility === Window.Maximized)
                    win.showNormal()
                else
                    win.showMaximized()
            }
        }
    }

    MouseArea {
        id: panArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        cursorShape: containsMouse && mouseY >= view.topBarHeight ? Qt.SizeAllCursor : Qt.ArrowCursor
        propagateComposedEvents: true
        property real lastX: 0
        property real lastY: 0
        property bool panning: false
        property bool inTopBar: mouseY < view.topBarHeight

        onPressed: (mouse) => {
            // 顶栏区域接受事件但不处理平移，等待双击
            if (mouse.y < view.topBarHeight) {
                mouse.accepted = true
                return
            }
            if (mouse.button !== Qt.LeftButton) {
                mouse.accepted = false
                return
            }

            const pickResult = view.pick(mouse.x, mouse.y)
            panning = !pickResult.objectHit

            if (!panning) {
                mouse.accepted = false
                return
            }
            lastX = mouse.x
            lastY = mouse.y
            mouse.accepted = true
        }

        onDoubleClicked: {
            // 顶栏区域双击切换最大化/还原
            if (mouseY < view.topBarHeight) {
                const win = Window.window
                if (win) {
                    if (win.visibility === Window.Maximized)
                        win.showNormal()
                    else
                        win.showMaximized()
                }
                mouse.accepted = true
            }
        }

        onPositionChanged: (mouse) => {
            if (!pressed)
                return

            if (!panning || !(mouse.buttons & Qt.LeftButton)) {
                mouse.accepted = false
                return
            }

            const dx = mouse.x - lastX
            const dy = mouse.y - lastY

            orbitOrigin.x -= dx * view.panSpeed
            orbitOrigin.y += dy * view.panSpeed

            lastX = mouse.x
            lastY = mouse.y
            mouse.accepted = true
        }

        onReleased: (mouse) => {
            panning = false
        }
    }

    // High intensity ambient/directional lighting to ensure brightness
    
    // Main Sunlight
    DirectionalLight {
        eulerRotation.x: -30
        eulerRotation.y: -30
        brightness: 1.5
        castsShadow: true
        ambientColor: "#808080" // High ambient level
    }

    // Fill Light 1
    DirectionalLight {
        eulerRotation.x: 30
        eulerRotation.y: 150
        brightness: 1.0
        color: "#E0E0E0"
    }
    
    // Fill Light 2 (Bottom up)
    DirectionalLight {
        eulerRotation.x: 90
        brightness: 0.8
        color: "#E0E0E0"
    }

    Node {
        id: earthCenter
        EarthItem {
            id: earthItem
        }
    }

    Node {
        id: orbitOrigin
        x: earthCenter.x
        y: earthCenter.y
        z: earthCenter.z
    }

    SatelliteInstancer {
        satelliteModel: SatelliteModel
    }
}
