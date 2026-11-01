import QtQuick
import QtQuick3D

Node {
    id: node

    // 获取可执行文件目录，构建文件系统路径（不依赖 qrc）
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property string modelDir: "file:///" + appDir + "models/liangge/"
    
    // 可调参数 - 实时打印
    property vector3d modelPosition: Qt.vector3d(0.11, 0.34, -40.25)  // 组合模型几何中心偏移
    property vector3d modelScale: Qt.vector3d(15, 15, 15)
    property vector3d modelRotation: Qt.vector3d(0, 0, 0)  // Euler angles (degrees)
    
    // 纹理
    property url textureData: modelDir + "maps/textureData.png"
    property url textureData12: modelDir + "maps/textureData12.png"
    
    Texture {
        id: baseColorTex
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData
    }
    Texture {
        id: metallicRoughnessTex
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData12
    }
    
    // 材质 - 匹配glb原始设置
    PrincipledMaterial {
        id: mainMaterial
        baseColor: "#ffcccccc"
        baseColorMap: baseColorTex
        metalness: 0.0
        roughness: 0.0
        metalnessMap: metallicRoughnessTex
        roughnessMap: metallicRoughnessTex
        alphaMode: PrincipledMaterial.Opaque
    }
    
    // 转台模型 (a660edf...)
    Model {
        id: zhuanTai
        position: modelPosition
        scale: modelScale
        eulerRotation: modelRotation
        source: modelDir + "meshes/a660edf19bd0adcd41a95dc58293cbd2_mesh.mesh"
        materials: [mainMaterial]
        Component.onCompleted: {
            console.log("[ZhuanTai] Model loaded")
            console.log("[ZhuanTai] position:", position)
            console.log("[ZhuanTai] scale:", scale)
            console.log("[ZhuanTai] rotation:", eulerRotation)
        }
    }
    
    // 实时打印参数
    onModelScaleChanged: console.log("[ZhuanTai] scale:", modelScale.x.toFixed(2), modelScale.y.toFixed(2), modelScale.z.toFixed(2))
}
