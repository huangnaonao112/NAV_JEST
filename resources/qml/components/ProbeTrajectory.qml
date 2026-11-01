import QtQuick
import QtQuick3D

Node {
    id: root
    
    // 五阶段颜色定义
    // 发射(红) -> 转移(青) -> 制动(蓝) -> 环月(绿) -> 着陆(黄)
    property var stageColors: ["#FF6B6B", "#4ECDC4", "#45B7D1", "#96CEB4", "#FFEAA7"]
    property var stageNames: ["发射", "转移", "制动", "环月", "着陆"]
    
    // 从 C++ 模型获取的轨迹数据
    // 数据结构: [[{x, y, z}, ...], [{x, y, z}, ...], ...] 5个阶段的轨迹点数组
    property var trajectoryPoints: []
    
    property real thickness: 0.3
    
    // 渲染五段轨迹
    Repeater3D {
        id: stageRepeater
        model: 5
        
        delegate: Node {
            // 该阶段的轨迹点
            property var stagePoints: {
                if (!root.trajectoryPoints || root.trajectoryPoints.length <= index) {
                    return [];
                }
                return root.trajectoryPoints[index];
            }
            
            // 采样渲染 - 减少模型数量优化性能
            Repeater3D {
                id: segmentRepeater
                
                property int step: {
                    if (stagePoints.length > 1000) return 5;
                    if (stagePoints.length > 500) return 3;
                    if (stagePoints.length > 200) return 2;
                    return 1;
                }
                
                model: {
                    if (stagePoints.length < 2) return 0;
                    return Math.floor((stagePoints.length - 1) / step);
                }
                
                delegate: Model {
                    source: "#Cube"
                    
                    property int startIndex: index * segmentRepeater.step
                    property int endIndex: Math.min(startIndex + segmentRepeater.step, stagePoints.length - 1)
                    
                    property vector3d start: stagePoints[startIndex]
                    property vector3d end: stagePoints[endIndex]
                    property vector3d diff: Qt.vector3d(end.x - start.x, end.y - start.y, end.z - start.z)
                    property real len: Math.sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z)
                    
                    x: (start.x + end.x) / 2
                    y: (start.y + end.y) / 2
                    z: (start.z + end.z) / 2
                    
                    scale: Qt.vector3d(root.thickness, root.thickness, len / 100)
                    
                    eulerRotation: {
                        var yaw = Math.atan2(diff.x, diff.z) * 180 / Math.PI;
                        var pitch = -Math.asin(diff.y / Math.max(0.001, len)) * 180 / Math.PI;
                        return Qt.vector3d(pitch, yaw, 0);
                    }
                    
                    materials: DefaultMaterial {
                        diffuseColor: root.stageColors[index]
                        lighting: DefaultMaterial.NoLighting
                    }
                }
            }
        }
    }
}