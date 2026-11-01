import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "pages" as Pages

Item {
    id: root

    property string currentSection: "user" // user/device/scene
    readonly property color brandBlue: "#0B2B5B"
    readonly property color primary: "#1677FF"
    readonly property color pageBg: "#F5F7FA"
    readonly property color border: "#E5E7EB"

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 52  // 为 AppShell 的 topBar 留出空间
        spacing: 0

        // 顶部 Tab 栏 - 深蓝色背景
        Rectangle {
            id: topTabs
            Layout.fillWidth: true
            implicitHeight: 56
            // 比顶部应用栏稍微浅一点的蓝色，区分层级
            color: "#115293"

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: "#0A2550"
                opacity: 0.65
            }

            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 30
                spacing: 10

                Rectangle {
                    width: 120
                    height: 36
                    radius: 6
                    color: root.currentSection === "user"
                           ? root.primary
                           : (userTabHover.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                    border.width: 0
                    border.color: "transparent"
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "用户管理"
                            color: "white"
                            font.pixelSize: 14
                            font.bold: root.currentSection === "user"
                        }
                    }
                    
                    MouseArea {
                        id: userTabHover
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: {
                            root.currentSection = "user"
                            console.log("切换到: 用户管理")
                        }
                    }
                }

                Rectangle {
                    width: 120
                    height: 36
                    radius: 6
                    color: root.currentSection === "device"
                           ? root.primary
                           : (deviceTabHover.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                    border.width: 0
                    border.color: "transparent"
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "设备管理"
                            color: "white"
                            font.pixelSize: 14
                            font.bold: root.currentSection === "device"
                        }
                    }
                    
                    MouseArea {
                        id: deviceTabHover
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: {
                            root.currentSection = "device"
                            console.log("切换到: 设备管理")
                        }
                    }
                }

                Rectangle {
                    width: 120
                    height: 36
                    radius: 6
                    color: root.currentSection === "scene"
                           ? root.primary
                           : (sceneTabHover.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent")
                    border.width: 0
                    border.color: "transparent"
                    
                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Text {
                            text: "场景管理"
                            color: "white"
                            font.pixelSize: 14
                            font.bold: root.currentSection === "scene"
                        }
                    }
                    
                    MouseArea {
                        id: sceneTabHover
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        hoverEnabled: true
                        onClicked: {
                            root.currentSection = "scene"
                            console.log("切换到: 场景管理")
                        }
                    }
                }
            }
        }

        // 内容区域
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                id: contentStack
                anchors.fill: parent
                currentIndex: {
                    switch (root.currentSection) {
                    case "user": return 0
                    case "device": return 1
                    case "scene": return 2
                    default: return 1
                    }
                }

                // 用户管理页面 - 预加载
                Loader {
                    id: userPageLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: true
                    asynchronous: true
                    sourceComponent: Pages.UserManagementPage {}
                }

                // 设备管理页面 - 预加载
                Loader {
                    id: devicePageLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: true
                    asynchronous: true
                    sourceComponent: Pages.DeviceManagementPage {}
                }

                // 场景管理页面 - 预加载
                Loader {
                    id: scenePageLoader
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: true
                    asynchronous: true
                    sourceComponent: Pages.ScenarioManagementPage {}
                }
            }
        }
    }
}