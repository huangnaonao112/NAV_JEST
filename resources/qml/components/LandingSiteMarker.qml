import QtQuick
import QtQuick3D

// 冯·卡门撞击坑着陆点标记 - 嫦娥四号着陆点
// 坐标: 45.444°S, 177.356°E (月球背面)
Node {
    id: root

    // 冯·卡门撞击坑坐标转换
    // 使用 OrbitalPhysics.llaToCartesian 公式:
    // x = r * cos(lat) * sin(lon)
    // y = r * sin(lat)
    // z = r * cos(lat) * cos(lon)
    // 计算: lat=-45.444°, lon=177.356°, radius=1737
    // 结果: (56.1, -1217.5, -1215.4) + 月球偏移(80000, 0, 0)
    // 最终位置: (80056.1, -1217.5, -1215.4)
    position: Qt.vector3d(80056.1, -1217.5, -1215.4)

    // 红色球体标记
    Model {
        id: markerSphere
        source: "#Sphere"
        scale: Qt.vector3d(50, 50, 50)
        materials: PrincipledMaterial {
            baseColor: "#FF0000"
            emissiveFactor: Qt.vector3d(1, 0, 0)
            roughness: 0.3
        }
    }

    // 标签文本
    Text {
        id: labelText
        text: "冯·卡门撞击坑\n嫦娥四号着陆点"
        color: "#FFFFFF"
        font.pixelSize: 14
        font.bold: true
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: -60
    }

    Component.onCompleted: {
        console.log("[LandingSiteMarker] Created at:", 
                    root.position.x.toFixed(1),
                    root.position.y.toFixed(1), 
                    root.position.z.toFixed(1))
    }
}