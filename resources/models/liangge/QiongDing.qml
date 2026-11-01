import QtQuick
import QtQuick3D

Node {
    id: node
    
    // 整体位置偏移（Node级别，子节点继承）
    position: Qt.vector3d(0.11, 0.34, -40.25)

    // 获取可执行文件目录，构建文件系统路径（不依赖 qrc）
    property string appPath: Qt.application.arguments[0].replace(/\\/g, '/')
    property string appDir: appPath.substring(0, appPath.lastIndexOf('/') + 1)
    property string modelDir: "file:///" + appDir + "models/liangge/"
    
    // 材质 - 匹配glb原始设置 (深蓝色)
    PrincipledMaterial {
        id: domeMaterial
        baseColor: "#ff083d8a"
        metalness: 1.0
        roughness: 1.0
        alphaMode: PrincipledMaterial.Opaque
    }
    
    // 穹顶模型
    Model {
        id: qiongDing
        scale: Qt.vector3d(1, 1, 1)
        eulerRotation: Qt.vector3d(0, 0, 0)
        source: modelDir + "meshes/__012_mesh.mesh"
        materials: [domeMaterial]
    }
    
    Component.onCompleted: {
        console.log("QiongDing loaded")
    }
}