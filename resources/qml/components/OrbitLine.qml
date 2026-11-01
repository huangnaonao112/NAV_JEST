import QtQuick
import QtQuick3D

/**
 * 高性能轨道线渲染组件
 * 优化：减少采样、缓存计算结果
 */
Node {
    id: root
    
    property var points: []
    property color color: "#00CCFF"
    property real thickness: 0.4 
    
    // 采样步长（减少 Model 数量）
    readonly property int step: {
        if (root.points.length > 300) return 3;
        if (root.points.length > 150) return 2;
        return 1;
    }
    
    // 缓存采样后的线段数
    readonly property int segmentCount: {
        if (root.points.length < 2) return 0;
        return Math.floor((root.points.length - 1) / step);
    }
    
    Repeater3D {
        model: root.segmentCount
        
        delegate: Model {
            id: segmentModel
            source: "#Cube"
            
            // 预计算索引
            readonly property int startIdx: index * root.step
            readonly property int endIdx: Math.min(startIdx + root.step, root.points.length - 1)
            
            // 直接访问 points 数组
            readonly property vector3d start: root.points[startIdx] || Qt.vector3d(0, 0, 0)
            readonly property vector3d end: root.points[endIdx] || Qt.vector3d(0, 0, 0)
            readonly property vector3d diff: Qt.vector3d(end.x - start.x, end.y - start.y, end.z - start.z)
            readonly property real len: Math.sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z)
            
            // 位置
            x: (start.x + end.x) / 2
            y: (start.y + end.y) / 2
            z: (start.z + end.z) / 2
            
            // 缩放
            scale: Qt.vector3d(root.thickness, root.thickness, Math.max(0.001, len / 100))
            
            // 旋转
            eulerRotation: Qt.vector3d(
                -Math.asin(diff.y / Math.max(0.001, len)) * 57.2958,
                Math.atan2(diff.x, diff.z) * 57.2958,
                0
            )
            
            // 简化材质
            materials: DefaultMaterial {
                diffuseColor: root.color
                lighting: DefaultMaterial.NoLighting
            }
        }
    }
}
