import QtQuick

Item {
    id: root
    clip: true

    // Debug overlay/logging for tuning zoom limits
    property bool debugZoom: false

    // ========== 瓦片覆盖边界限制 ==========
    // 世界地图范围：经度 -180~180，纬度约 -85~85 (WebMercator 限制)
    property real boundsWest: -180.0
    property real boundsEast: 180.0
    property real boundsSouth: -85.0
    property real boundsNorth: 85.0
    property bool enableBoundsCheck: true

    // Background for areas without tiles
    Rectangle {
        anchors.fill: parent
        color: "#0e2a47"  // 深海蓝：未铺瓦片的区域视觉上是海洋而非黑边
    }

    // WebMercator tile scheme
    property real centerLon: 0
    property real centerLat: 0
    property real zoomContinuous: 2.0
    property int minZoom: 0
    property int maxZoom: 10

    property string tileUrlTemplate: "image://mbtiles/{z}/{x}/{y}"

    // Interaction
    property bool dragging: false
    property real lastX: 0
    property real lastY: 0
    property real velocityX: 0
    property real velocityY: 0
    property bool _clampingZoom: false
    property bool _initialized: false

    function clampLat(lat) {
        return Math.max(-85, Math.min(85, lat));
    }

    function lonLatToTileXY(lon, lat, z) {
        const n = Math.pow(2, z);
        const x = (lon + 180.0) / 360.0 * n;
        const latRad = lat * Math.PI / 180.0;
        const y = (1.0 - Math.log(Math.tan(latRad) + 1.0 / Math.cos(latRad)) / Math.PI) / 2.0 * n;
        return { x: x, y: y };
    }

    function tileXYToLonLat(x, y, z) {
        const n = Math.pow(2, z);
        const lon = x / n * 360.0 - 180.0;
        const latRad = Math.atan(Math.sinh(Math.PI * (1 - 2 * y / n)));
        const lat = latRad * 180.0 / Math.PI;
        return { lon: lon, lat: lat };
    }

    function tileUrl(z, x, y) {
        var u = tileUrlTemplate
            .replace("{z}", z)
            .replace("{x}", x)
            .replace("{y}", y)
        if (u.indexOf(".") < 0) {
            u = u + ".png"
        }
        return u
    }

    readonly property int tileSize: 256
    readonly property real zoomClamped: Math.max(minZoom, Math.min(maxZoom + 0.999, zoomContinuous))
    readonly property int zInt: Math.floor(zoomClamped)
    readonly property real zFrac: zoomClamped - zInt
    readonly property real scaleFactor: Math.pow(2, zFrac)
    readonly property real scaledTileSize: tileSize * scaleFactor

    // compute center in tile coordinates
    readonly property var centerTile: lonLatToTileXY(centerLon, clampLat(centerLat), zInt)

    // number of tiles needed to fill viewport
    readonly property int tilesX: {
        const worldTilesAtZoom = Math.pow(2, zInt)
        const tilesNeeded = Math.ceil(width / scaledTileSize) + 2
        return Math.min(tilesNeeded, worldTilesAtZoom)
    }
    readonly property int tilesY: {
        const worldTilesAtZoom = Math.pow(2, zInt)
        const tilesNeeded = Math.ceil(height / scaledTileSize) + 2
        return Math.min(tilesNeeded, worldTilesAtZoom)
    }

    // top-left tile coordinate (float)
    readonly property real topLeftTileX: centerTile.x - (width / scaledTileSize) / 2
    readonly property real topLeftTileY: centerTile.y - (height / scaledTileSize) / 2

    function screenToLonLat(px, py) {
        const tx = topLeftTileX + (px / scaledTileSize)
        const ty = topLeftTileY + (py / scaledTileSize)
        return tileXYToLonLat(tx, ty, zInt)
    }

    function lonLatToScreenXY(lon, lat) {
        const t = lonLatToTileXY(lon, clampLat(lat), zInt)
        return {
            x: (t.x - topLeftTileX) * scaledTileSize,
            y: (t.y - topLeftTileY) * scaledTileSize
        }
    }

    // ========== 全球地图充满视口的最小缩放限制 ==========
    // WebMercator 世界是正方形瓦片网格，但经纬度范围是 360°×180°
    // 需要确保缩放后瓦片网格能覆盖整个视口
    readonly property real minZoomForWholeWorld: {
        if (width <= 0 || height <= 0) return 0
        // 世界地图是正方形，需要填满视口的最大尺寸
        return Math.max(0, Math.log2(Math.max(width, height) / tileSize))
    }

    // 边界限制：确保视口不超出世界地图范围
    function clampCenterToBounds(lon, lat) {
        if (!enableBoundsCheck) return { lon: lon, lat: clampLat(lat) }

        // 在最小缩放时，强制居中显示整个世界
        if (isAtMinZoom) {
            return { lon: 0, lat: 0 }
        }

        // 计算视口覆盖的经纬度范围
        const viewportLonDeg = (width / scaledTileSize) / Math.pow(2, zInt) * 360
        const viewportLatDeg = (height / scaledTileSize) / Math.pow(2, zInt) * 170  // WebMercator 纬度约 -85~85

        // 世界范围
        const worldLonDeg = 360
        const worldLatDeg = 170

        let clampedLon = lon
        let clampedLat = clampLat(lat)

        // 经度限制
        if (viewportLonDeg >= worldLonDeg) {
            clampedLon = 0
        } else {
            const maxLonOffset = (worldLonDeg - viewportLonDeg) / 2
            clampedLon = Math.max(-maxLonOffset, Math.min(maxLonOffset, lon))
        }

        // 纬度限制
        if (viewportLatDeg >= worldLatDeg) {
            clampedLat = 0
        } else {
            const maxLatOffset = (worldLatDeg - viewportLatDeg) / 2
            clampedLat = Math.max(-maxLatOffset, Math.min(maxLatOffset, lat))
        }

        return { lon: clampedLon, lat: clampLat(clampedLat) }
    }

    // 检查是否在最小缩放级别（无法再缩小）
    readonly property bool isAtMinZoom: zoomContinuous <= minZoomForWholeWorld + 0.05

    function setCenterFromTileXY(tileX, tileY) {
        const ll = tileXYToLonLat(tileX, tileY, zInt)
        const clamped = clampCenterToBounds(ll.lon, ll.lat)
        centerLon = clamped.lon
        centerLat = clamped.lat
    }

    // 初始化：等待布局完成后设置缩放
    Component.onCompleted: {
        _initView()
    }

    onWidthChanged: Qt.callLater(_refitZoom)
    onHeightChanged: Qt.callLater(_refitZoom)

    // 窗口尺寸变化后的重适配：全局最小缩放随视口增大而抬高。
    // 此前只有一次性的 _initView（_initialized 后直接返回），窗口从
    // 预加载尺寸(520x720)变到全屏后最小缩放仍停留在 1.49，世界地图
    // 盖不住变宽的视口，右侧露出大片底色
    function _refitZoom() {
        if (width <= 0 || height <= 0) return
        if (!_initialized) {
            _initView()
            return
        }
        const effMin = Math.max(minZoom, minZoomForWholeWorld)
        if (zoomContinuous < effMin - 1e-6) {
            zoomContinuous = effMin
            centerLon = 0
            centerLat = 0
        }
        Qt.callLater(_checkBounds)
    }

    function _initView() {
        if (width <= 0 || height <= 0) return
        if (_initialized) return

        zoomContinuous = minZoomForWholeWorld
        centerLon = 0
        centerLat = 0
        _initialized = true
        console.log("[TileMapView] Initialized: zoom=" + zoomContinuous.toFixed(2) +
                    " viewport=" + width + "x" + height +
                    " tiles=" + tilesX + "x" + tilesY)
    }

    onZoomContinuousChanged: {
        if (!_clampingZoom) {
            const effectiveMinZoom = Math.max(minZoom, minZoomForWholeWorld)
            const clamped = Math.max(effectiveMinZoom, Math.min(maxZoom + 0.999, zoomContinuous))
            if (Math.abs(clamped - zoomContinuous) > 1e-6) {
                _clampingZoom = true
                zoomContinuous = clamped
                _clampingZoom = false
                return
            }
        }
        Qt.callLater(_checkBounds)
    }

    onCenterLonChanged: Qt.callLater(_checkBounds)
    onCenterLatChanged: Qt.callLater(_checkBounds)

    property bool _checkingBounds: false
    function _checkBounds() {
        if (_checkingBounds || !enableBoundsCheck) return
        const clamped = clampCenterToBounds(centerLon, centerLat)
        if (Math.abs(clamped.lon - centerLon) > 0.001 || Math.abs(clamped.lat - centerLat) > 0.001) {
            _checkingBounds = true
            centerLon = clamped.lon
            centerLat = clamped.lat
            _checkingBounds = false
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 10
        radius: 6
        color: "#AA000000"
        visible: root.debugZoom

        Text {
            id: debugText
            padding: 8
            color: "white"
            font.pixelSize: 12
            text: "z=" + root.zoomContinuous.toFixed(3) +
                  " (zInt " + root.zInt + ", scale " + root.scaleFactor.toFixed(3) + ")\n" +
                  "center=(" + root.centerLon.toFixed(2) + ", " + root.centerLat.toFixed(2) + ")\n" +
                  "tiles=" + root.tilesX + "x" + root.tilesY +
                  "  minZ=" + root.minZoomForWholeWorld.toFixed(2) +
                  " atMin=" + root.isAtMinZoom
        }
        width: debugText.implicitWidth
        height: debugText.implicitHeight
    }

    Repeater {
        model: tilesX * tilesY
        delegate: Image {
            asynchronous: true
            cache: false
            fillMode: Image.Stretch
            smooth: true
            mipmap: true

            readonly property int ix: index % tilesX
            readonly property int iy: Math.floor(index / tilesX)

            readonly property int tx: Math.floor(topLeftTileX) + ix
            readonly property int ty: Math.floor(topLeftTileY) + iy

            // wrap x
            readonly property int n: Math.pow(2, zInt)
            readonly property int wx: ((tx % n) + n) % n

            // y clamp
            readonly property bool yValid: ty >= 0 && ty < n

            width: scaledTileSize
            height: scaledTileSize

            sourceSize.width: tileSize
            sourceSize.height: tileSize

            x: (tx - topLeftTileX) * scaledTileSize
            y: (ty - topLeftTileY) * scaledTileSize

            source: yValid ? tileUrl(zInt, wx, ty) : ""
            visible: yValid
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPressed: (mouse) => {
            dragging = true
            lastX = mouse.x
            lastY = mouse.y
            velocityX = 0
            velocityY = 0
        }
        onPositionChanged: (mouse) => {
            if (!dragging) return
            const dx = mouse.x - lastX
            const dy = mouse.y - lastY

            const deltaTilesX = -dx / scaledTileSize
            const deltaTilesY = -dy / scaledTileSize
            setCenterFromTileXY(centerTile.x + deltaTilesX, centerTile.y + deltaTilesY)

            velocityX = dx
            velocityY = dy
            lastX = mouse.x
            lastY = mouse.y
        }
        onReleased: {
            dragging = false
        }
        onWheel: (wheel) => {
            const mouseLonLat = screenToLonLat(wheel.x, wheel.y)
            const delta = wheel.angleDelta.y > 0 ? 0.25 : -0.25
            const effectiveMinZoom = Math.max(minZoom, minZoomForWholeWorld)
            const clampedZoom = Math.max(effectiveMinZoom, Math.min(maxZoom, zoomContinuous + delta))
            zoomContinuous = clampedZoom

            const newZInt = Math.max(minZoom, Math.min(maxZoom, Math.floor(zoomContinuous)))
            const newScale = Math.pow(2, zoomContinuous - newZInt)
            const newScaledTile = tileSize * newScale

            const t = lonLatToTileXY(mouseLonLat.lon, clampLat(mouseLonLat.lat), newZInt)
            const newTopLeftX = t.x - (wheel.x / newScaledTile)
            const newTopLeftY = t.y - (wheel.y / newScaledTile)
            const newCenterX = newTopLeftX + (width / newScaledTile) / 2
            const newCenterY = newTopLeftY + (height / newScaledTile) / 2
            const ll = tileXYToLonLat(newCenterX, newCenterY, newZInt)

            const bounded = clampCenterToBounds(ll.lon, ll.lat)
            centerLon = bounded.lon
            centerLat = bounded.lat

            wheel.accepted = true
        }
    }
}
