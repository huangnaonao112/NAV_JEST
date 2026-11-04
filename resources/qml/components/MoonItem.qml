import QtQuick
import QtQuick3D
import NavModels

/**
 * 月球模型 - 包含月球本体、报文轨迹点、动态轨迹线
 * 
 * 渲染顺序（使用OIT技术）：
 * 1. 月球球体（不透明，写入深度）
 * 2. 轨迹线（透明，OIT启用，始终可见）
 * 3. 轨迹点（透明，OIT启用）
 * 
 * 关键技术：
 * - Order Independent Transparency (OIT) - Qt 6.11+
 * - 确保轨迹线从任何角度都可见，不被月球遮挡
 */
Node {
    id: moonNode
    
    // 月球在场景中的 X 位置（空间 2 的摆位，尺度见 CoordinateSystem）
    readonly property real moonCenterX: CoordinateSystem.moonSceneX

    // 线宽随相机距离等比缩放系数（父级 SceneViewItem 注入，保持屏幕观感恒定）
    property real lineWidthScale: 1.0
    
    // ========== 报文点轨迹线（红色，TrajectoryLineGeometry 交叉ribbon）==========
    Node {
        eulerRotation.y: moonSphere.eulerRotation.y

        Model {
            id: messageTrajectoryLine
            visible: true
            opacity: 0.9  // 透明度，启用 OIT

            geometry: TrajectoryLineGeometry {
                id: messageTrajectoryGeometry
                // 月面局部坐标单位为 km（1 场景单位 = 1 km），线宽同为 km
                // 具体数值由 CoordinateSystem 统一定义（30 km ≈ 1~2 px）
                thickness: CoordinateSystem.moonTrailThicknessKm * moonNode.lineWidthScale
            }

            materials: DefaultMaterial {
                diffuseColor: "#FF0000"
                lighting: DefaultMaterial.NoLighting
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.NeverDepthDraw
            }
        }
    }
    
    // ========== 测试点轨迹线（已禁用）==========
    // Node {
    //     eulerRotation.y: moonSphere.eulerRotation.y
    //     
    //     Model {
    //         id: testTrajectoryLine
    //         visible: false  // 已禁用
    //         
    //         geometry: MoonTrajectoryGeometry {
    //             id: testTrajectoryGeometry
    //             startColor: "#00FF00"
    //             endColor: "#00C8FF"
    //             lineWidth: 2.0
    //             trajectoryRadius: 1740.0
    //         }
    //         
    //         materials: DefaultMaterial {
    //             diffuseColor: "#00FF00"
    //             lighting: DefaultMaterial.NoLighting
    //             cullMode: DefaultMaterial.NoCulling
    //             depthDrawMode: DefaultMaterial.NeverDepthDraw
    //         }
    //     }
    // }
    
    // Main Moon Body
    Model {
        id: moonSphere
        source: "#Sphere"
        materials: PrincipledMaterial {
            baseColor: "#dddddd"
            baseColorMap: Texture {
                source: "qrc:/resources/images/moon_color.jpg"
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
            }
            emissiveMap: Texture {
                source: "qrc:/resources/images/moon_color.jpg"
            }
            emissiveFactor: Qt.vector3d(0.15, 0.15, 0.15)
            metalness: 0.0
            roughness: 0.6
            specularAmount: 0.5
        }
        // 场景半径 = 月球半径 1737 km（#Sphere 基准半径 50 × scale 34.74）
        scale: Qt.vector3d(CoordinateSystem.moonNodeScale,
                           CoordinateSystem.moonNodeScale,
                           CoordinateSystem.moonNodeScale)
    }
    
    // ========== 监听新轨迹点信号（报文点）==========
    Connections {
        target: MoonTrajectoryModel
        // 批量信号：模型 33ms 节流 flush 后一次推送新点（月球局部 km 坐标）
        function onNewTrajectoryPoints(points) {
            for (var i = 0; i < points.length; ++i) {
                var p = points[i].position
                // 月面局部坐标即 km（空间 A：1 场景单位 = 1 km，月球 #Sphere
                // 基准半径 50 × scale 34.74 = 1737），直接喂入，不做任何换算
                messageTrajectoryGeometry.appendPoint(
                    Qt.vector3d(p.x, p.y, p.z))
            }
        }
        function onTrajectoryChanged() {
            // 当轨迹清空时，同时清除轨迹线几何体
            if (MoonTrajectoryModel && MoonTrajectoryModel.trajectoryCount === 0) {
                messageTrajectoryGeometry.clear()
            }
        }
    }
    
    // ========== 测试点（已禁用）==========
    // Node {
    //     eulerRotation.y: moonSphere.eulerRotation.y
    //     
    //     Repeater3D {
    //         model: MoonTrajectoryModel ? MoonTrajectoryModel.testPoints : []
    //         delegate: GroundWarningItem {
    //             position: modelData.position
    //             color: modelData.color
    //             type: modelData.type
    //             radius: modelData.radius
    //             errorRadius: modelData.errorRadius || 0
    //         }
    //     }
    // }
    
    // ========== 当前位置信标（红色脉冲）==========
    // recentPoints 只含最新 1 个点（小脉冲指示当前位置）；历史轨迹
    // 由上方红色轨迹线呈现。此前 60 个红盘标记沿轨道排布并持续刷新，
    // 叠加波纹动画形成"环月红光一直闪"的视觉噪音。
    Node {
        eulerRotation.y: moonSphere.eulerRotation.y

        Repeater3D {
            model: MoonTrajectoryModel ? MoonTrajectoryModel.recentPoints : []
            delegate: GroundWarningItem {
                position: modelData.position
                color: modelData.color
                type: modelData.type
                radius: modelData.radius
                errorRadius: modelData.errorRadius || 0
            }
        }
    }
    
    // 公开接口：清空报文点轨迹
    function clearTrajectory() {
        messageTrajectoryGeometry.clear()
    }
}
