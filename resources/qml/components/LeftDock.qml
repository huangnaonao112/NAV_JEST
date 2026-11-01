// 左侧仿真视图切换 Dock：3D 与三种 2D 场景入口
// 由 AppShell 提供 currentPage/simCurrentView/simCurrentScenario 状态，
// 点击时通过信号回传选择，AppShell 负责实际跳转
import QtQuick

Column {
    id: root

    required property string currentPage
    required property string simCurrentView
    required property string simCurrentScenario

    signal sim3DRequested()
    signal sim2DRequested(string scenario)

    anchors.left: parent.left
    anchors.bottom: parent.bottom
    anchors.leftMargin: 18
    anchors.bottomMargin: 18
    spacing: 12
    z: 10
    // 在管理中心、main4、dome页面隐藏左侧按钮
    visible: root.currentPage !== "management" && root.currentPage !== "cabinet" && root.currentPage !== "dome"

    component DockButton: Rectangle {
        width: 56
        height: 56
        radius: 10
        color: "#22000000"
        border.width: 1

        property alias iconSource: icon.source
        property bool active: false

        border.color: active ? "#00c853" : "#66ffffff"
        signal activated()

        Image {
            id: icon
            anchors.fill: parent
            anchors.margins: 8
            fillMode: Image.PreserveAspectFit
            asynchronous: true
        }
        MouseArea {
            anchors.fill: parent
            onClicked: parent.activated()
        }
    }

    DockButton {
        iconSource: "qrc:/icons/nav_left_1_3d.png"
        active: root.currentPage === "sim" && root.simCurrentView === "3D"
        onActivated: {
            // 切回 3D 视图时，场景统一为“地月空间导航装备试验场景”
            root.sim3DRequested()
        }
    }

    DockButton {
        iconSource: "qrc:/icons/nav_left_2_2d_a.png"
        active: root.currentPage === "sim" && root.simCurrentView === "2D" && root.simCurrentScenario === "智能无人机导航试验场景"
        onActivated: root.sim2DRequested("智能无人机导航试验场景")
    }

    DockButton {
        iconSource: "qrc:/icons/nav_left_3_2d_b.png"
        active: root.currentPage === "sim" && root.simCurrentView === "2D" && root.simCurrentScenario === "无人机导航装备试验场景"
        onActivated: root.sim2DRequested("无人机导航装备试验场景")
    }

    DockButton {
        iconSource: "qrc:/icons/nav_left_4_reserved.png"
        active: root.currentPage === "sim" && root.simCurrentView === "2D" && root.simCurrentScenario === "月球车载导航装备试验场景"
        onActivated: root.sim2DRequested("月球车载导航装备试验场景")
    }
}
