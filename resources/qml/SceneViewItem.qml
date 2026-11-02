import QtQuick
import QtQuick3D
import QtQuick.Controls
import "components"
import NavModels

Item {
    id: rootItem
    anchors.fill: parent

    property string viewMode: "earth"
    // 初始相机角度：NAVJEST_CAMERA_ROT_X/Y 可覆盖（自动化多角度取证用）
    property real cameraRotationX: typeof _cameraRotX !== "undefined" ? _cameraRotX : -30
    property real cameraRotationY: typeof _cameraRotY !== "undefined" ? _cameraRotY : 15
    
    // 月球模型引用（供测试面板使用）
    property var moonModelRef: null
    
    // 相机目标位置
    property vector3d cameraTargetPosition: Qt.vector3d(0, 0, 0)
    property real cameraDistance: (typeof _cameraDist !== "undefined" && _cameraDist > 0)
                                  ? _cameraDist : CoordinateSystem.defaultCameraDistance

    // 线宽自适应：世界单位线宽会随缩放变粗/变细，这里按
    // cameraDistance / 基准视距 等比缩放，屏幕观感≈恒定 2px。
    // 上下限防止极端缩放下线宽失真。
    readonly property real lineWidthScale: Math.max(0.15, Math.min(1.5,
        cameraDistance / CoordinateSystem.defaultCameraDistance))
    
    // 动画过渡属性
    property vector3d animTargetPosition: Qt.vector3d(0, 0, 0)
    property real animDistance: 30000
    property bool animating: false
    
    // 使用 Behavior 实现硬件加速动画
    Behavior on cameraTargetPosition {
        enabled: rootItem.animating
        Vector3dAnimation {
            duration: 1000
            easing.type: Easing.InOutCubic  // 更丝滑的缓动
            onRunningChanged: {
                if (!running) {
                    rootItem.animating = false
                }
            }
        }
    }
    
    Behavior on cameraDistance {
        enabled: rootItem.animating
        NumberAnimation {
            duration: 1000
            easing.type: Easing.InOutCubic
        }
    }
    
    // 切换到月球视角（平滑过渡，对准轨迹点）
    function switchToMoonView() {
        if (rootItem.viewMode === "moon" || rootItem.animating) return
        rootItem.animating = true
        rootItem.viewMode = "moon"
        
        // 月球场景半径 = 34.74（QML scale值）
        // 参考地球：半径 127.42，相机 30000，比例 1:235
        // 月球相机：34.74 * 235 ≈ 8164，取整 8000
        var targetX = CoordinateSystem.moonSceneX  // 月球位置（空间2）
        var targetY = 0
        var targetZ = 0
        var viewDist = CoordinateSystem.moonViewCameraDistance  // 月球视角距离
        
        // 如果有轨迹点，对准轨迹点
        if (rootItem.moonTrajectoryPoints.length > 0) {
            var lastPoint = rootItem.moonTrajectoryPoints[rootItem.moonTrajectoryPoints.length - 1]
            targetX = lastPoint.x
            targetY = lastPoint.y
            targetZ = lastPoint.z
        }
        
        // 直接设置目标值，Behavior 会自动动画
        rootItem.cameraTargetPosition = Qt.vector3d(targetX, targetY, targetZ)
        rootItem.cameraDistance = viewDist
    }
    
    // 切换到地球视角（平滑过渡）
    function switchToEarthView() {
        if (rootItem.viewMode === "earth" || rootItem.animating) return
        rootItem.animating = true
        rootItem.viewMode = "earth"
        
        // 直接设置目标值，Behavior 会自动动画
        rootItem.cameraTargetPosition = Qt.vector3d(0, 0, 0)
        rootItem.cameraDistance = 30000
    }
    
    // 轨道控制点数据
    property var controlPointX: []
    property var controlPointY: []
    property var controlPointZ: []
    property var controlPointNames: []
    property var controlPointColors: []
    property int selectedPointIndex: 0
    
    // 轨迹加载器
    property bool useTrajectoryFile: false
    // true = 使用 $TRJ 实时轨迹（C++ 批量场景点）；false = 文件/默认控制点通道
    property bool useTelemetryTrajectory: false
    // 探测器小球位置（最新轨迹点，场景 2 单位）
    property vector3d probePos: Qt.vector3d(0, 0, 0)
    property var trajectoryData: null

    // 慢慢绘制参数
    property int drawProgress: 0 // 当前已绘制到的点索引（不含）
    property int drawStep: 1
    property int drawIntervalMs: 16
    
    // ========== 坐标转换参数（唯一来源：CoordinateSystem）==========
    // 场景里存在两套空间，切勿混用：
    //   空间1 天体局部：1 单位 = 1 km（地球/月球/轨道环/月面轨迹）
    //   空间2 地月压缩：1 单位 ≈ 4.805 km（$TRJ 回放轨迹、月球摆位）
    // 详见 docs/坐标系与尺度规范.md
    readonly property real earthRadiusKm: CoordinateSystem.earthRadiusKm
    readonly property real moonRadiusKm: CoordinateSystem.moonRadiusKm
    readonly property real earthMoonDistanceKm: CoordinateSystem.earthMoonDistanceKm

    readonly property real sceneEarthRadius: CoordinateSystem.earthNodeScale
    readonly property real sceneMoonRadius: CoordinateSystem.moonNodeScale
    readonly property real sceneMoonPosition: CoordinateSystem.moonSceneX

    readonly property real distanceScale: CoordinateSystem.distanceScale
    
    Component.onCompleted: {
        // 轨迹数据现在通过TCP TRAJ报文接收，不再从文件加载
        // 主动加载现有的月球轨迹数据（如果有的话）
        Qt.callLater(function() {
            rootItem.updateMoonTrajectoryFromModel()
        })
        
        // 加载默认测试轨迹点（用于对比显示）
        rootItem.initDefaultControlPoints()
    }
    
    // 监听PntDataModel轨迹变化
    // 33ms 节流：$TRJ 推流期信号密集，全量重建大数组绑定代价高，
    // 合并到定时器单次刷新（与 MoonTrajectoryModel 节流口径一致）
    Timer {
        id: trajectoryThrottle
        interval: 33
        running: false
        onTriggered: rootItem.updateTrajectoryFromPntModel()
    }

    Connections {
        target: PntDataModel
        enabled: rootItem.visible
        function onTrajectory3DChanged() {
            if (!trajectoryThrottle.running)
                trajectoryThrottle.start()
        }
    }
    
    // 监听 MoonTrajectoryModel 更新（月球表面轨迹）
    Connections {
        target: MoonTrajectoryModel
        enabled: rootItem.visible
        function onTrajectoryChanged() {
            rootItem.updateMoonTrajectoryFromModel()
        }
        function onCurrentPositionChanged() {
            rootItem.updateMoonTrajectoryFromModel()
        }
    }
    
    // 月球轨迹数据（用于视角切换和渲染）
    property var moonTrajectoryPoints: []
    
    // 从 MoonTrajectoryModel 更新轨迹数据
    function updateMoonTrajectoryFromModel() {
        if (!MoonTrajectoryModel) return
        var count = MoonTrajectoryModel.trajectoryCount
        if (count <= 0) return

        // 使用 currentPosition（世界坐标，包含80000偏移）
        var currentPos = MoonTrajectoryModel.currentPosition
        rootItem.moonTrajectoryPoints = [{
            x: currentPos.x,
            y: currentPos.y,
            z: currentPos.z
        }]
        console.log("月球轨迹更新: 位置(" + currentPos.x + ", " + currentPos.y + ", " + currentPos.z + ")")
    }
    
    // 文件/默认控制点通道的几何点（与 $TRJ 实时通道互斥）
    property var legacyTrajectoryPoints: {
        var arr = []
        arr.length = controlPointX.length
        for (var i = 0; i < controlPointX.length; i++) {
            arr[i] = Qt.vector3d(controlPointX[i] || 0,
                                 controlPointY[i] || 0,
                                 controlPointZ[i] || 0)
        }
        return arr
    }

    // 从PntDataModel更新轨迹数据（33ms 节流后调用）
    // 性能要点：坐标转换与逐点循环已在 C++ 侧批量完成
    // （PntDataModel.trajectory3DScenePoints），此处只做 O(1) 状态更新，
    // 万点推流时不再产生 N 个 JS 对象/次，消除 GC 卡顿。
    function updateTrajectoryFromPntModel() {
        if (!PntDataModel) return
        var count = PntDataModel.trajectory3DCount
        if (count <= 0) {
            drawProgress = 0
            useTrajectoryFile = false
            useTelemetryTrajectory = false
            return
        }
        // 有实时 $TRJ 数据时，停用文件/默认控制点通道，几何体改绑批量数据
        useTelemetryTrajectory = true
        probePos = PntDataModel.trajectory3DLastScenePoint()
        drawProgress = count
        useTrajectoryFile = true
    }

    // 视角测试钩子: NAVJEST_MOON_VIEW_MS=N 时 N 毫秒后切月心视角（自动化验证月面轨迹）
    Timer {
        running: typeof _moonViewAtMs !== "undefined" && _moonViewAtMs > 0
        interval: typeof _moonViewAtMs !== "undefined" ? _moonViewAtMs : 0
        repeat: false
        onTriggered: rootItem.switchToMoonView()
    }

    // 性能测试钩子: NAVJEST_SIMTEST=1 时模拟用户按住拖动地球（60Hz 连续更新相机）
    // 等价于 MouseArea onPositionChanged 的连续触发路径
    // 测试月球视角切换（含 1s 相机过渡动画 + 月球轨迹渲染路径）
    Timer {
        running: _perfSimTest === true
        interval: 9000
        repeat: false
        onTriggered: rootItem.switchToMoonView()
    }
    Timer {
        running: _perfSimTest === true
        interval: 14000
        repeat: false
        onTriggered: rootItem.switchToEarthView()
    }

    // NAVJEST_SIMTEST=1 时一次性添加 12 颗不同倾角的卫星（含轨道线），
    // 模拟真实使用时的场景规模
    Timer {
        running: _perfSimTest === true
        interval: 200
        repeat: true
        property int added: 0
        onTriggered: {
            if (added >= 12) {
                stop()
                return
            }
            const i = added
            SatelliteModel.addSatellite(
                26500 + i * 400,   // 半长轴
                0.02 + (i % 4) * 0.01,
                40 + i * 4,        // 倾角
                i * 30,            // 升交点
                i * 15,            // 近地点幅角
                i * 20,            // 平近点角
                0.0009,            // 平均运动
                i % 2 === 0 ? "#00e5ff" : "#ff9100")
            ++added
        }
    }

    Timer {
        running: _perfSimTest === true
        interval: 16
        repeat: true
        property real dir: 1.0
        onTriggered: {
            rootItem.cameraRotationY -= 2.4
            rootItem.cameraRotationX += dir * 0.8
            if (rootItem.cameraRotationX > 85 || rootItem.cameraRotationX < -85) dir *= -1
            rootItem.cameraRotationX = Math.max(-85, Math.min(85, rootItem.cameraRotationX))
        }
    }

    // ========== 坐标转换函数 ==========
    
    // ECI坐标（km）转场景坐标
    function eciToScene(x, y, z) {
        return {
            x: x * distanceScale,
            y: y * distanceScale,
            z: z * distanceScale
        }
    }
    
    // 场景坐标转ECI坐标（km）
    function sceneToEci(x, y, z) {
        return {
            x: x / distanceScale,
            y: y / distanceScale,
            z: z / distanceScale
        }
    }

    // 从真实ECI坐标文件加载轨迹数据
    function loadTrajectoryFromECIFile(filePath) {
        var xhr = new XMLHttpRequest()
        xhr.open("GET", filePath, false)
        xhr.send()

        if (xhr.status === 200 || xhr.status === 0) {
            try {
                var rawData = JSON.parse(xhr.responseText)
                console.log("ECI轨迹数据加载成功: " + rawData.metadata.name)
                console.log("总点数: " + rawData.metadata.totalPoints)
                console.log("坐标系统: " + rawData.metadata.coordinateSystem)
                console.log("单位: " + rawData.metadata.units)
                
                // 转换ECI坐标到场景坐标
                var convertedPoints = []
                for (var i = 0; i < rawData.points.length; i++) {
                    var p = rawData.points[i]
                    var scenePos = eciToScene(p.x, p.y, p.z)
                    convertedPoints.push({
                        time: p.time,
                        x: scenePos.x,
                        y: scenePos.y,
                        z: scenePos.z,
                        phase: p.phase,
                        eciX: p.x,
                        eciY: p.y,
                        eciZ: p.z
                    })
                }
                
                trajectoryData = {
                    metadata: rawData.metadata,
                    points: convertedPoints
                }
                
                console.log("坐标转换完成，距离比例: " + distanceScale)
                return true
            } catch (e) {
                console.log("解析轨迹数据失败: " + e.message)
                return false
            }
        } else {
            console.log("加载轨迹文件失败: " + xhr.status)
            return false
        }
    }
    
    // 从文件加载轨迹数据（场景坐标版本）
    function loadTrajectoryFromFile(filePath) {
        var xhr = new XMLHttpRequest()
        xhr.open("GET", filePath, false)
        xhr.send()
        
        if (xhr.status === 200 || xhr.status === 0) {
            try {
                trajectoryData = JSON.parse(xhr.responseText)
                console.log("轨迹数据加载成功: " + trajectoryData.metadata.name)
                console.log("总点数: " + trajectoryData.metadata.totalPoints)
                return true
            } catch (e) {
                console.log("解析轨迹数据失败: " + e.message)
                return false
            }
        } else {
            console.log("加载轨迹文件失败: " + xhr.status)
            return false
        }
    }
    
    // 获取轨迹点位置（场景坐标）
    function getTrajectoryPosition(time) {
        if (!trajectoryData || !trajectoryData.points) return null
        
        var points = trajectoryData.points
        for (var i = 0; i < points.length; i++) {
            if (points[i].time >= time) {
                return {
                    x: points[i].x,
                    y: points[i].y,
                    z: points[i].z,
                    phase: points[i].phase,
                    index: i
                }
            }
        }
        return null
    }
    
    // 获取轨迹点位置（真实ECI坐标，km）
    function getTrajectoryPositionECI(time) {
        if (!trajectoryData || !trajectoryData.points) return null
        
        var points = trajectoryData.points
        for (var i = 0; i < points.length; i++) {
            if (points[i].time >= time) {
                return {
                    x: points[i].eciX || points[i].x,
                    y: points[i].eciY || points[i].y,
                    z: points[i].eciZ || points[i].z,
                    phase: points[i].phase,
                    index: i
                }
            }
        }
        return null
    }
    
    function initControlPoints() {
        // 从C++侧加载的轨迹点列表填充
        if (TrajectoryLoader && TrajectoryLoader.points && TrajectoryLoader.points.length > 0) {
            var xs = [], ys = [], zs = [], names = [], colors = []
            var points = TrajectoryLoader.points
            
            // 颜色映射 - 统一白色
            var phaseColors = {
                "earth_orbit_1": "#FFFFFF",
                "earth_orbit_2": "#FFFFFF",
                "earth_orbit_3": "#FFFFFF",
                "transfer": "#FFFFFF",
                "moon_orbit_1": "#FFFFFF",
                "moon_orbit_2": "#FFFFFF",
                "moon_orbit_3": "#FFFFFF"
            }
            
            for (var i = 0; i < points.length; i++) {
                xs.push(points[i].x)
                ys.push(points[i].y)
                zs.push(points[i].z)
                names.push("P-" + (i + 1))
                colors.push("#FFFFFF")
            }
            
            controlPointX = xs
            controlPointY = ys
            controlPointZ = zs
            controlPointNames = names
            controlPointColors = colors
            rootItem.useTelemetryTrajectory = false  // 文件通道，与 $TRJ 通道互斥
            
            console.log("=== 从C++加载轨迹数据 ===")
            console.log("总点数: " + xs.length)
        }
    }
    
    // 默认控制点（备用）
    function initDefaultControlPoints() {
        var xs = [], ys = [], zs = [], names = [], colors = []
        
        // 场景坐标系说明：
        // - 地球中心在原点 (0, 0, 0)
        // - 月球中心在 (80000, 0, 0)
        // - 控制点使用世界坐标
        
        // 近地停车轨道 - 围绕地球原点的3圈椭圆（共同切点）
        // 目标：三圈椭圆逐渐变大，并在 -X 方向具有共同切点（近地点）
        // 参数化方式：以焦点在原点的椭圆（Kepler轨道），近地点 r_p 固定，改变离心率 e
        //   a = r_p / (1 - e)
        //   b = a * sqrt(1 - e^2)
        //   c = a * e
        // 近地点在 -X 方向，位置为 (-r_p, 0, 0)
        var earthRp = 12000
        var earthEList = [0.15, 0.35, 0.55]
        var earthColors = ["#FF6B6B", "#FF8888", "#FFAAAA"]
        for (var orbit = 0; orbit < 3; orbit++) {
            var eE = earthEList[orbit]
            var aE = earthRp / (1.0 - eE)
            var bE = aE * Math.sqrt(Math.max(0.0, 1.0 - eE * eE))
            var cE = aE * eE
            // 椭圆中心在 (+cE, 0)，焦点在原点
            // t=π 时：x = cE + aE*cos(π) = cE - aE = -aE*(1-eE) = -r_p（近地点在 -X）
            for (var i = 0; i < 12; i++) {
                var t = i * Math.PI / 6
                xs.push(cE + aE * Math.cos(t))
                ys.push(0)
                zs.push(bE * Math.sin(t))
                names.push("E" + (orbit + 1) + "-" + (i + 1))
                colors.push(earthColors[orbit])
            }
        }

        // 月球轨道参数（提前定义，供转移轨道使用）
        var moonCenterX = CoordinateSystem.moonSceneX
        var moonRp = 3200
        
        // 地月转移轨道 (36-59) - 大弧线连接地球切点和月球切点
        var transferCount = 24
        // 地月转移轨道：从地球共同切点出发，绕大弧到达月球共同切点
        // 地球切点在左侧（-X方向），月球切点在右侧（远离地球方向）
        // 起点：地球切点 (-earthRp, 0, 0)
        // 终点：月球切点 (moonCenterX + moonRp, 0, 0)
        var startX = -earthRp  // 地球左侧切点
        var endX = moonCenterX + moonRp  // 月球右侧切点（远离地球）
        
        // 大椭圆弧参数
        var midX = (startX + endX) / 2
        var halfDist = (endX - startX) / 2
        // 弧的高度（Z方向偏移），增大弧度使其不小于地球最大轨道弧度
        var arcHeight = halfDist * 0.8
        
        // 使用椭圆参数方程生成弧线
        var a = halfDist
        var b = arcHeight
        // 从 t=PI (起点，地球切点) 到 t=0 (终点，月球切点)
        // t=PI 时：x = midX - a = startX（地球切点）
        // t=0 时：x = midX + a = endX（月球切点）
        for (var k = 0; k < transferCount; k++) {
            var t = Math.PI - k * Math.PI / (transferCount - 1)
            var x = midX + a * Math.cos(t)
            var z = -b * Math.sin(t)  // 向-Z方向延伸（向外绕远路）
            xs.push(x)
            ys.push(0)
            zs.push(z)
            names.push("T-" + (k + 1))
            colors.push("#4ECDC4")
        }

        // 近月轨道 - 围绕月球中心的3圈椭圆（共同切点）
        // 目标：三圈椭圆逐渐变大，并在"远离地球"的方向具有共同切点
        // 月球在 (80000, 0, 0)，地球在原点，所以远离地球是月球坐标系的 +X 方向
        // 采用与地球相同的椭圆族生成方式：固定近地点 r_p，逐圈增大离心率 e
        //   a = r_p / (1 - e)
        //   b = a * sqrt(1 - e^2)
        //   c = a * e
        // 椭圆中心在 (-cM, 0)，t=0 时近地点在 +X 方向（远离地球）
        // 月球三圈：从大到小（外圈离心率更大），共同切点远离地球侧
        var moonEList = [0.55, 0.35, 0.15]
        var moonColors = ["#45B7D1", "#60C0E0", "#80D0F0"]
        for (var orbit = 0; orbit < 3; orbit++) {
            var eM = moonEList[orbit]
            var aM = moonRp / (1.0 - eM)
            var bM = aM * Math.sqrt(Math.max(0.0, 1.0 - eM * eM))
            var cM = aM * eM
            for (var j = 0; j < 12; j++) {
                var t2 = j * Math.PI / 6
                // t2=0 时：x = -cM + aM = aM(1-eM) = moonRp（近地点远离地球）
                var localX = -cM + aM * Math.cos(t2)
                var localZ = bM * Math.sin(t2)
                xs.push(moonCenterX + localX)
                ys.push(0)
                zs.push(localZ)
                names.push("M" + (orbit + 1) + "-" + (j + 1))
                colors.push(moonColors[orbit])
            }
        }
        
        controlPointX = xs
        controlPointY = ys
        controlPointZ = zs
        controlPointNames = names
        controlPointColors = colors
        rootItem.useTelemetryTrajectory = false  // 默认演示通道
    }

    // 1. Static 2D Background Image
    Image {
        anchors.fill: parent
        source: "qrc:/resources/images/space_background.png"
        fillMode: Image.PreserveAspectCrop
        z: -1
    }

    // 2. Loader for the entire View3D scene
    Loader {
        id: sceneLoader
        anchors.fill: parent
        asynchronous: true
        sourceComponent: view3DComponent
    }

    // 3. 3D Scene Component
    Component {
        id: view3DComponent
        
        View3D {
            id: view
            
            environment: SceneEnvironment {
                backgroundMode: SceneEnvironment.Transparent
                antialiasingMode: SceneEnvironment.MSAA
                antialiasingQuality: SceneEnvironment.High
            }

// Camera Pivot for Orbiting
                Node {
                    id: cameraPivot
                    
                    // 用户交互时直接修改，无动画
                    // 视角切换时通过 switchToMoonView/switchToEarthView 触发动画
                    x: rootItem.cameraTargetPosition.x
                    y: rootItem.cameraTargetPosition.y
                    z: rootItem.cameraTargetPosition.z

                    eulerRotation.x: rootItem.cameraRotationX
                    eulerRotation.y: rootItem.cameraRotationY
                    
                    PerspectiveCamera {
                        id: camera
                        
                        // 用户交互时直接修改，无动画
                        z: rootItem.cameraDistance

                        fieldOfView: 35
                        clipFar: 1000000
                        clipNear: 100
                    }
                }
            
            // Scene Content
            Node {
                // Lights
                DirectionalLight {
                    eulerRotation.x: -30
                    eulerRotation.y: -30
                    brightness: 1.2
                    castsShadow: true
                }

                DirectionalLight {
                    eulerRotation.x: 30
                    eulerRotation.y: 150
                    brightness: 0.4
                    color: "#CCCCFF"
                }
                
                DirectionalLight {
                    eulerRotation.x: 90
                    brightness: 0.6
                    color: "#FFFFFF"
                }

                // Earth
                EarthItem {
                    id: earth
                }

                // Moon
                Node {
                    id: moonPivot
                    x: CoordinateSystem.moonSceneX  // 月球摆位（空间2，非真实地月距离）
                    
                    // 月球本体（包含轨迹点）
                    MoonItem {
                        id: moonModel
                        lineWidthScale: rootItem.lineWidthScale
                        Component.onCompleted: {
                            rootItem.moonModelRef = moonModel
                        }
                    }
                }

                // Satellites
                SatelliteInstancer {
                    satelliteModel: SatelliteModel
                }

                // Orbits - 始终创建，通过 visible 控制显示（预加载优化）
                // TrajectoryLineGeometry：单节点交叉ribbon，全量渲染无跳点
                Repeater3D {
                    model: SatelliteModel ? SatelliteModel.orbitPaths : null
                    delegate: Model {
                        // 真实模式：环缓存是 ECI 采样，绕地轴转 -GMST 得地固朝向
                        // （与卫星 ECEF 位置重合，GMST 推进零重建）
                        eulerRotation.z: SatelliteModel.realOrbitMode
                                         ? -SatelliteModel.gmstDeg : 0
                        // 可见性 = 环数据非空 且 该卫星当前可见。
                        // 真实模式($ORB)下卫星按 health 升/降，绑定引用
                        // visibilityEpoch 使 health 翻转时仅重估本 delegate 的
                        // visible（零重建）；仿真模式下 epoch 恒定、由
                        // orbitPaths 重发驱动重建，行为不变。
                        visible: {
                            var hasRing = modelData && modelData.points
                                          && modelData.points.length > 0
                            if (!hasRing) return false
                            if (SatelliteModel.visibilityEpoch >= 0)
                                return SatelliteModel.isSatelliteVisible(index)
                            return true
                        }
                        geometry: TrajectoryLineGeometry {
                            points: modelData ? modelData.points : []
                            // 场景 1 单位 = 1 km（地球 #Sphere 基准半径 50 × scale 127.42）
                            // 线宽统一来自 CoordinateSystem，禁止手写字面量
                            thickness: (modelData && modelData.thickness !== undefined
                                        ? modelData.thickness : CoordinateSystem.orbitThicknessKm)
                                       * rootItem.lineWidthScale
                        }
                        materials: DefaultMaterial {
                            diffuseColor: modelData ? modelData.color : "transparent"
                            lighting: DefaultMaterial.NoLighting
                            cullMode: DefaultMaterial.NoCulling
                        }
                    }
                }

                // 轨迹线渲染：TrajectoryLineGeometry 全量渲染，
                // drawCount 跟随回放进度实现逐步绘制
                Model {
                    id: trajectoryLine
                    visible: rootItem.useTrajectoryFile && rootItem.drawProgress > 1
                    geometry: TrajectoryLineGeometry {
                        // 实时通道：直接绑定 C++ 批量场景点（已含压缩比，无 JS 循环）
                        // 文件通道：legacyTrajectoryPoints（点数少，不在推流热路径）
                        // 注意：不要写成 `cond ? A : { ... }`，块不是 JS 表达式，
                        // QML 会实例化失败 → 整个 3D 场景不渲染（曾踩过）
                        points: rootItem.useTelemetryTrajectory
                                ? PntDataModel.trajectory3DScenePoints
                                : rootItem.legacyTrajectoryPoints
                        // 回放轨迹点已乘 distanceScale（空间2），线宽同属空间2
                        thickness: CoordinateSystem.replayThicknessUnits * rootItem.lineWidthScale
                        drawCount: rootItem.drawProgress
                    }
                    materials: DefaultMaterial {
                        diffuseColor: "#FFFFFF"
                        lighting: DefaultMaterial.NoLighting
                        cullMode: DefaultMaterial.NoCulling
                    }
                }

                // 探测器小球：沿轨迹运动（实时通道用 C++ 单次返回的最新点）
                Node {
                    id: probeNode
                    visible: rootItem.useTrajectoryFile && rootItem.drawProgress > 0
                    property int idx: Math.max(0, Math.min(rootItem.drawProgress - 1, rootItem.controlPointX.length - 1))
                    x: rootItem.useTelemetryTrajectory ? rootItem.probePos.x
                                                       : (rootItem.controlPointX[idx] || 0)
                    y: rootItem.useTelemetryTrajectory ? rootItem.probePos.y
                                                       : (rootItem.controlPointY[idx] || 0)
                    z: rootItem.useTelemetryTrajectory ? rootItem.probePos.z
                                                       : (rootItem.controlPointZ[idx] || 0)

                    Model {
                        source: "#Sphere"
                        scale: Qt.vector3d(12, 12, 12)
                        materials: PrincipledMaterial {
                            baseColor: "#FFFFFF"
                            emissiveFactor: Qt.vector3d(0.2, 0.2, 0.2)
                        }
                    }
                }
            }
            
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                property point lastMousePos
                
                onPressed: function(mouse) { lastMousePos = Qt.point(mouse.x, mouse.y) }
                
                onPositionChanged: function(mouse) {
                    if (!pressed) return
                    var dx = mouse.x - lastMousePos.x
                    var dy = mouse.y - lastMousePos.y
                    if (mouse.buttons & Qt.LeftButton) {
                        // 直接修改 rootItem 属性（无 Behavior 动画，响应即时）
                        rootItem.cameraRotationY -= dx * 0.2
                        rootItem.cameraRotationX -= dy * 0.2
                        rootItem.cameraRotationX = Math.max(-85, Math.min(85, rootItem.cameraRotationX))
                    } else if (mouse.buttons & Qt.RightButton) {
                        var panSpeed = rootItem.cameraDistance * 0.001
                        rootItem.cameraTargetPosition.x -= dx * panSpeed
                        rootItem.cameraTargetPosition.y += dy * panSpeed
                    }
                    lastMousePos = Qt.point(mouse.x, mouse.y)
                }
                
                onWheel: function(wheel) {
                    // 直接修改 rootItem 属性（无 Behavior 动画，响应即时）
                    var zoomStep = rootItem.cameraDistance * 0.1
                    if (wheel.angleDelta.y > 0)
                        rootItem.cameraDistance -= zoomStep
                    else
                        rootItem.cameraDistance += zoomStep
                    rootItem.cameraDistance = Math.max(1000, Math.min(200000, rootItem.cameraDistance))
                }
            }
        }
    }

    // 4. Interactive UI Components（已拆分至 components/SceneViewOverlays.qml）
    SceneViewOverlays {
        anchors.fill: parent
        scene: rootItem
    }
}




