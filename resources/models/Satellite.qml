import QtQuick 2.15
import QtQuick3D 1.15

Node {
    id: satellite_obj

    // 获取可执行文件目录，构建文件系统路径（不依赖 qrc）
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property string modelDir: "file:///" + appDir + "models/"

    Model {
        id: node10477_Satellite_v1_SG
        source: satellite_obj.modelDir + "meshes/node10477_Satellite_v1_SG.mesh"

        DefaultMaterial {
            id: node10477_Satellite_v1_material
            diffuseMap: Texture {
                source: satellite_obj.modelDir + "satellite_diffuse.jpg"
                tilingModeHorizontal: Texture.Repeat
                tilingModeVertical: Texture.Repeat
            }
        }
        materials: [
            node10477_Satellite_v1_material
        ]
    }
}
