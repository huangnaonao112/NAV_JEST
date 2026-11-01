import QtQuick
import QtQuick3D

Node {
    id: node

    // 获取可执行文件目录，构建文件系统路径（不依赖 qrc）
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property string modelDir: "file:///" + appDir + "models/"

    // Resources
    property url textureData: modelDir + "maps/textureData.jpg"
    Texture {
        id: _0_texture
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData
    }
    PrincipledMaterial {
        id: tripo_mat_6b8f1a17_e70c_4ec2_9dbc_3d96db177f92_material
        baseColorMap: _0_texture
        roughness: 0.5
        alphaMode: PrincipledMaterial.Opaque
    }

    // Nodes:
    Model {
        id: tripo_node_6b8f1a17_e70c_4ec2_9dbc_3d96db177f92
        source: node.modelDir + "meshes/tripo_mesh_6b8f1a17_e70c_4ec2_9dbc_3d96db177f92_mesh.mesh"
        materials: [
            tripo_mat_6b8f1a17_e70c_4ec2_9dbc_3d96db177f92_material
        ]
    }

    // Animations:
}
