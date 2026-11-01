// 右侧功能 Dock：仿真主页 / 机柜 / 暗室 入口
import QtQuick

Column {
    id: root

    required property string currentPage

    signal simRequested()
    signal cabinetRequested()
    signal domeRequested()

    anchors.right: parent.right
    anchors.bottom: parent.bottom
    anchors.rightMargin: 18
    anchors.bottomMargin: 18
    spacing: 12
    z: 10
    visible: root.currentPage !== "management"  // 管理中心页面隐藏右侧按钮

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
        iconSource: "qrc:/icons/nav_right_1_reserved.png"
        onActivated: root.simRequested()
    }

    DockButton {
        iconSource: "qrc:/icons/nav_right_2_main4.png"
        active: root.currentPage === "cabinet"
        onActivated: root.cabinetRequested()
    }

    DockButton {
        iconSource: "qrc:/icons/nav_right_3_dome.png"
        active: root.currentPage === "dome"
        onActivated: root.domeRequested()
    }
}
