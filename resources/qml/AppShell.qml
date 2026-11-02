import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window 2.15
import Qt5Compat.GraphicalEffects
import "."
import "pages" as Pages
import NavModels
import "components"

Item {
    id: root
    anchors.fill: parent

    property string currentTop: "task" // task/device/scene/cmd/script/monitor/about
    property string currentSide: ""     // per-top section
    property string simCurrentView: "3D"

    // 视角测试钩子: NAVJEST_2D_VIEW_MS=N 时 N 毫秒后切 sim 页 2D 地图
    // （自动化验证 2D 星下点与 3D 同源一致性）
    Timer {
        running: typeof _map2DViewMs !== "undefined" && _map2DViewMs > 0
        interval: typeof _map2DViewMs !== "undefined" ? _map2DViewMs : 0
        repeat: false
        onTriggered: {
            root.simCurrentView = "2D"
            openPage("sim")
            locateTimer.start()
        }
    }
    // 切 2D 后延时定位到模拟源($PNT 报文给出的接收站位置)
    Timer {
        id: locateTimer
        interval: 800
        repeat: false
        onTriggered: {
            if (simPageLoader.item && simPageLoader.item.map2DView) {
                // 中等缩放：可见卫星星下点(±70°范围)全部入画
                simPageLoader.item.map2DView.locateTo(116.4, 39.9, 4.5)
            }
        }
    }
    property string currentPage: "sim"
    property string pendingPage: ""        // 待确认的目标页面
    property string pendingPageName: ""    // 待确认的页面名称（用于显示）
    // 当前试验场景（主界面默认地月空间；下方三个 2D 按钮分别对应智能无人机/无人机装备/月球车载）
    property string simCurrentScenario: "地月空间导航装备试验场景"
    readonly property var simScenarioList: ["地月空间导航装备试验场景", "智能无人机导航试验场景", "无人机导航装备试验场景", "月球车载导航装备试验场景"]
    readonly property color neon: "#22d3ee"
    readonly property color neon2: "#60a5fa"
    readonly property color topBg1: "#0b1220"
    readonly property color topBg2: "#0a2a4a"
    readonly property color topBg3: "#07111f"
    // 当前顶层 Window，用于最小化/最大化/关闭
    property var window: Qt.application.activeWindow

    // 运行状态：从 PntDataModel 获取任务状态“空闲”
    readonly property string runStatus: PntDataModel ? PntDataModel.taskStatus : "空闲"

    // 顶部时间显示（YYYY/MM/DD HH:MM:SS）
    property string currentTime: Qt.formatDateTime(new Date(), "yyyy/MM/dd hh:mm:ss")

    Timer {
        id: clockTimer
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.currentTime = Qt.formatDateTime(new Date(), "yyyy/MM/dd hh:mm:ss")
    }

    Rectangle {
        anchors.fill: parent
        color: "#1f2329"
    }

    function openPage(key) {
        currentPage = key
    }

    // 显示确认对话框
    function showConfirmDialog(key, displayName) {
        // 仅在 2D 地图场景下启用预览窗口，其它场景直接进入页面
        if (currentPage === "sim" && simCurrentView === "2D") {
            pendingPage = key
            pendingPageName = displayName
            previewDialog.visible = true
        } else {
            openPage(key)
        }
    }

    // 确认导航
    function confirmNavigation() {
        if (pendingPage !== "") {
            openPage(pendingPage)
            pendingPage = ""
            pendingPageName = ""
            previewDialog.visible = false
        }
    }

    // 取消导航
    function cancelNavigation() {
        pendingPage = ""
        pendingPageName = ""
        previewDialog.visible = false
    }

    function pageForKey(key) {
        switch (key) {
        case "management": return managementPage
        case "user": return userPage
        case "device": return devicePage
        case "scene": return scenePage
        case "cmd": return cmdPage
        case "script": return scriptPage
        case "monitor": return monitorPage
        case "about": return aboutPage
        case "sim": return simPage
        case "cabinet": return cabinetPage
        case "dome": return domePage
        default: return monitorPage
        }
    }

    Item {
        id: contentRoot
        anchors.fill: parent

        // 页容器 - 所有页面同时异步预加载
        Item {
            anchors.fill: parent
            
            // 首页仿真页面
            Loader {
                id: simPageLoader
                anchors.fill: parent
                active: true
                asynchronous: true
                visible: root.currentPage === "sim"
                sourceComponent: simPage
            }

            // 后台管理页面
            Loader {
                id: managementPageLoader
                anchors.fill: parent
                active: true
                asynchronous: true
                visible: root.currentPage === "management"
                sourceComponent: managementPage
            }

            // 机柜页面
            Loader {
                id: cabinetPageLoader
                anchors.fill: parent
                active: true
                asynchronous: true
                visible: root.currentPage === "cabinet"
                sourceComponent: cabinetPage
            }

            // 暗室页面 —— 懒加载：页面内有大量与仿真信号联动的 3D 绑定，
            // 常驻激活会让离开页面后全局帧率被拖低（Debug 下实测 180 -> 64），
            // 因此离开即卸载，重进时异步重建
            Loader {
                id: domePageLoader
                anchors.fill: parent
                active: root.currentPage === "dome"
                asynchronous: true
                visible: root.currentPage === "dome"
                sourceComponent: domePage
            }
        }

            Rectangle {
                id: topBar
                z: 100
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 4
                // 顶栏高度统一为后台样式（52），首页与后台保持一致
                height: 52
                color: "transparent"

            Rectangle {
                anchors.fill: parent
                color: root.currentPage === "management" ? "#0B2B5B" : "transparent"
            }

            Row {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 10
                spacing: 8

                Item {
                    id: brandPill
                    height: 40
                    width: brandRow.implicitWidth + 18

                    Rectangle {
                        anchors.fill: parent
                        radius: 10
                        color: "transparent"
                        border.width: 0
                        border.color: "transparent"
                    }

                    Row {
                        id: brandRow
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        spacing: 8

                        Image {
                            height: 36
                            fillMode: Image.PreserveAspectFit
                            source: "qrc:/icons/top_brand.png"
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: 2
                            text: "导航场景模拟仿真软件"
                            color: "white"
                            font.pixelSize: 16
                            font.bold: true
                        }
                    }
                }
            }

            // 顶部右侧：状态 + 时间 + 用户头像（所有页面一致）
            Column {
                id: topRightColumn
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 10
                spacing: 4

                Row {
                    id: topRightRow
                    anchors.right: parent.right
                    spacing: 10

                    // 运行状态指示（在左）
                    Rectangle {
                        width: 96
                        height: 34      // 与头像高度保持一致
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.06)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.35)
                        Text {
                            anchors.centerIn: parent
                            text: root.runStatus
                            color: "white"
                            font.pixelSize: 12
                        }
                    }

                    // 时间显示（空闲与用户头像之间），在所有页面显示
                    Rectangle {
                        width: 170
                        height: 34
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.06)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.35)
                        visible: true

                        Text {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            text: root.currentTime
                            color: "white"
                            font.pixelSize: 12
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideNone
                        }
                    }

                    // 用户头像入口：点击弹出 首页 / 后台 / 退出（在右）
                    Rectangle {
                        id: userEntry
                        width: 34
                        height: 34
                        radius: 8
                        color: userMenu.visible ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.06)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.55)

                        Image {
                            anchors.centerIn: parent
                            fillMode: Image.PreserveAspectFit
                            source: "qrc:/icons/user.png"
                            sourceSize.width: 20
                            sourceSize.height: 20
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: userMenu.open()
                        }

                        Popup {
                            id: userMenu
                            x: parent.width - width
                            y: parent.height + 6
                            width: 120
                            padding: 4
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                            background: Rectangle {
                                color: "#FFFFFF"
                                radius: 6
                                border.color: "#E5E7EB"
                                border.width: 1
                            }

                            contentItem: Column {
                                spacing: 2

                                Rectangle {
                                    width: parent.width
                                    height: 32
                                    color: homeItem.hovered ? "#F3F4F6" : "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "首页"
                                        color: "#111827"
                                        font.pixelSize: 14
                                    }
                                    MouseArea {
                                        id: homeItem
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            userMenu.close()
                                            openPage("sim")
                                        }
                                    }
                                }

                                Rectangle {
                                    width: parent.width
                                    height: 32
                                    color: mgmtItem.hovered ? "#F3F4F6" : "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "后台"
                                        color: "#111827"
                                        font.pixelSize: 14
                                    }
                                    MouseArea {
                                        id: mgmtItem
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            userMenu.close()
                                            openPage("management")
                                        }
                                    }
                                }

                                Rectangle {
                                    width: parent.width
                                    height: 32
                                    color: exitItem.hovered ? "#FEE2E2" : "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: "退出"
                                        color: "#B91C1C"
                                        font.pixelSize: 14
                                    }
                                    MouseArea {
                                        id: exitItem
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            userMenu.close()
                                            if (root.window) root.window.close();
                                            else Qt.quit();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

            }

            // 第二行：场景名称，统一放在顶栏下方（只在首页/2D页面等非后台页面显示）
            Rectangle {
                id: scenarioTag
                anchors.right: parent.right
                anchors.top: topBar.bottom
                anchors.topMargin: 4
                anchors.rightMargin: 10
                width: Math.max(210, scenarioText.implicitWidth + 32)
                height: 38
                radius: 10
                color: Qt.rgba(1, 1, 1, 0.06)
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.35)
                visible: root.currentPage !== "management"
                         && root.currentPage !== "cabinet"
                         && root.currentPage !== "dome"

                Text {
                    id: scenarioText
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    text: root.simCurrentScenario
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            // 双击顶栏空白区域切换 最大化 / 还原
            MouseArea {
                id: topBarDoubleClickArea
                anchors.fill: parent
                z: 50
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                propagateComposedEvents: true
                
                // 排除右侧控件区域（用户头像等）
                readonly property real rightExclusionWidth: topRightColumn.width + 20
                
                // 拖动相关
                property real lastMouseX: 0
                property real lastMouseY: 0
                property bool isDragging: false
                
                onPressed: (mouse) => {
                    // 右侧区域不处理，让事件穿透
                    if (mouse.x > width - rightExclusionWidth) {
                        mouse.accepted = false
                        return
                    }
                    // 记录拖动起始位置
                    lastMouseX = mouse.x
                    lastMouseY = mouse.y
                    isDragging = true
                    mouse.accepted = true
                }
                onPositionChanged: (mouse) => {
                    if (!isDragging || !root.window) return
                    // 非最大化/全屏状态下才允许拖动
                    if (root.window.visibility === Window.Maximized || root.window.visibility === Window.FullScreen) return
                    
                    const dx = mouse.x - lastMouseX
                    const dy = mouse.y - lastMouseY
                    root.window.x += dx
                    root.window.y += dy
                }
                onReleased: {
                    isDragging = false
                }
                onDoubleClicked: (mouse) => {
                    isDragging = false
                    // 右侧区域不处理
                    if (mouse.x > width - rightExclusionWidth) {
                        return
                    }
                    if (!root.window) return
                    // 在全屏和正常窗口之间切换
                    if (root.window.visibility === Window.FullScreen) {
                        // 还原：只调 showNormal，Qt 自动恢复进入全屏前的
                        // 窗口几何 —— 位置、大小、所在屏幕与“刚才的窗口模式”
                        // 完全一致，全程不做任何跨屏坐标计算
                        root.window.showNormal()
                    } else {
                        // 进入全屏：不改动几何，让 Qt 把当前窗口状态存为还原目标
                        root.window.showFullScreen()
                    }
                }
            }
        }

        // 2D 地图位置信息（稳定位于用户图标下方）
        Rectangle {
            id: mapPointInfoPanel
            visible: root.currentPage === "sim" && root.simCurrentView === "2D"
            z: 200
            parent: contentRoot

            width: 160
            height: panelContent.implicitHeight + 20
            radius: 10
            color: Qt.rgba(0, 0, 0, 0.5)
            border.width: 1
            border.color: Qt.rgba(255/255, 255/255, 255/255, 0.7)

            readonly property color simSourceColor: "#22C55E"  // 模拟源绿色
            readonly property color dutColor: "#3B82F6"         // 待测设备蓝色

            function fmt(v) {
                if (v === undefined || v === null || v === 0) return "--"
                return Number(v).toFixed(4)
            }

            // 计算误差（模拟源 - 待测设备）
            function calcError(simVal, dutVal) {
                if (simVal === undefined || simVal === null || dutVal === undefined || dutVal === null || simVal === 0 || dutVal === 0) return "--"
                return Number(simVal - dutVal).toFixed(4)
            }

            function updatePos() {
                if (!userEntry || !contentRoot) return
                
                // 强制获取最新的坐标转换
                const p = userEntry.mapToItem(contentRoot, userEntry.width, userEntry.height)
                
                // 计算目标 X 位置（用户头像左侧）
                let nx = Math.round(p.x - width)
                
                // 根据是否有场景标签，动态确定卡片的最小 Y 值，避免被挡住
                const baseTop = topBar ? (topBar.y + topBar.height) : 52
                const scenarioBottom = (typeof scenarioTag !== "undefined" && scenarioTag.visible)
                                        ? (scenarioTag.y + scenarioTag.height)
                                        : baseTop
                const minTop = scenarioBottom + 8   // 再留一点间距
                let ny = Math.round(p.y + 10)
                
                // 夹紧到屏幕范围内
                const maxX = contentRoot.width - width - 10
                const maxY = contentRoot.height - height - 10
                
                // 边界保护：确保坐标在有效范围内
                nx = Math.max(10, Math.min(maxX, nx))
                ny = Math.max(minTop, Math.min(maxY, ny))
                
                // 只在坐标确实需要变化时更新，避免不必要的布局刷新
                if (Math.abs(x - nx) > 1 || Math.abs(y - ny) > 1) {
                    x = nx
                    y = ny
                }
            }

            function requestUpdatePos() {
                if (!visible) return
                Qt.callLater(updatePos)
            }

            onVisibleChanged: if (visible) requestUpdatePos()
            onWidthChanged: requestUpdatePos()
            onHeightChanged: requestUpdatePos()

            Connections {
                target: root
                function onCurrentPageChanged() { mapPointInfoPanel.requestUpdatePos() }
                function onSimCurrentViewChanged() { mapPointInfoPanel.requestUpdatePos() }
                function onSimCurrentScenarioChanged() { mapPointInfoPanel.requestUpdatePos() }
            }

            Connections {
                target: topBar
                function onHeightChanged() { mapPointInfoPanel.requestUpdatePos() }
            }

            // 监听窗口尺寸变化（包括全屏切换）
            Connections {
                target: contentRoot
                function onWidthChanged() { mapPointInfoPanel.requestUpdatePos() }
                function onHeightChanged() { mapPointInfoPanel.requestUpdatePos() }
            }

            // 监听窗口可见性变化（全屏/窗口切换）
            // 说明：Connections 直接挂 onVisibilityChanged 在部分 Qt 版本下
            // 解析不到 QWindow 信号（运行期告警且不生效）。改为绑定 visibility
            // 属性，靠属性变更通知触发，行为稳定。
            Item {
                visible: false
                property int winVisibility: root.window ? root.window.visibility : 0
                onWinVisibilityChanged: mapPointUpdateTimer.start()
            }

            // 延迟更新定时器，确保布局完成后再计算位置
            Timer {
                id: mapPointUpdateTimer
                interval: 150
                repeat: false
                onTriggered: {
                    mapPointInfoPanel.requestUpdatePos()
                    // 再次延迟确保二次布局完成
                    secondaryUpdateTimer.start()
                }
            }
            
            // 二次更新定时器
            Timer {
                id: secondaryUpdateTimer
                interval: 150
                repeat: false
                onTriggered: mapPointInfoPanel.requestUpdatePos()
            }

            layer.enabled: true
            layer.effect: DropShadow {
                horizontalOffset: 0
                verticalOffset: 6
                radius: 16
                samples: 32
                color: "#28000000"
            }

            // 点击卡片定位到模拟源位置
            MouseArea {
                id: cardMouseArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                // 点击时定位到模拟源位置
                onClicked: {
                    // 确保 map2DView 可访问
                    if (simPageLoader.item && simPageLoader.item.map2DView) {
                        simPageLoader.item.map2DView.locateToSimPoint()
                    } else {
                        console.log("[mapPointInfoPanel] map2DView not available")
                    }
                }
                
                // 悬停效果
                Rectangle {
                    anchors.fill: parent
                    radius: parent.parent.radius
                    color: "transparent"
                    border.width: cardMouseArea.containsMouse ? 2 : 0
                    border.color: "#22C55E"  // 绿色高亮边框
                }
            }

            Column {
                id: panelContent
                x: 10
                y: 10
                width: parent.width - 20
                spacing: 8

                // 模拟源 (绿色)
                Row {
                    width: parent.width
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    Rectangle { width: 10; height: 10; radius: 5; color: mapPointInfoPanel.simSourceColor; border.width: 1; border.color: "white" }
                    Text {
                        text: "模拟源"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width - 18
                        elide: Text.ElideRight
                    }
                }
                Text { text: "纬度  " + mapPointInfoPanel.fmt(PntDataModel ? PntDataModel.simLatitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
                Text { text: "经度  " + mapPointInfoPanel.fmt(PntDataModel ? PntDataModel.simLongitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
                Text { text: "高程  " + mapPointInfoPanel.fmt(PntDataModel ? PntDataModel.simAltitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(255/255, 255/255, 255/255, 0.5) }

                // 待测设备 (蓝色)
                Row {
                    width: parent.width
                    spacing: 8
                    anchors.horizontalCenter: parent.horizontalCenter
                    Rectangle { width: 10; height: 10; radius: 5; color: mapPointInfoPanel.dutColor; border.width: 1; border.color: "white" }
                    Text {
                        text: "待测设备"
                        color: "#FFFFFF"
                        font.pixelSize: 14
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        width: parent.width - 18
                        elide: Text.ElideRight
                    }
                }
                Text { text: "纬度  " + mapPointInfoPanel.fmt(PntDataModel ? PntDataModel.dutLatitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
                Text { text: "经度  " + mapPointInfoPanel.fmt(PntDataModel ? PntDataModel.dutLongitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
                Text { text: "高程  " + mapPointInfoPanel.fmt(PntDataModel ? PntDataModel.dutAltitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }

                Rectangle { width: parent.width; height: 1; color: Qt.rgba(255/255, 255/255, 255/255, 0.5) }

                // 误差显示（模拟源 - 待测设备）
                Text {
                    text: "误差"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.bold: true
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                }
                Text { text: "纬度误差  " + mapPointInfoPanel.calcError(PntDataModel ? PntDataModel.simLatitude : null, PntDataModel ? PntDataModel.dutLatitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
                Text { text: "经度误差  " + mapPointInfoPanel.calcError(PntDataModel ? PntDataModel.simLongitude : null, PntDataModel ? PntDataModel.dutLongitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
                Text { text: "高程误差  " + mapPointInfoPanel.calcError(PntDataModel ? PntDataModel.simAltitude : null, PntDataModel ? PntDataModel.dutAltitude : null); color: "#FFFFFF"; font.pixelSize: 13; width: parent.width; horizontalAlignment: Text.AlignHCenter }
            }
        }

        LeftDock {
            currentPage: root.currentPage
            simCurrentView: root.simCurrentView
            simCurrentScenario: root.simCurrentScenario
            onSim3DRequested: {
                root.simCurrentScenario = "地月空间导航装备试验场景"
                root.simCurrentView = "3D"
                openPage("sim")
            }
            onSim2DRequested: (scenario) => {
                root.simCurrentScenario = scenario
                root.simCurrentView = "2D"
                openPage("sim")
            }
        }

        RightDock {
            currentPage: root.currentPage
            onSimRequested: openPage("sim")
            onCabinetRequested: showConfirmDialog("cabinet", "机柜")
            onDomeRequested: showConfirmDialog("dome", "暗室")
        }
    }

    Component { id: managementPage; ManagementContainer {} }
    Component { id: userPage; Pages.UserManagementPage {} }
    Component { id: devicePage; Pages.DeviceManagementPage {} }
    Component { id: scenePage; Pages.ScenarioManagementPage {} }
    Component { id: cmdPage; Pages.CommandSetManagementPage {} }
    Component { id: scriptPage; Pages.ScriptManagementPage {} }
    Component { id: monitorPage; Pages.TaskMonitoringPage {} }
    Component { id: aboutPage; Pages.AboutPage {} }
    Component { id: domePage; Pages.DomeScenePage {} }
    Component { id: cabinetPage; Pages.CabinetPage {} }
    Component {
        id: simPage
        Item {
            property string currentView: root.simCurrentView
            property string currentScenario: root.simCurrentScenario
            
            // 暴露 map2DView 供外部访问（用于卡片点击定位）
            readonly property var map2DView: map2DLoader.item

            // 3D场景 - 预加载
            Loader {
                id: scene3DLoader
                anchors.fill: parent
                active: true
                asynchronous: true
                visible: root.simCurrentView === "3D"
                sourceComponent: SceneViewItem {}
            }

            // 2D地图 - 预加载
            Loader {
                id: map2DLoader
                anchors.fill: parent
                active: true
                asynchronous: true
                visible: root.simCurrentView === "2D"
                sourceComponent: Map2DView {
                    objectName: "Map2DViewRoot"
                }
            }
        }
    }

        // 小型化悬浮预览窗口（导航确认 + 实时预览 + 拖动/缩放）
        PreviewConfirmDialog {
            id: previewDialog
            z: 999
            pendingPage: root.pendingPage
            onConfirmed: root.confirmNavigation()
            onCanceled: root.cancelNavigation()
        }

}
