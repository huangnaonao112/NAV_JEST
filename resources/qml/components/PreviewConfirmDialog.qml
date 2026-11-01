// 小型化悬浮预览窗口：导航确认 + 实时预览（机柜/暗室）+ 拖动/缩放
// AppShell 持有 pendingPage 状态并通过 confirmed()/canceled() 接收回调
import QtQuick
import "qrc:/resources/qml/pages" as Pages

Item {
    id: root

    property string pendingPage: ""
    signal confirmed()
    signal canceled()

    anchors.fill: parent

    // 半透明遮罩（独立于窗口）
    Rectangle {
        id: dialogOverlay
        visible: dialog.visible
        anchors.fill: parent
        color: "#60000000"
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: (mouse) => { mouse.accepted = true }
            onClicked: (mouse) => {
                // 仅当点击在窗口外部时关闭
                const local = dialog.mapFromItem(dialogOverlay, mouse.x, mouse.y)
                const inside = (local.x >= 0 && local.y >= 0 && local.x <= dialog.width && local.y <= dialog.height)
                if (!inside) {
                    root.canceled()
                }
                mouse.accepted = true
            }
            onWheel: (wheel) => { wheel.accepted = true }
        }
    }

    Rectangle {
        id: dialog
        visible: false
        // 预览内容的真实基准尺寸（用于等比例缩放）
        property int baseW: root.pendingPage === "cabinet" ? 776 : 800
        property int baseH: root.pendingPage === "cabinet" ? 700 : 600

        // 预览窗口默认尺寸（用户选择的最合适显示大小）
        property int defaultW: root.pendingPage === "cabinet" ? 400 : 408
        property int defaultH: root.pendingPage === "cabinet" ? 397 : 323

        width: defaultW
        height: defaultH
        x: parent ? (parent.width - width) / 2 : 0
        y: parent ? (parent.height - height) / 2 : 0
        radius: 12
        color: "#1a1a2e"
        border.color: "#00c853"
        border.width: 2
        z: 1000

        property int minW: root.pendingPage === "cabinet" ? 400 : 408
        property int minH: root.pendingPage === "cabinet" ? 397 : 323

        // 拖动句柄 - 顶部区域
        Rectangle {
            id: dragHandle
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 20
            color: "transparent"
            radius: 12
            z: 5

            // 拖动区域
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                hoverEnabled: true
                cursorShape: Qt.SizeAllCursor
                property real offsetX: 0
                property real offsetY: 0
                property int clickCount: 0
                Timer {
                    id: topDblClickTimer
                    interval: 260
                    repeat: false
                    onTriggered: parent.clickCount = 0
                }
                onPressed: (mouse) => {
                    offsetX = mouse.x
                    offsetY = mouse.y
                    mouse.accepted = true
                }
                onClicked: (mouse) => {
                    clickCount += 1
                    if (clickCount === 1) {
                        topDblClickTimer.restart()
                        mouse.accepted = true
                        return
                    }
                    topDblClickTimer.stop()
                    clickCount = 0
                    console.log("[PreviewEnter] page=" + root.pendingPage)
                    root.confirmed()
                    mouse.accepted = true
                }
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    let newX = dialog.x + mouse.x - offsetX
                    let newY = dialog.y + mouse.y - offsetY
                    newX = Math.max(0, Math.min(dialog.parent.width - dialog.width, newX))
                    newY = Math.max(0, Math.min(dialog.parent.height - dialog.height, newY))
                    dialog.x = newX
                    dialog.y = newY
                }
            }
        }

        // 关闭按钮
        Rectangle {
            width: 26
            height: 26
            radius: 13
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 10
            anchors.rightMargin: 10
            color: closeBtn.containsMouse ? "#ff4444" : "#333"
            z: 10
            Text {
                anchors.centerIn: parent
                text: "✕"
                color: "white"
                font.pixelSize: 12
            }
            MouseArea {
                id: closeBtn
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.canceled()
            }
        }

        // 预览内容区域
        Item {
            id: previewViewport
            anchors.fill: parent
            anchors.topMargin: 28
            anchors.margins: 10
            clip: true

            property bool resizing: false

            Item {
                id: previewHost
                anchors.centerIn: parent
                width: dialog.baseW
                height: dialog.baseH
                z: 1
                transform: Scale {
                    origin.x: previewHost.width / 2
                    origin.y: previewHost.height / 2
                    xScale: Math.min(previewViewport.width / previewHost.width, previewViewport.height / previewHost.height)
                    yScale: xScale
                }

                Loader {
                    id: previewLoader
                    anchors.fill: parent
                    sourceComponent: root.pendingPage === "cabinet" ? cabinetPreviewComp : (root.pendingPage === "dome" ? domePreviewComp : null)
                    z: 1
                }
            }

        }

        // 右下角调整大小句柄
        Rectangle {
            id: resizeHandle
            width: 20
            height: 20
            radius: 3
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 4
            anchors.bottomMargin: 4
            color: resizeMA.containsMouse ? "#00c853" : "#2a2a4a"
            border.color: "#4b5563"
            border.width: 1
            z: 10

            // 三角形指示
            Canvas {
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext('2d')
                    ctx.strokeStyle = resizeMA.containsMouse ? "#00c853" : "#666"
                    ctx.lineWidth = 2
                    ctx.beginPath()
                    ctx.moveTo(width - 4, 4)
                    ctx.lineTo(4, height - 4)
                    ctx.moveTo(width - 4, 10)
                    ctx.lineTo(10, height - 4)
                    ctx.stroke()
                }
            }

            MouseArea {
                id: resizeMA
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SizeFDiagCursor
                property real lastLogT: 0
                property real startW: 0
                property real startH: 0
                property real startX: 0
                property real startY: 0
                onPressed: (mouse) => {
                    startW = dialog.width
                    startH = dialog.height
                    startX = mouse.x
                    startY = mouse.y
                    previewViewport.resizing = true
                    mouse.accepted = true
                }
                onReleased: previewViewport.resizing = false
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    const dx = mouse.x - startX
                    const dy = mouse.y - startY
                    let newW = Math.max(dialog.minW, startW + dx)
                    let newH = Math.max(dialog.minH, startH + dy)
                    if (dialog.x + newW > dialog.parent.width)
                        newW = dialog.parent.width - dialog.x
                    if (dialog.y + newH > dialog.parent.height)
                        newH = dialog.parent.height - dialog.y
                    dialog.width = newW
                    dialog.height = newH

                    const t = Date.now()
                    if (t - lastLogT > 120) {
                        lastLogT = t
                        const r = (newW / newH)
                        console.log("[PreviewResize] page=" + root.pendingPage + " size=" + Math.round(newW) + "x" + Math.round(newH) + " ratio=" + r.toFixed(4))
                    }
                }
            }
        }
    }

    // 机柜预览组件
    Component {
        id: cabinetPreviewComp
        Item {
            anchors.fill: parent
            Pages.CabinetPage {
                anchors.fill: parent
            }
        }
    }

    // 暗室预览组件
    Component {
        id: domePreviewComp
        Item {
            anchors.fill: parent
            Pages.DomeScenePage {
                anchors.fill: parent
            }
        }
    }
}
