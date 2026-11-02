import QtQuick
import QtQuick3D
import NavModels

Node {
    id: root

    // Required property injected from outside
    required property var satelliteModel

    // 场景相机引用（SceneViewItem 注入）：用于按相机距离做标记尺寸补偿
    property var sceneCamera: null

    // 获取可执行文件目录，构建文件系统路径（不依赖 qrc）
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property string modelDir: "file:///" + appDir + "models/"

    // 干扰目标位置：北京附近 (116.4°E, 39.9°N)
    readonly property vector3d jammerTargetPos: {
        // 使用 OrbitalPhysics 的坐标转换逻辑
        var latRad = 39.9 * Math.PI / 180.0
        var lonRad = 116.4 * Math.PI / 180.0
        var earthRadius = 6371.0
        return Qt.vector3d(
            earthRadius * Math.cos(latRad) * Math.sin(lonRad),
            earthRadius * Math.sin(latRad),
            earthRadius * Math.cos(latRad) * Math.cos(lonRad)
        )
    }

    // 卫星渲染：使用 Repeater3D
    Repeater3D {
        model: root.satelliteModel
        delegate: Node {
            id: satNode
            position: model.position
            eulerRotation: model.eulerRotation

            // ===== 标记尺寸：屏幕准恒定（部分补偿）=====
            // 卫星是符号化标记；透视会让近轨掠过相机时巨大、远轨（GEO 背面）
            // 几乎不可见。按"相机到卫星的距离"做幂次补偿（指数<1 保留深度感）：
            //   distScale = clamp((camDist/REF)^EXP, MIN, MAX)
            // 屏幕视角 ≈ scale/dist，GEO 远端与 MEO 远端差 <5%。
            readonly property real camDist: {
                if (!sceneCamera) return 0
                var c = sceneCamera.scenePosition
                var p = satNode.scenePosition
                var dx = c.x - p.x, dy = c.y - p.y, dz = c.z - p.z
                return Math.sqrt(dx*dx + dy*dy + dz*dz)
            }
            readonly property real distScale: camDist > 0
                ? Math.max(CoordinateSystem.markerScaleMin,
                           Math.min(CoordinateSystem.markerScaleMax,
                                    Math.pow(camDist / CoordinateSystem.markerRefDistanceKm,
                                             CoordinateSystem.markerScaleExponent)))
                : 1.0
            // 真实模式($ORB)：引用 visibilityEpoch，health 翻转（卫星升/降）时
            // 仅重估本 delegate 的 visible（零重建），与轨道环同一机制。
            // 仿真模式：epoch 恒定，恒取 model.visible（面板开关控制）。
            visible: {
                if (SatelliteModel.visibilityEpoch >= 0)
                    return SatelliteModel.isSatelliteVisible(index)
                return model.visible
            }

            // 动态缩放：基础尺寸(模型注入) × 距离补偿
            scale: {
                let s = (model.visualScale !== undefined ? model.visualScale : 1.5) * distScale;
                return Qt.vector3d(s, s, s);
            }

            Model {
                id: satModel
                source: root.modelDir + "meshes/node10477_Satellite_v1_SG.mesh"
                materials: PrincipledMaterial {
                    baseColor: model.isDeception ? (model.signalActive ? "#FF6666" : "#FFAAAA") : "#FFFFFF"
                    baseColorMap: Texture {
                        id: satTexture
                        source: root.modelDir + "satellite_diffuse.jpg"
                    }
                    emissiveMap: satTexture
                    emissiveFactor: Qt.vector3d(0.6, 0.6, 0.6)
                    metalness: 0.5
                    roughness: 0.3
                    specularAmount: 0.8
                }

                // 使用 TapHandler 处理 3D 对象点击
                TapHandler {
                    enabled: model.isDeception === true
                    onTapped: {
                        if (model.isDeception && SatelliteModel) {
                            var newActive = !model.signalActive
                            SatelliteModel.setSignalActive(model.index, newActive)
                        }
                    }
                }
            }

            // 任务标签
            Node {
                y: 200
                visible: model.phase !== undefined && model.phase > 0

                Text {
                    text: {
                        if (model.phase === undefined) return "";
                        switch(model.phase) {
                            case 1: return "地球停泊轨道";
                            case 2: return "奔月转移(TLI)";
                            case 3: return "中途轨道修正";
                            case 4: return "制动捕获阶段";
                            case 5: return "环月任务轨道";
                            default: return "";
                        }
                    }
                    color: "#00AAFF"
                    font.pixelSize: 40
                    font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
    }

    // 欺骗卫星干扰信号效果 - 独立渲染
    Repeater3D {
        model: root.satelliteModel
        delegate: DeceptionSignalItem {
            visible: model.isDeception === true && model.visible && model.signalActive === true
            satellitePosition: model.position
            // 目标位置：固定在北京地面点
            targetPosition: root.jammerTargetPos
            signalColor: "#FF0000"
            active: model.isDeception === true && model.visible && model.signalActive === true
        }
    }
}
