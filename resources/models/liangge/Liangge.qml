import QtQuick
import QtQuick3D

Node {
    id: node

    // 获取可执行文件目录，构建文件系统路径（不依赖 qrc）
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property string modelDir: "file:///" + appDir + "models/liangge/"
    
    // Resources - 使用绝对路径
    property url textureData: modelDir + "maps/textureData.png"
    property url textureData12: modelDir + "maps/textureData12.png"
    Texture {
        id: _0_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData
    }
    Texture {
        id: _1_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData12
    }
    PrincipledMaterial {
        id: fallback_Material_material
        baseColor: "#ff083d8a"
        metalness: 1
        roughness: 1
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: material_material
        baseColor: "#ffcccccc"
        baseColorMap: _0_texture
        metalnessMap: _1_texture
        roughnessMap: _1_texture
        alphaMode: PrincipledMaterial.Opaque
    }

    // 转台模型 - 第一部分（主模型）
    // scale: 10 * 1.5 = 15
    Model {
        id: root
        position: Qt.vector3d(0, 0, 0)
        scale: Qt.vector3d(15, 15, 15)
        source: modelDir + "meshes/a660edf19bd0adcd41a95dc58293cbd2_mesh.mesh"
        materials: [material_material, fallback_Material_material]
        Component.onCompleted: {
            console.log("[Liangge] Model 1 loaded, source:", source)
        }
    }

    // 转台模型 - 第二部分
    // scale: 0.01 * 100 = 1
    Model {
        id: root2
        position: Qt.vector3d(0, 0, 0)
        scale: Qt.vector3d(1, 1, 1)
        source: modelDir + "meshes/__012_mesh.mesh"
        materials: [material_material, fallback_Material_material]
        Component.onCompleted: {
            console.log("[Liangge] Model 2 loaded, source:", source)
        }
    }
}
