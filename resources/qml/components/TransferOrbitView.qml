import QtQuick
import QtQuick3D

Node {
    id: root
    
    // 从 C++ 模型获取的轨道数据
    property var transferOrbitModel: null
    
    // 航天器属性
    property real spacecraftScale: 10.0
    
    // 控制点小球属性
    property bool showControlPoints: true
    property real controlPointSize: 30.0  // 控制点球体大小
    
    // 存储轨道段数据
    property var segmentsData: []
    
    // 当模型数据加载完成时
    onTransferOrbitModelChanged: {
        if (transferOrbitModel) {
            console.log("TransferOrbitModel connected, segmentCount:", transferOrbitModel.segmentCount)
            segmentsData = transferOrbitModel.segments
        }
    }
    
    // 监听模型数据加载完成信号
    Connections {
        target: transferOrbitModel
        function onDataLoaded() {
            console.log("Transfer orbit data loaded, segments:", transferOrbitModel.segmentCount)
            segmentsData = transferOrbitModel.segments
        }
        function onSegmentsChanged() {
            console.log("Segments changed, count:", transferOrbitModel.segmentCount)
            segmentsData = transferOrbitModel.segments
        }
    }
    
    // 轨道段渲染
    Repeater3D {
        id: segmentRepeater
        model: segmentsData.length
        
        delegate: Node {
            id: segmentNode
            
            property int segIndex: index
            property var seg: segmentsData[segIndex] || {}
            property bool isVisible: seg.visible !== false
            property color segColor: seg.color || "#FFFFFF"
            property var points: seg.points || []
            
            visible: isVisible && points.length > 1
            
            Component.onCompleted: {
                console.log("Segment", segIndex, "name:", seg.displayName, "points:", points.length)
                if (points.length > 0) {
                    console.log("  First point:", points[0])
                }
            }
            
            // 轨道线渲染
            Repeater3D {
                id: lineRepeater
                
                model: points.length - 1
                
                delegate: Model {
                    source: "#Cube"
                    
                    property vector3d p1: points[index]
                    property vector3d p2: points[index + 1]
                    property vector3d diff: Qt.vector3d(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z)
                    property real len: Math.sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z)
                    
                    x: (p1.x + p2.x) / 2
                    y: (p1.y + p2.y) / 2
                    z: (p1.z + p2.z) / 2
                    
                    // 更粗的线条，确保可见
                    scale: Qt.vector3d(3.0, 3.0, Math.max(1.0, len / 50))
                    
                    eulerRotation: {
                        if (len < 0.001) return Qt.vector3d(0, 0, 0)
                        var yaw = Math.atan2(diff.x, diff.z) * 180 / Math.PI
                        var pitch = -Math.asin(diff.y / len) * 180 / Math.PI
                        return Qt.vector3d(pitch, yaw, 0)
                    }
                    
                    materials: DefaultMaterial {
                        diffuseColor: segColor
                        lighting: DefaultMaterial.NoLighting
                    }
                }
            }
        }
    }
    
    // 控制点小球渲染
    Repeater3D {
        id: controlPointRepeater
        model: segmentsData.length
        
        delegate: Node {
            id: cpSegmentNode
            
            property int segIndex: index
            property var seg: segmentsData[segIndex] || {}
            property var controlPoints: seg.controlPoints || []
            property color segColor: seg.color || "#FFFFFF"
            
            // 每个段的控制点
            Repeater3D {
                model: controlPoints.length
                
                delegate: Node {
                    id: controlPointNode
                    
                    property var cp: controlPoints[index]
                    property vector3d cpPos: cp.position || Qt.vector3d(0, 0, 0)
                    property color cpColor: cp.color || segColor
                    property string cpName: cp.name || ("P" + index)
                    
                    x: cpPos.x
                    y: cpPos.y
                    z: cpPos.z
                    
                    visible: root.showControlPoints
                    
                    // 控制点球体
                    Model {
                        source: "#Sphere"
                        scale: Qt.vector3d(root.controlPointSize, root.controlPointSize, root.controlPointSize)
                        
                        materials: PrincipledMaterial {
                            baseColor: cpColor
                            emissiveFactor: Qt.vector3d(0.8, 0.8, 0.8)  // 自发光，容易看到
                            roughness: 0.3
                            metalness: 0.1
                        }
                    }
                    
                    // 十字线（更精确的位置指示）
                    Model {
                        source: "#Cylinder"
                        scale: Qt.vector3d(0.2, root.controlPointSize * 2, 0.2)
                        materials: PrincipledMaterial {
                            baseColor: cpColor
                            emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)
                        }
                    }
                    
                    Model {
                        source: "#Cylinder"
                        scale: Qt.vector3d(0.2, root.controlPointSize * 2, 0.2)
                        eulerRotation: Qt.vector3d(0, 0, 90)
                        materials: PrincipledMaterial {
                            baseColor: cpColor
                            emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)
                        }
                    }
                    
                    Component.onCompleted: {
                        console.log("[ControlPoint]", cpName, "at (", 
                                    cpPos.x.toFixed(1), ",", 
                                    cpPos.y.toFixed(1), ",", 
                                    cpPos.z.toFixed(1), ")")
                    }
                }
            }
        }
    }
    
    // 航天器模型
    Model {
        id: spacecraft
        visible: transferOrbitModel !== null && transferOrbitModel.segmentCount > 0
        
        source: "#Sphere"
        
        // 位置绑定到模型
        x: transferOrbitModel ? transferOrbitModel.spacecraftPosition.x : 0
        y: transferOrbitModel ? transferOrbitModel.spacecraftPosition.y : 0
        z: transferOrbitModel ? transferOrbitModel.spacecraftPosition.z : 0
        
        scale: Qt.vector3d(root.spacecraftScale, root.spacecraftScale, root.spacecraftScale)
        
        materials: PrincipledMaterial {
            baseColor: "#FFD700"
            metalness: 0.8
            roughness: 0.2
            emissiveFactor: Qt.vector3d(0.5, 0.5, 0.2)
        }
    }
}