import QtQuick
import QtQuick3D

// 位置标记组件 - 用于在3D场景中标记和调试位置
// 使用方法：在 View3D 中直接放置 <PositionMarker position="Qt.vector3d(x,y,z)" />
Node {
    id: root
    
    // 可配置属性
    property color markerColor: "#FF0000"
    property real markerSize: 0.3
    property string labelText: "Marker"
    property bool showLabel: true
    
    // 位置属性（可绑定）
    property alias pos: root.position
    
    // 信号
    signal positionChanged(vector3d newPosition)
    signal clicked()
    
    // 可视化球体（标记点）
    Model {
        id: sphere
        source: "#Sphere"
        scale: Qt.vector3d(root.markerSize, root.markerSize, root.markerSize)
        materials: PrincipledMaterial {
            baseColor: root.markerColor
            emissiveFactor: Qt.vector3d(0.8, 0.8, 0.8)  // 自发光，容易看到

            roughness: 0.3
        }
    }
    
    // 十字线（更精确的位置指示）
    Model {
        source: "#Cylinder"
        scale: Qt.vector3d(0.02, root.markerSize * 2, 0.02)
        materials: PrincipledMaterial {
            baseColor: root.markerColor
            emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)

        }
    }
    
    Model {
        source: "#Cylinder"
        scale: Qt.vector3d(0.02, root.markerSize * 2, 0.02)
        eulerRotation.y: 90
        materials: PrincipledMaterial {
            baseColor: root.markerColor
            emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)

        }
    }
    
    Model {
        source: "#Cylinder"
        scale: Qt.vector3d(0.02, root.markerSize * 2, 0.02)
        eulerRotation.x: 90
        materials: PrincipledMaterial {
            baseColor: root.markerColor
            emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)

        }
    }
    
    // 打印位置信息
    Component.onCompleted: {
        console.log("[PositionMarker] Created at:", root.position.x.toFixed(2), 
                    root.position.y.toFixed(2), root.position.z.toFixed(2))
    }
    
    onPositionChanged: (pos) => {
        console.log("[PositionMarker] Position:", pos.x.toFixed(2), 
                    pos.y.toFixed(2), pos.z.toFixed(2))
        root.positionChanged(pos)
    }
}
