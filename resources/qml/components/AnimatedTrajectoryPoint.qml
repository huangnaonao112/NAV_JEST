import QtQuick
import QtQuick3D

/**
 * 动画轨迹点 - 带呼吸闪烁和扩散波纹效果
 * 参考2D地球场景的轨迹点动画（Map2DView.qml:340-369）
 */
Node {
    id: root
    
    // 输入属性
    property vector3d pointPosition: Qt.vector3d(0, 0, 0)
    property color pointColor: "#22C55E"  // 默认绿色
    property real baseSize: 0.3  // 基础大小
    
    // 设置位置
    position: pointPosition
    
    // 1. 基础球体（呼吸闪烁）
    Model {
        source: "#Sphere"
        scale: Qt.vector3d(root.baseSize, root.baseSize, root.baseSize)
        
        materials: PrincipledMaterial {
            baseColor: root.pointColor
            emissiveFactor: Qt.vector3d(0.3, 0.3, 0.3)
            lighting: PrincipledMaterial.NoLighting
        }
        
        // 呼吸闪烁动画
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { 
                from: 1.0
                to: 0.45
                duration: 650
                easing.type: Easing.InOutQuad
            }
            NumberAnimation { 
                from: 0.45
                to: 1.0
                duration: 650
                easing.type: Easing.InOutQuad
            }
        }
    }
    
    // 2. 扩散波纹（淡出动画）
    Model {
        source: "#Sphere"
        scale: Qt.vector3d(root.baseSize * 2.5, root.baseSize * 2.5, root.baseSize * 2.5)
        
        materials: PrincipledMaterial {
            baseColor: root.pointColor
            opacity: 0.6
            alphaMode: PrincipledMaterial.Blend
            lighting: PrincipledMaterial.NoLighting
        }
        
        // 扩散动画
        SequentialAnimation on scale {
            loops: Animation.Infinite
            Vector3dAnimation {
                from: Qt.vector3d(root.baseSize * 1.5, root.baseSize * 1.5, root.baseSize * 1.5)
                to: Qt.vector3d(root.baseSize * 3.0, root.baseSize * 3.0, root.baseSize * 3.0)
                duration: 900
                easing.type: Easing.OutQuad
            }
            Vector3dAnimation {
                from: Qt.vector3d(root.baseSize * 3.0, root.baseSize * 3.0, root.baseSize * 3.0)
                to: Qt.vector3d(root.baseSize * 1.5, root.baseSize * 1.5, root.baseSize * 1.5)
                duration: 900
                easing.type: Easing.InQuad
            }
        }
        
        // 淡出动画
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { from: 0.8; to: 0.25; duration: 900; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 0.25; to: 0.8; duration: 900; easing.type: Easing.InOutQuad }
        }
    }
}