import QtQuick
import QtQuick3D
import NavModels

/**
 * 地球模型 - 包含地球本体和地面标记点
 *
 * 地面标记点显示方法：
 * 1. C++ 侧：SatelliteModel::groundPoints() 返回 QVariantList
 * 2. 使用 OrbitalPhysics::llaToCartesian(lat, lon, 6391.0) 计算位置，单位 km
 * 3. QML 侧：Repeater3D + GroundWarningItem 渲染
 * 4. GroundWarningItem 会自动处理 km 到场景单位的转换
 *
 * 月球轨迹点使用完全相同的方法，参见 MoonItem.qml
 */
Node {
    id: earthNode

    // Main Earth Body
    Model {
        id: earthSphere
        source: "#Sphere"
        materials: PrincipledMaterial {
            baseColor: "#ffffff"

            baseColorMap: Texture {
                id: earthTexture
                source: "qrc:/resources/images/earth_color.jpg"
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
            }

            emissiveMap: earthTexture
            emissiveFactor: Qt.vector3d(0.15, 0.15, 0.15)

            metalness: 0.0
            roughness: 0.6
            specularAmount: 0.5
        }
        scale: Qt.vector3d(127.42, 127.42, 127.42) // 直径 12742km / 100 = 127.42
    }

    // 地面标记点（与月球轨迹点使用相同方法）
    Node {
        eulerRotation.y: earthSphere.eulerRotation.y

        // 使用 Repeater3D 统一管理所有点位效果
        Repeater3D {
            model: SatelliteModel ? SatelliteModel.groundPoints : []
            delegate: GroundWarningItem {
                // 干扰点（type=1）只在欺骗卫星信号激活时显示
                visible: modelData.type !== 1 || (PntDataModel && PntDataModel.jammerActive)
                anchorPosition: modelData.position
                color: modelData.color
                type: modelData.type
                radius: modelData.radius
                errorRadius: modelData.errorRadius || 0
            }
        }
    }

    // Atmosphere Glow Layer
    Model {
        id: atmosphere
        source: "#Sphere"
        scale: Qt.vector3d(128.42, 128.42, 128.42) // 直径 12842km / 100 = 128.42
        materials: PrincipledMaterial {
            baseColor: Qt.rgba(0.5, 0.8, 1.0, 0.3)
            alphaMode: PrincipledMaterial.Blend
            opacity: 0.3
            depthDrawMode: PrincipledMaterial.NeverDepthDraw
            lighting: PrincipledMaterial.NoLighting
        }
    }
}