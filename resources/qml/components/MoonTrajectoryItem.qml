import QtQuick
import QtQuick3D

/**
 * 月球轨迹渲染组件
 * 
 * 显示月球表面的运动轨迹和当前位置标记。
 * 参考 GroundWarningItem 实现，贴合球面。
 */
Node {
    id: root
    
    // 轨迹点数组（场景坐标，已包含月球位置偏移）
    property var trajectoryPoints: []
    
    // 月球位置（用于转换为局部坐标）
    property real moonCenterX: 80000.0
    
    // 显示控制
    property bool showTrajectory: true
    property bool showCurrentPosition: true
    
    // 样式配置
    property color lineColor: "#FFD700"        // 金色轨迹线
    property color currentPointColor: "#FF4444" // 红色当前位置标记
    property real lineWidth: 0.3
    
    // 当前位置（最后一个轨迹点，转换为局部坐标）
    readonly property vector3d currentPosition: {
        if (root.trajectoryPoints.length > 0) {
            var p = root.trajectoryPoints[root.trajectoryPoints.length - 1]
            // 转换为局部坐标（相对于月球中心）
            return Qt.vector3d(p.x - root.moonCenterX, p.y, p.z)
        }
        return Qt.vector3d(0, 0, 0)
    }
    
    // ========== 轨迹线渲染 ==========
    Repeater3D {
        id: lineRepeater
        
        // 性能优化：采样渲染
        property int step: {
            if (root.trajectoryPoints.length > 1000) return 5;
            if (root.trajectoryPoints.length > 500) return 3;
            if (root.trajectoryPoints.length > 200) return 2;
            return 1;
        }
        
        model: {
            if (!root.showTrajectory || root.trajectoryPoints.length < 2) return 0;
            return Math.floor((root.trajectoryPoints.length - 1) / step);
        }
        
        delegate: Model {
            source: "#Cube"
            
            property int startIndex: index * lineRepeater.step
            property int endIndex: Math.min(startIndex + lineRepeater.step, root.trajectoryPoints.length - 1)
            
            // 转换为局部坐标
            property vector3d start: {
                var p = root.trajectoryPoints[startIndex]
                return Qt.vector3d(p.x - root.moonCenterX, p.y, p.z)
            }
            property vector3d end: {
                var p = root.trajectoryPoints[endIndex]
                return Qt.vector3d(p.x - root.moonCenterX, p.y, p.z)
            }
            property vector3d diff: Qt.vector3d(end.x - start.x, end.y - start.y, end.z - start.z)
            property real len: Math.sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z)
            
            x: (start.x + end.x) / 2
            y: (start.y + end.y) / 2
            z: (start.z + end.z) / 2
            
            scale: Qt.vector3d(root.lineWidth, root.lineWidth, len / 100)
            
            eulerRotation: {
                var yaw = Math.atan2(diff.x, diff.z) * 180 / Math.PI;
                var pitch = -Math.asin(diff.y / Math.max(0.001, len)) * 180 / Math.PI;
                return Qt.vector3d(pitch, yaw, 0);
            }
            
            materials: DefaultMaterial {
                diffuseColor: root.lineColor
                lighting: DefaultMaterial.NoLighting
                depthDrawMode: DefaultMaterial.NeverDepthDraw
            }
        }
    }
    
    // ========== 测试轨迹点标记 - 用于调试 ==========
    Repeater3D {
        id: testPointRepeater
        model: root.trajectoryPoints
        
        // 调试：在模型变化时输出
        onModelChanged: {
            console.log("[MoonTrajectoryItem] 测试点模型变化: " + (model ? model.length : 0) + " 个点")
        }
        
        delegate: Node {
            id: pointNode
            
            // 转换为局部坐标（相对于月球中心）
            // modelData 是 {x, y, z} 对象
            x: modelData.x - root.moonCenterX
            y: modelData.y
            z: modelData.z
            
            // 使用 Cube 替代 Sphere（Sphere 有渲染问题）
            Model {
                source: "#Cube"
                scale: Qt.vector3d(10.0, 10.0, 10.0)
                
                materials: DefaultMaterial {
                    diffuseColor: index === 0 ? "#FF0000" : "#FFFF00"
                    lighting: DefaultMaterial.NoLighting
                    depthDrawMode: DefaultMaterial.NeverDepthDraw
                }
            }
            
            Component.onCompleted: {
                var distance = Math.sqrt(pointNode.x*pointNode.x + pointNode.y*pointNode.y + pointNode.z*pointNode.z)
                console.log("[MoonTrajectoryItem] 测试点[" + index + "] 位置: (" + 
                          pointNode.x.toFixed(2) + ", " + 
                          pointNode.y.toFixed(2) + ", " + 
                          pointNode.z.toFixed(2) + ") 距离月球中心=" + 
                          distance.toFixed(2))
            }
        }
    }

    // ========== 当前位置标记 - 参考 GroundWarningItem ==========
    // 使用 Node 包裹，添加位置和旋转
    
    Node {
        id: markerNode
        visible: root.showCurrentPosition && root.trajectoryPoints.length > 0
        
        // 设置位置（局部坐标）
        x: root.currentPosition.x
        y: root.currentPosition.y
        z: root.currentPosition.z
        
        // 计算旋转，让标记贴合球面（法线方向）
        // 参考 GroundWarningItem 第 22-26 行
        eulerRotation: {
            var pos = root.currentPosition
            // 已经是局部坐标
            var len = Math.sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z)
            
            if (len < 0.1) return Qt.vector3d(0, 0, 0)
            
            var yaw = Math.atan2(pos.x, pos.z) * 180 / Math.PI
            var pitch = -Math.asin(pos.y / Math.max(0.1, len)) * 180 / Math.PI
            return Qt.vector3d(pitch - 90, yaw, 0)  // -90 让标记平躺
        }
        
        // 中心点 - 使用 Cube 替代 Sphere
        Model {
            source: "#Cube"
            scale: Qt.vector3d(10.0, 10.0, 10.0)
            
            materials: DefaultMaterial {
                diffuseColor: root.currentPointColor
                lighting: DefaultMaterial.NoLighting
                depthDrawMode: DefaultMaterial.NeverDepthDraw
            }
        }

    }
}
