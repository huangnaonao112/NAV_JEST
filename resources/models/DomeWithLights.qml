import QtQuick
import QtQuick3D
import NavModels

Node {
    id: domeNode

    // ==================== 模型旋转控制 ====================
    property real modelRotationX: 0
    property real modelRotationY: 0
    property real modelRotationZ: 0

    // ==================== 穹顶参数 ====================
    property real domeRadius: 10.0
    property int meridians: 12
    property int parallels: 6
    property real tubeRadius: 0.15
    property real lightSphereRadius: 0.25

    // ==================== 灯光参数（从 DomeScenePage 传入） ====================
    property var lightPositions: []
    property var lightAdjustments: []
    property int selectedLightIndex: 0
    property real lightBrightness: 5.0
    property bool lightsEnabled: true
    
    // 全局偏移和旋转
    property real globalOffsetX: 0
    property real globalOffsetY: 0
    property real globalOffsetZ: 0
    property real globalRotationX: 0
    property real globalRotationY: 0
    property real globalRotationZ: 0
    property real lightsOrbitAngle: 0
    
    // 穹顶顶点
    property vector3d domeTopPoint: Qt.vector3d(0, 96.47, 0)
    
    // ==================== 灯光状态颜色映射 ====================
    // 状态: 0=未使用(灰), 1=使用(绿), 2=干扰(红)
    function getLightColor(state) {
        switch(state) {
            case 0: return "#888888"  // 未使用 - 更亮的灰色
            case 1: return "#00FF00"  // 使用 - 纯绿色
            case 2: return "#FF0000"  // 干扰 - 纯红色
            default: return "#FFD700" // 默认 - 金色
        }
    }
    
    function getGlowColor(state) {
        switch(state) {
            case 0: return "#666666"  // 未使用 - 灰色光
            case 1: return "#00FF00"  // 使用 - 绿色光
            case 2: return "#FF4444"  // 干扰 - 红色光
            default: return "#FFD700" // 默认 - 金色光
        }
    }

    // ==================== 转台模型属性 ====================
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property url zhuanTaiUrl: Qt.resolvedUrl("file:///" + appDir + "models/liangge/ZhuanTai.qml")
    property url qiongDingUrl: Qt.resolvedUrl("file:///" + appDir + "models/liangge/QiongDing.qml")
    property var zhuanTaiComponent: null
    property var zhuanTaiInstance: null
    property var qiongDingComponent: null
    property var qiongDingInstance: null
    
    // Pivot节点 - 模型绕自身中心旋转
    Node {
        id: modelPivot
        eulerRotation.x: domeNode.modelRotationX
        eulerRotation.y: domeNode.modelRotationY
        eulerRotation.z: domeNode.modelRotationZ
        
        Component.onCompleted: {
            console.log("[DomeWithLights] modelPivot created")
        }
        
        // ==================== 灯光节点（跟随模型旋转） ====================
        Node {
            // 旋转中心：穹顶顶点 + 全局偏移（在模型坐标系中）
            position: Qt.vector3d(
                domeNode.domeTopPoint.x + domeNode.globalOffsetX,
                domeNode.domeTopPoint.y + domeNode.globalOffsetY,
                domeNode.domeTopPoint.z + domeNode.globalOffsetZ
            )
            
            Node {
                // 应用全局旋转 + 灯光独立旋转（不包含模型旋转，因为已经在 modelPivot 下）
                eulerRotation.x: domeNode.globalRotationX
                eulerRotation.y: domeNode.globalRotationY + domeNode.lightsOrbitAngle
                eulerRotation.z: domeNode.globalRotationZ
                
                Repeater3D {
                    model: domeNode.lightPositions.length
                    
                    Node {
                        id: lightNode
                        
                        // 获取当前灯的独立调节参数
                        property var adj: domeNode.lightAdjustments[index]
                        
                        // 位置是相对于顶点的（原始位置 - 顶点位置 + 独立偏移）
                        position: Qt.vector3d(
                            domeNode.lightPositions[index].x - domeNode.domeTopPoint.x + adj.offsetX,
                            domeNode.lightPositions[index].y - domeNode.domeTopPoint.y + adj.offsetY,
                            domeNode.lightPositions[index].z - domeNode.domeTopPoint.z + adj.offsetZ
                        )
                        
                        // 从C++模型获取灯状态（0=未使用, 1=使用, 2=干扰）
                        property int lightState: AntLightModel ? AntLightModel.getState(index) : 0
                        
                        // 监听模型变化信号，强制更新
                        Connections {
                            target: AntLightModel
                            function onStateChanged(idx, state) {
                                if (idx === index) {
                                    lightNode.lightState = state
                                }
                            }
                            function onStatesChanged() {
                                lightNode.lightState = AntLightModel ? AntLightModel.getState(index) : 0
                            }
                        }
                        
                        // 灯泡球体
                        Model {
                            source: "#Sphere"
                            scale: Qt.vector3d(0.05, 0.05, 0.05)
                            materials: PrincipledMaterial {
                                baseColor: domeNode.getLightColor(lightNode.lightState)
                                emissiveFactor: lightNode.lightState === 0 ? 
                                    Qt.vector3d(0.15, 0.15, 0.15) : 
                                    (lightNode.lightState === 1 ? 
                                        Qt.vector3d(0, 1.5, 0) : 
                                        Qt.vector3d(1.5, 0, 0))
                            }
                        }
                        
                        // 点光源 - 光照范围限制在小球附近
                        PointLight {
                            brightness: lightNode.lightState === 0 ? 0.1 : 0.5
                            color: domeNode.getGlowColor(lightNode.lightState)
                            linearFade: 5.0      // 线性衰减，值越大范围越小
                            quadraticFade: 10.0  // 二次衰减，值越大范围越小
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        console.log("[DomeWithLights] Node created")
        
        // 加载转台模型
        zhuanTaiComponent = Qt.createComponent(zhuanTaiUrl)
        if (zhuanTaiComponent.status === Component.Ready) {
            zhuanTaiInstance = zhuanTaiComponent.createObject(modelPivot)
            console.log("[DomeWithLights] ZhuanTai created")
        } else if (zhuanTaiComponent.status === Component.Error) {
            console.log("[DomeWithLights] ZhuanTai ERROR:", zhuanTaiComponent.errorString())
        }
        
        // 加载穹顶模型
        qiongDingComponent = Qt.createComponent(qiongDingUrl)
        if (qiongDingComponent.status === Component.Ready) {
            qiongDingInstance = qiongDingComponent.createObject(modelPivot)
            console.log("[DomeWithLights] QiongDing created")
        } else if (qiongDingComponent.status === Component.Error) {
            console.log("[DomeWithLights] QiongDing ERROR:", qiongDingComponent.errorString())
        }
    }
}