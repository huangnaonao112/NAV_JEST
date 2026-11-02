import QtQuick
import QtQuick3D

Node {
    id: root
    
    property vector3d anchorPosition: Qt.vector3d(0, 0, 0)
    property color color: "red"
    property real radius: 500.0 // 默认 500km
    property int type: 0        // 0: Base, 1: Jammer, 2: Target, 3: Trajectory, 4: Spoofed
    property real errorRadius: 0.0
    
    // 场景比例：1 单位 = 1 km（地球 #Sphere 基准半径 50 × scale 127.42 = 6371）
    // 入参 radius 即 km，可直接作为场景半径；额外 /100 会让标记缩小到不可见
    readonly property real scaledRadius: radius
    readonly property real scaledErrorRadius: errorRadius
    
    // Position the node - 接收km单位（C++端），由scaledRadius处理转换
    x: anchorPosition.x
    y: anchorPosition.y
    z: anchorPosition.z
    
    // Rotate to face away from center (normal to surface)
    eulerRotation: {
        var yaw = Math.atan2(anchorPosition.x, position.z) * 180 / Math.PI;
        var pitch = -Math.asin(anchorPosition.y / Math.max(0.1, Math.sqrt(anchorPosition.x*position.x + position.y*position.y + position.z*position.z))) * 180 / Math.PI;
        return Qt.vector3d(pitch - 90, yaw, 0); // -90 to lay flat on surface
    }

    // ========== 中心亮点 - 扁圆盘发光点 ==========
    // 外层发光光晕（根据radius缩放）
    Model {
        source: "#Cylinder"
        y: 0.0  // 紧贴表面
        scale: Qt.vector3d(root.scaledRadius * 0.12, 0.008, root.scaledRadius * 0.12)  // 半径的12%
        materials: [
            DefaultMaterial {
                diffuseColor: Qt.rgba(root.color.r, root.color.g, root.color.b, 0.4)
                lighting: DefaultMaterial.NoLighting
                emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.AlwaysDepthDraw
            }
        ]
    }
    
    // 内层核心亮点（更亮更小，根据radius缩放）
    Model {
        source: "#Cylinder"
        y: 0.0
        scale: Qt.vector3d(root.scaledRadius * 0.05, 0.01, root.scaledRadius * 0.05)  // 半径的5%
        objectName: "surfacePoint"
        materials: [
            DefaultMaterial {
                diffuseColor: root.color
                lighting: DefaultMaterial.NoLighting
                emissiveFactor: Qt.vector3d(1, 1, 1)  // 强发光
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.AlwaysDepthDraw
            }
        ]
    }

    // ========== 扩散波纹动画 ==========
    // Wave 1 - 第一层波纹
    Model {
        id: wave1
        source: "#Cylinder"
        y: 0.0  // 紧贴表面
        scale: Qt.vector3d(0.4, 0.005, 0.4) 
        visible: root.type === 1 || root.type === 2 || root.type === 3 || root.type === 4
        
        materials: [
            DefaultMaterial {
                diffuseColor: root.color
                lighting: DefaultMaterial.NoLighting
                opacity: wave1.opacity
                cullMode: DefaultMaterial.NoCulling
                emissiveFactor: Qt.vector3d(0.8, 0.8, 0.8)
                depthDrawMode: DefaultMaterial.NeverDepthDraw
            }
        ]

        SequentialAnimation on scale {
            loops: Animation.Infinite
            Vector3dAnimation {
                from: Qt.vector3d(0.4, 0.005, 0.4)
                to: Qt.vector3d(root.scaledRadius, 0.005, root.scaledRadius)
                duration: 2000
                easing.type: Easing.OutQuad
            }
        }
        
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation {
                from: 0.8 
                to: 0.0
                duration: 2000
                easing.type: Easing.OutQuad
            }
        }
    }

    // Wave 2 - 第二层波纹（延迟启动）
    Model {
        id: wave2
        source: "#Cylinder"
        y: 0.0
        scale: Qt.vector3d(0.4, 0.005, 0.4)
        opacity: 0.0
        visible: root.type === 1 || root.type === 2 || root.type === 3 || root.type === 4
        
        materials: [
            DefaultMaterial {
                diffuseColor: root.color
                lighting: DefaultMaterial.NoLighting
                opacity: wave2.opacity
                cullMode: DefaultMaterial.NoCulling
                emissiveFactor: Qt.vector3d(0.8, 0.8, 0.8)
                depthDrawMode: DefaultMaterial.NeverDepthDraw
            }
        ]

        SequentialAnimation {
            loops: Animation.Infinite
            PauseAnimation { duration: 1000 }
            ParallelAnimation {
                Vector3dAnimation {
                    target: wave2
                    property: "scale"
                    from: Qt.vector3d(0.4, 0.005, 0.4)
                    to: Qt.vector3d(root.scaledRadius, 0.005, root.scaledRadius)
                    duration: 2000
                    easing.type: Easing.OutQuad
                }
                SequentialAnimation {
                    NumberAnimation { target: wave2; property: "opacity"; from: 0.0; to: 0.8; duration: 500 }
                    NumberAnimation { target: wave2; property: "opacity"; from: 0.8; to: 0.0; duration: 1500 }
                }
            }
        }
    }

    // Error radius circle (Target only) - 误差圆
    Model {
        source: "#Cylinder"
        y: 0.0
        scale: Qt.vector3d(root.scaledErrorRadius, 0.003, root.scaledErrorRadius)
        visible: root.type === 2 && root.errorRadius > 0
        materials: [
            DefaultMaterial {
                diffuseColor: Qt.rgba(root.color.r, root.color.g, root.color.b, 0.3)
                lighting: DefaultMaterial.NoLighting
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.AlwaysDepthDraw
            }
        ]
    }
}