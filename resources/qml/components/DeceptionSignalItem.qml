import QtQuick
import QtQuick3D

/**
 * 欺骗卫星干扰信号效果组件
 * 从卫星向地面发射红色扩散信号环
 */
Node {
    id: root

    // 卫星位置 (km单位)
    property vector3d satellitePosition: Qt.vector3d(0, 0, 0)

    // 目标地面点位置 (km单位)
    property vector3d targetPosition: Qt.vector3d(0, 0, 0)

    // 信号颜色
    property color signalColor: "#FF0000"

    // 是否激活
    property bool active: true

    // 动画参数
    readonly property int ringCount: 4        // 同时在空中的环数量
    readonly property int flightDuration: 2000 // 环飞行时间(ms)
    readonly property int emitInterval: 500    // 发射间隔(ms)

    // 计算信号距离（从卫星到地面）
    readonly property real beamLength: Math.sqrt(
        (targetPosition.x - satellitePosition.x) * (targetPosition.x - satellitePosition.x) +
        (targetPosition.y - satellitePosition.y) * (targetPosition.y - satellitePosition.y) +
        (targetPosition.z - satellitePosition.z) * (targetPosition.z - satellitePosition.z)
    )

    visible: root.active

    // 设置位置在卫星位置
    position: satellitePosition

    // 使用与 GroundWarningItem 相同的旋转计算方式
    // 让信号指向地心（-satellitePosition 方向）
    eulerRotation: {
        var pos = satellitePosition
        var len = Math.sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z)
        var yaw = Math.atan2(pos.x, pos.z) * 180 / Math.PI
        var pitch = -Math.asin(pos.y / Math.max(0.1, len)) * 180 / Math.PI
        // +90 让信号指向地心（与 GroundWarningItem 的 -90 相反）
        return Qt.vector3d(pitch + 90, yaw, 0)
    }

    // 全局时间持续增长（不循环）
    property real globalTime: 0

    NumberAnimation on globalTime {
        from: 0
        to: 100000  // 足够大的值
        duration: 100000
        loops: Animation.Infinite
        running: root.active
    }

    // 计算环的进度，使用取模实现无缝循环
    function ringProgress(ringIndex) {
        var t = root.globalTime
        // 计算该环的发射时间（考虑周期）
        var launchTime = Math.floor(t / root.emitInterval) * root.emitInterval + ringIndex * root.emitInterval
        // 如果发射时间在未来，用上一周期
        if (launchTime > t) {
            launchTime -= root.emitInterval * root.ringCount
        }
        var elapsed = t - launchTime
        var progress = elapsed / root.flightDuration
        return progress
    }

    // 信号环0
    Node {
        id: ring0
        property real progress: ringProgress(0)
        visible: root.active && progress >= 0 && progress <= 1
        y: -progress * root.beamLength

        Model {
            source: "#Cylinder"
            scale: Qt.vector3d(8 + ring0.progress * 32, 0.005, 8 + ring0.progress * 32)
            opacity: 0.7 * (1.0 - ring0.progress * 0.5)
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(root.signalColor.r, root.signalColor.g, root.signalColor.b, 0.7)
                lighting: DefaultMaterial.NoLighting
                emissiveFactor: Qt.vector3d(1.0, 0.3, 0.3)
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.NeverDepthDraw
                opacity: parent.opacity
            }
        }
    }

    // 信号环1
    Node {
        id: ring1
        property real progress: ringProgress(1)
        visible: root.active && progress >= 0 && progress <= 1
        y: -progress * root.beamLength

        Model {
            source: "#Cylinder"
            scale: Qt.vector3d(8 + ring1.progress * 32, 0.005, 8 + ring1.progress * 32)
            opacity: 0.7 * (1.0 - ring1.progress * 0.5)
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(root.signalColor.r, root.signalColor.g, root.signalColor.b, 0.7)
                lighting: DefaultMaterial.NoLighting
                emissiveFactor: Qt.vector3d(1.0, 0.3, 0.3)
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.NeverDepthDraw
                opacity: parent.opacity
            }
        }
    }

    // 信号环2
    Node {
        id: ring2
        property real progress: ringProgress(2)
        visible: root.active && progress >= 0 && progress <= 1
        y: -progress * root.beamLength

        Model {
            source: "#Cylinder"
            scale: Qt.vector3d(8 + ring2.progress * 32, 0.005, 8 + ring2.progress * 32)
            opacity: 0.7 * (1.0 - ring2.progress * 0.5)
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(root.signalColor.r, root.signalColor.g, root.signalColor.b, 0.7)
                lighting: DefaultMaterial.NoLighting
                emissiveFactor: Qt.vector3d(1.0, 0.3, 0.3)
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.NeverDepthDraw
                opacity: parent.opacity
            }
        }
    }

    // 信号环3
    Node {
        id: ring3
        property real progress: ringProgress(3)
        visible: root.active && progress >= 0 && progress <= 1
        y: -progress * root.beamLength

        Model {
            source: "#Cylinder"
            scale: Qt.vector3d(8 + ring3.progress * 32, 0.005, 8 + ring3.progress * 32)
            opacity: 0.7 * (1.0 - ring3.progress * 0.5)
            materials: DefaultMaterial {
                diffuseColor: Qt.rgba(root.signalColor.r, root.signalColor.g, root.signalColor.b, 0.7)
                lighting: DefaultMaterial.NoLighting
                emissiveFactor: Qt.vector3d(1.0, 0.3, 0.3)
                cullMode: DefaultMaterial.NoCulling
                depthDrawMode: DefaultMaterial.NeverDepthDraw
                opacity: parent.opacity
            }
        }
    }
}
