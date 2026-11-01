import QtQuick
import QtQuick3D

Node {
    id: turntableNode

    // 加载转换后的 666.mesh 模型
    Node666 {
        id: model666
        scale: Qt.vector3d(0.01, 0.01, 0.01)
        position: Qt.vector3d(0, 0, 0)
    }
}
