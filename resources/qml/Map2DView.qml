import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import "components"
import NavModels

Item {
    id: map2DRoot
    // 显式锚定：依赖 Loader 的自动尺寸传递在窗口 resize 后不可靠，
    // 曾致瓦片画布停留在旧宽度（右侧露出底色，"初始瓦片显示不完整"）
    anchors.fill: parent
    clip: true

    // =========================================================================
    // 状态属性
    // =========================================================================
    // Offline tiles use WebMercator continuous zoom; zoomLevel is derived for legacy UI scaling.
    property real baseTileZoom: 9.0
    // zoomLevel 基于 TileMapView 的当前缩放（安全访问）
    readonly property real zoomLevel: (tileMap && tileMap.visible) ? Math.pow(2.0, tileMap.zoomContinuous - baseTileZoom) : 1.0
    // Default view: global center when switching to 2D.
    property real defaultCenterLon: 0
    property real defaultCenterLat: 0
    property bool recenterOnShow: false

    property real centerLon: defaultCenterLon
    property real centerLat: defaultCenterLat
    property url mapSource: "../images/earth_color.jpg"
    property bool useOfflineTiles: true
    property string tileUrlTemplate: "image://mbtiles/{z}/{x}/{y}"

    onVisibleChanged: {
        if (!visible) return
        if (recenterOnShow) {
            centerLon = defaultCenterLon
            centerLat = defaultCenterLat
        }
    }

    // 惯性相关
    property real velocityLon: 0
    property real velocityLat: 0
    property bool isDragging: false

    // =========================================================================
    // 布局计算 (核心逻辑 - 修复比例失真)
    // =========================================================================
    
    // 核心：计算不失真的基础尺寸 (始终保持 2:1)
    // baseWidth 至少要能覆盖屏幕宽度，且对应的 baseHeight 也要能覆盖屏幕高度
    readonly property real baseWidth: Math.max(width, height * 2)
    readonly property real baseHeight: baseWidth / 2

    // 当前地图内容的实际显示宽度和高度
    readonly property real contentWidth: baseWidth * zoomLevel
    readonly property real contentHeight: baseHeight * zoomLevel

    // 归一化坐标映射
    function normalizeLon(lon) {
        var n = (lon + 180) % 360;
        if (n < 0) n += 360;
        return n / 360.0;
    }
    
    function normalizeLat(lat) {
        // 纬度 [-90, 90] -> [0, 1]
        return (90 - lat) / 180.0;
    }

    // 核心：计算 y 轴限制，防止地图上下出现黑边
    // 要使地图铺满屏幕，y 必须满足：y <= 0 且 y + contentHeight >= height
    function clampLat(lat, zoom) {
        // 计算当前缩放级别下，允许的最大纬度偏移
        // y = H/2 - normLat * H * zoom
        // 1. H/2 - normLat * H * zoom <= 0  => normLat >= 1 / (2 * zoom)
        // 2. H/2 - normLat * H * zoom + H * zoom >= H => normLat <= 1 - 1 / (2 * zoom)
        
        var minNorm = 1.0 / (2.0 * zoom);
        var maxNorm = 1.0 - minNorm;
        
        // 将 normLat 限制在 [minNorm, maxNorm]
        var currentNorm = normalizeLat(lat);
        var clampedNorm = Math.max(minNorm, Math.min(maxNorm, currentNorm));
        
        // 转回纬度: norm = (90 - lat) / 180 => lat = 90 - norm * 180
        return 90 - clampedNorm * 180;
    }

    // =========================================================================
    // 惯性模拟
    // =========================================================================
    Timer {
        id: inertiaTimer
        interval: 16
        repeat: true
        running: !map2DRoot.useOfflineTiles && !isDragging && (Math.abs(velocityLon) > 0.01 || Math.abs(velocityLat) > 0.01)
        onTriggered: {
            map2DRoot.centerLon -= velocityLon;
            map2DRoot.centerLat += velocityLat;
            
            // 阻尼
            velocityLon *= 0.95;
            velocityLat *= 0.95;
            
            // 修正
            while (map2DRoot.centerLon > 180) map2DRoot.centerLon -= 360;
            while (map2DRoot.centerLon < -180) map2DRoot.centerLon += 360;
            map2DRoot.centerLat = clampLat(map2DRoot.centerLat, map2DRoot.zoomLevel);
        }
    }

    // =========================================================================
    // 视图渲染层
    // =========================================================================
    
    // 1. 地图底层
    Item {
        id: mapEffectViewport
        anchors.fill: parent
        clip: true

        // 离线瓦片地图优先，否则回退到旧的整图模式
        TileMapView {
            id: tileMap
            anchors.fill: parent
            visible: map2DRoot.useOfflineTiles
            // 初始中心点（单向绑定）
            centerLon: map2DRoot.centerLon
            centerLat: map2DRoot.centerLat
            // 不绑定 zoomContinuous，让 TileMapView 自己管理
            minZoom: 0
            maxZoom: 15
            tileUrlTemplate: map2DRoot.tileUrlTemplate

            // 边界限制使用 TileMapView 默认值（全球范围）
            enableBoundsCheck: true

            // 只在用户交互时同步回父组件
            onCenterLonChanged: if (dragging) map2DRoot.centerLon = centerLon
            onCenterLatChanged: if (dragging) map2DRoot.centerLat = centerLat
        }

        // 回退：原始整图渲染（保留）
        Item {
            visible: !map2DRoot.useOfflineTiles
            anchors.fill: parent

            // 关键：将 layer 特效应用在固定大小的视口上，而不是随缩放变大的地图上
            // 这样 layer 的纹理尺寸始终等于屏幕尺寸，不会超过硬件限制 (16384)
            layer.enabled: true
            layer.effect: Colorize {
                hue: 0.6
                saturation: 0.5
                lightness: -0.25
            }

            Item {
                id: mapContentContainer
                width: map2DRoot.contentWidth
                height: map2DRoot.contentHeight
                x: worldContainer.x
                y: worldContainer.y

                Row {
                    id: mapRow
                    anchors.verticalCenter: parent.verticalCenter
                    height: parent.height
                    x: -parent.width

                    Repeater {
                        model: 3
                        Image {
                            width: mapContentContainer.width
                            height: mapContentContainer.height
                            source: map2DRoot.mapSource
                            fillMode: Image.Stretch
                            asynchronous: true
                        }
                    }
                }

                Item {
                    id: gridLayer
                    anchors.fill: parent
                    opacity: 0.1

                    Repeater {
                        model: 7
                        Rectangle {
                            width: parent.width; height: 1; color: "#00FFFF"
                            y: (index / 6.0) * parent.height
                        }
                    }
                    Repeater {
                        model: 13
                        Rectangle {
                            width: 1; height: parent.height; color: "#00FFFF"
                            x: (index / 12.0) * parent.width
                        }
                    }
                }
            }
        }
    }

    // 2. 覆盖层 (PNT位置点)
    Item {
        id: worldContainer
        anchors.fill: parent
        
        visible: map2DRoot.useOfflineTiles

        // ========== 轨迹线绘制 ==========
        Canvas {
            id: trajectoryCanvas
            anchors.fill: parent
            
            // 渲染优化
            renderTarget: Canvas.FramebufferObject
            renderStrategy: Canvas.Threaded
            
            property int simPointCount: 0
            property int dutPointCount: 0
            
            // 监听地图变化，触发重绘
            Connections {
                target: tileMap
                enabled: map2DRoot.visible
                function onCenterLonChanged() { trajectoryCanvas.requestPaint() }
                function onCenterLatChanged() { trajectoryCanvas.requestPaint() }
                function onZoomContinuousChanged() { trajectoryCanvas.requestPaint() }
            }
            
            // 监听PntDataModel的2D轨迹变化信号
            Connections {
                target: PntDataModel
                enabled: map2DRoot.visible
                function onTrajectoryChanged() {
                    trajectoryCanvas.simPointCount = PntDataModel ? PntDataModel.trajectoryCount : 0
                    trajectoryCanvas.requestPaint()
                }
                function onDutTrajectoryChanged() {
                    trajectoryCanvas.dutPointCount = PntDataModel ? PntDataModel.dutTrajectoryCount : 0
                    trajectoryCanvas.requestPaint()
                }
            }
            
            onPaint: {
                var ctx = getContext("2d");
                ctx.clearRect(0, 0, width, height);
                
                if (!PntDataModel) return;
                
                // ========== 模拟源轨迹（绿色）==========
                if (simPointCount >= 2) {
                    ctx.strokeStyle = "#22C55E";
                    ctx.lineWidth = 2;
                    ctx.lineCap = "round";
                    ctx.lineJoin = "round";
                    
                    ctx.beginPath();
                    for (var i = 0; i < simPointCount; i++) {
                        var lat = PntDataModel.trajectoryLat(i);
                        var lon = PntDataModel.trajectoryLon(i);
                        var pt = tileMap.lonLatToScreenXY(lon, lat);
                        
                        if (i === 0) {
                            ctx.moveTo(pt.x, pt.y);
                        } else {
                            ctx.lineTo(pt.x, pt.y);
                        }
                    }
                    ctx.stroke();
                    
                    // 绘制轨迹点
                    ctx.fillStyle = "#22C55E";
                    for (var j = 0; j < simPointCount; j++) {
                        var lat2 = PntDataModel.trajectoryLat(j);
                        var lon2 = PntDataModel.trajectoryLon(j);
                        var pt2 = tileMap.lonLatToScreenXY(lon2, lat2);
                        ctx.beginPath();
                        ctx.arc(pt2.x, pt2.y, 3, 0, Math.PI * 2);
                        ctx.fill();
                    }
                }
                
                // ========== 待测设备轨迹（蓝色）==========
                if (dutPointCount >= 2) {
                    ctx.strokeStyle = "#3B82F6";
                    ctx.lineWidth = 2;
                    ctx.lineCap = "round";
                    ctx.lineJoin = "round";
                    
                    ctx.beginPath();
                    for (var k = 0; k < dutPointCount; k++) {
                        var lat3 = PntDataModel.dutTrajectoryLat(k);
                        var lon3 = PntDataModel.dutTrajectoryLon(k);
                        var pt3 = tileMap.lonLatToScreenXY(lon3, lat3);
                        
                        if (k === 0) {
                            ctx.moveTo(pt3.x, pt3.y);
                        } else {
                            ctx.lineTo(pt3.x, pt3.y);
                        }
                    }
                    ctx.stroke();
                    
                    // 绘制轨迹点
                    ctx.fillStyle = "#3B82F6";
                    for (var m = 0; m < dutPointCount; m++) {
                        var lat4 = PntDataModel.dutTrajectoryLat(m);
                        var lon4 = PntDataModel.dutTrajectoryLon(m);
                        var pt4 = tileMap.lonLatToScreenXY(lon4, lat4);
                        ctx.beginPath();
                        ctx.arc(pt4.x, pt4.y, 3, 0, Math.PI * 2);
                        ctx.fill();
                    }
                }
            }
        }

        // ========== PNT位置点显示 ==========
        // 模拟源位置点 (绿色) - 无数据时隐藏
        Item {
            id: simSourcePoint
            visible: PntDataModel && PntDataModel.hasSimData
            readonly property var screenPt: tileMap.lonLatToScreenXY(PntDataModel ? PntDataModel.simLongitude : 0, PntDataModel ? PntDataModel.simLatitude : 0)
            x: screenPt.x
            y: screenPt.y
            
            // 1. 基础圆点 (带呼吸闪烁)
            Rectangle {
                width: 12; height: 12; radius: 6; x: -6; y: -6
                color: "#22C55E"
                border.color: "white"; border.width: 1.5
                scale: Math.min(1.5, Math.max(1.0, map2DRoot.zoomLevel * 0.15))
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.45; duration: 650; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.45; to: 1.0; duration: 650; easing.type: Easing.InOutQuad }
                }
            }

            // 2. 扩散动画
            Rectangle {
                width: 30; height: 30; radius: 15; x: -15; y: -15
                color: "transparent"
                border.color: "#22C55E"
                border.width: 2
                scale: Math.min(1.5, Math.max(1.0, map2DRoot.zoomLevel * 0.15))
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.9; to: 0.25; duration: 900; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.25; to: 0.9; duration: 900; easing.type: Easing.InOutQuad }
                }
            }

            // 3. 波纹效果
            Rectangle {
                id: simWave
                width: 60; height: 60; radius: 30; x: -30; y: -30
                color: "transparent"
                border.color: "#22C55E"
                border.width: 2
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 0.6; duration: 500 }
                    NumberAnimation { from: 0.6; to: 0; duration: 1500 }
                }
                NumberAnimation on scale {
                    from: 0.1; to: 1.0; duration: 2000; loops: Animation.Infinite
                }
            }
        }
        
        // 待测设备位置点 (蓝色) - 无数据时隐藏
        Item {
            id: dutPoint
            visible: PntDataModel && PntDataModel.hasDutData
            readonly property var screenPt: tileMap.lonLatToScreenXY(PntDataModel ? PntDataModel.dutLongitude : 0, PntDataModel ? PntDataModel.dutLatitude : 0)
            x: screenPt.x
            y: screenPt.y
            
            // 1. 基础圆点 (带呼吸闪烁)
            Rectangle {
                width: 12; height: 12; radius: 6; x: -6; y: -6
                color: "#3B82F6"
                border.color: "white"; border.width: 1.5
                scale: Math.min(1.5, Math.max(1.0, map2DRoot.zoomLevel * 0.15))
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.45; duration: 650; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.45; to: 1.0; duration: 650; easing.type: Easing.InOutQuad }
                }
            }

            // 2. 扩散动画
            Rectangle {
                width: 30; height: 30; radius: 15; x: -15; y: -15
                color: "transparent"
                border.color: "#3B82F6"
                border.width: 2
                scale: Math.min(1.5, Math.max(1.0, map2DRoot.zoomLevel * 0.15))
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.9; to: 0.25; duration: 900; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.25; to: 0.9; duration: 900; easing.type: Easing.InOutQuad }
                }
            }

            // 3. 波纹效果
            Rectangle {
                id: dutWave
                width: 60; height: 60; radius: 30; x: -30; y: -30
                color: "transparent"
                border.color: "#3B82F6"
                border.width: 2
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 0.6; duration: 500 }
                    NumberAnimation { from: 0.6; to: 0; duration: 1500 }
                }
                NumberAnimation on scale {
                    from: 0.1; to: 1.0; duration: 2000; loops: Animation.Infinite
                }
            }
        }

        // ========== SAT 卫星星下点显示 ==========
        Repeater {
            id: satRepeater
            model: (PntDataModel && map2DRoot.visible && PntDataModel.satCount) ? PntDataModel.satCount : 0
            delegate: Item {
                id: satMarker
                property string prn: PntDataModel ? PntDataModel.satPrn(index) : ""
                property real satLat: PntDataModel ? PntDataModel.satLat(index) : 0
                property real satLon: PntDataModel ? PntDataModel.satLon(index) : 0
                readonly property var screenPt: tileMap.lonLatToScreenXY(satLon, satLat)
                x: screenPt.x
                y: screenPt.y
                visible: prn.length > 0

                readonly property color fillColor: (prn.length > 0 && prn[0] === "G") ? "#2563EB" : "#EF4444"

                Rectangle {
                    width: 26; height: 26; radius: 13; x: -13; y: -13
                    color: satMarker.fillColor
                    border.color: "white"
                    border.width: 1.5
                    opacity: 0.95
                }

                Text {
                    anchors.centerIn: parent
                    text: satMarker.prn
                    color: "white"
                    font.pixelSize: 12
                    font.bold: true
                }
            }
        }

        // ========== 干扰机显示（从报文获取动态数据）==========
        Item {
            id: jammerPoint
            visible: PntDataModel !== null && PntDataModel.jammerActive

            // 从报文获取动态坐标和数据
            readonly property real jammerLon: PntDataModel ? PntDataModel.jammerLongitude : 0
            readonly property real jammerLat: PntDataModel ? PntDataModel.jammerLatitude : 0
            readonly property real jammerRadiusKm: PntDataModel ? PntDataModel.jammerRadius : 0

            // 根据缩放级别计算屏幕上的干扰范围半径（像素）
            readonly property real radiusPixels: {
                var degreesPerPixel = 360.0 / (tileMap.tileSize * Math.pow(2, tileMap.zoomContinuous))
                var kmPerDegree = 111.0
                return jammerRadiusKm / (kmPerDegree * degreesPerPixel)
            }

            readonly property var screenPt: tileMap.lonLatToScreenXY(jammerLon, jammerLat)
            x: screenPt.x
            y: screenPt.y

            property bool hovered: false

            // 悬停检测区域 - 放在最上层
            MouseArea {
                id: jammerHoverArea
                anchors.centerIn: parent
                width: 60
                height: 60
                hoverEnabled: true
                acceptedButtons: Qt.NoButton
                onContainsMouseChanged: {
                    jammerPoint.hovered = containsMouse
                }
            }

            // 1. 中心红点
            Rectangle {
                width: 16; height: 16; radius: 8; x: -8; y: -8
                color: "#FF3333"
                border.color: "white"; border.width: 2
                scale: Math.min(1.5, Math.max(1.0, map2DRoot.zoomLevel * 0.15))
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.5; duration: 400; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 0.5; to: 1.0; duration: 400; easing.type: Easing.InOutQuad }
                }
            }

            // 2. 第一层波纹
            Rectangle {
                id: jammerWave1
                width: 40; height: 40; radius: 20; x: -20; y: -20
                color: "transparent"
                border.color: "#FF6666"
                border.width: 2
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 0.8; duration: 300 }
                    NumberAnimation { from: 0.8; to: 0; duration: 700 }
                }
                NumberAnimation on scale {
                    from: 0.5; to: 2.0; duration: 1000; loops: Animation.Infinite
                }
            }

            // 3. 第二层波纹
            Rectangle {
                id: jammerWave2
                width: 40; height: 40; radius: 20; x: -20; y: -20
                color: "transparent"
                border.color: "#FF9999"
                border.width: 2
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 0.6; duration: 300 }
                    PauseAnimation { duration: 500 }
                    NumberAnimation { from: 0.6; to: 0; duration: 700 }
                }
                NumberAnimation on scale {
                    from: 0.5; to: 2.5; duration: 1200; loops: Animation.Infinite
                }
            }

            // 干扰范围圆圈 - 动态大小
            Rectangle {
                id: jammerRadiusCircle
                width: jammerPoint.radiusPixels * 2
                height: jammerPoint.radiusPixels * 2
                radius: jammerPoint.radiusPixels
                x: -jammerPoint.radiusPixels
                y: -jammerPoint.radiusPixels
                color: "transparent"
                border.color: "#FF6666"
                border.width: 2
                opacity: 0.5
            }

            // 悬停信息弹窗
            Rectangle {
                visible: jammerPoint.hovered
                x: 25
                y: -90
                width: 180
                height: 110
                radius: 8
                color: Qt.rgba(0, 0, 0, 0.9)
                border.color: "#FF6666"
                border.width: 1
                z: 500

                Column {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 3

                    Text {
                        text: "干扰部署"
                        color: "#FF6666"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    Text {
                        text: "经度  " + jammerPoint.jammerLon.toFixed(4)
                        color: "white"
                        font.pixelSize: 11
                    }

                    Text {
                        text: "纬度  " + jammerPoint.jammerLat.toFixed(4)
                        color: "white"
                        font.pixelSize: 11
                    }

                    Text {
                        text: "干扰范围  " + jammerPoint.jammerRadiusKm.toFixed(1) + " km"
                        color: "white"
                        font.pixelSize: 11
                    }

                    Text {
                        text: "干扰方向  " + (PntDataModel ? PntDataModel.jammerDirection.toFixed(1) : "0.0") + "° (地心)"
                        color: "#FFAA66"
                        font.pixelSize: 11
                    }

                }
            }
        }
    }

    // 3. 2D 交互配置（已移除悬浮窗：浮球 + 配置面板）

    // 3.5 位置信息卡片已移至 AppShell（定位到用户图标下方），2D 地图内不再显示

    // =========================================================================
    // 公共 API：定位到模拟源位置
    // =========================================================================
    
    /**
     * 定位到模拟源位置并放大到最大瓦片级别
     * 如果模拟源坐标有效，则设置地图中心点并缩放到最大级别
     */
    // 定位到指定经纬度并设定缩放（自动化验证/通用定位入口）
    function locateTo(lon, lat, zoom) {
        if (isNaN(lon) || isNaN(lat)) return false
        tileMap.centerLon = lon
        tileMap.centerLat = lat
        if (!isNaN(zoom) && zoom > 0)
            tileMap.zoomContinuous = zoom
        if (typeof SceneLogger !== "undefined")
            SceneLogger.logQml("MAP2D", "locateTo " + lon + "," + lat
                               + " zoom=" + tileMap.zoomContinuous)
        return true
    }

    function locateToSimPoint() {
        if (!PntDataModel) {
            console.log("[Map2DView] locateToSimPoint: PntDataModel is null")
            return false
        }
        
        var targetLon = PntDataModel.simLongitude
        var targetLat = PntDataModel.simLatitude
        
        // 检查坐标有效性（经度 -180~180，纬度 -90~90）
        if (isNaN(targetLon) || isNaN(targetLat) || 
            targetLon < -180 || targetLon > 180 ||
            targetLat < -90 || targetLat > 90) {
            console.log("[Map2DView] locateToSimPoint: invalid coordinates", targetLon, targetLat)
            return false
        }
        
        // 设置中心点
        tileMap.centerLon = targetLon
        tileMap.centerLat = targetLat
        
        // 放大到最大瓦片级别
        tileMap.zoomContinuous = tileMap.maxZoom
        
        if (typeof SceneLogger !== "undefined")
            SceneLogger.logQml("MAP2D", "locateToSimPoint centered to " + targetLon + "," + targetLat
                               + " zoom=" + tileMap.maxZoom + " tileCenter=" + tileMap.centerLon + "," + tileMap.centerLat)
        return true
    }

    // =========================================================================
    // 交互逻辑
    // =========================================================================
    MouseArea {
        id: mouseAreaGlobal
        anchors.fill: parent
        enabled: !map2DRoot.useOfflineTiles
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        
        property real lastX: 0
        property real lastY: 0
        property real lastTime: 0

        onPressed: (mouse) => {
            map2DRoot.isDragging = true;
            lastX = mouse.x;
            lastY = mouse.y;
            lastTime = Date.now();
            velocityLon = 0;
            velocityLat = 0;
            inertiaTimer.stop();
        }

        onPositionChanged: (mouse) => {
            if (pressed) {
                var dx = mouse.x - lastX;
                var dy = mouse.y - lastY;
                var now = Date.now();
                var dt = Math.max(1, now - lastTime);

                var dLon = (dx / map2DRoot.contentWidth) * 360;
                var dLat = (dy / map2DRoot.contentHeight) * 180;

                map2DRoot.centerLon -= dLon;
                map2DRoot.centerLat += dLat;

                velocityLon = dLon / (dt / 16);
                velocityLat = dLat / (dt / 16);

                while (map2DRoot.centerLon > 180) map2DRoot.centerLon -= 360;
                while (map2DRoot.centerLon < -180) map2DRoot.centerLon += 360;
                map2DRoot.centerLat = clampLat(map2DRoot.centerLat, map2DRoot.zoomLevel);

                lastX = mouse.x;
                lastY = mouse.y;
                lastTime = now;
            }
        }

        onReleased: {
            map2DRoot.isDragging = false;
        }

        onDoubleClicked: (mouse) => {
            zoomTo(mouse.x, mouse.y, map2DRoot.zoomLevel * 2.0);
        }

        onWheel: (wheel) => {
            var factor = wheel.angleDelta.y > 0 ? 1.2 : 0.8;
            zoomTo(wheel.x, wheel.y, map2DRoot.zoomLevel * factor);
        }

        function zoomTo(mouseX, mouseY, targetZoom) {
            targetZoom = Math.max(1.0, Math.min(50.0, targetZoom));
            
            var oldZoom = map2DRoot.zoomLevel;
            var dx = mouseX - map2DRoot.width / 2;
            var dy = mouseY - map2DRoot.height / 2;
            
            var lonOffset = (dx / (map2DRoot.baseWidth * oldZoom)) * 360;
            var latOffset = (dy / (map2DRoot.baseHeight * oldZoom)) * 180;
            
            // zoomLevel 是只读派生属性（基于 tileMap.zoomContinuous），
            // 直接赋值会静默失败——此前滚轮缩放实际未生效
            if (tileMap) tileMap.zoomContinuous = Math.log2(targetZoom) + map2DRoot.baseTileZoom;
            map2DRoot.centerLon += lonOffset * (1 - oldZoom / targetZoom);
            map2DRoot.centerLat -= latOffset * (1 - oldZoom / targetZoom);
            
            while (map2DRoot.centerLon > 180) map2DRoot.centerLon -= 360;
            while (map2DRoot.centerLon < -180) map2DRoot.centerLon += 360;
            map2DRoot.centerLat = clampLat(map2DRoot.centerLat, map2DRoot.zoomLevel);
        }
    }
}
