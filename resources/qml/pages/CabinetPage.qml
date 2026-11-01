import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NavModels

Item {
    id: root
    // 移除 anchors.fill: parent，由 StackView 管理尺寸
    implicitWidth: 800
    implicitHeight: 600

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0b1220" }
            GradientStop { position: 0.55; color: "#0a2a4a" }
            GradientStop { position: 1.0; color: "#07111f" }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.18
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 0.6; color: "#00000000" }
            GradientStop { position: 1.0; color: "#AA000000" }
        }
    }

    readonly property var leftRackModel: (typeof RackStatusModelA !== "undefined") ? RackStatusModelA : null
    readonly property var rightRackModel: (typeof RackStatusModelB !== "undefined") ? RackStatusModelB : null

    Rectangle {
        visible: false
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        height: 34
        radius: 6
        color: "#AA000000"

        Text {
            anchors.centerIn: parent
            text: "机柜模型"
            color: "white"
            font.pixelSize: 14
            font.bold: true
        }
        width: titleText.implicitWidth + 24

        Text { id: titleText; visible: false; text: "机柜模型" }
    }

    Item {
        anchors.fill: parent
        anchors.margins: 40
        anchors.topMargin: 70

        RowLayout {
            anchors.centerIn: parent
            spacing: 56

            Item {
                Layout.preferredWidth: 360
                Layout.preferredHeight: 630

                RackCabinet {
                    anchors.fill: parent
                    title: "机柜A"
                    model: leftRackModel
                }
            }

            Item {
                Layout.preferredWidth: 360
                Layout.preferredHeight: 630

                RackCabinet {
                    anchors.fill: parent
                    title: "机柜B"
                    model: rightRackModel
                }
            }
        }
    }

    component RackCabinet: Item {
        id: cab
        property string title: ""
        property var model

        // shadow
        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.topMargin: 12
            radius: 14
            color: "#000000"
            opacity: 0.35
        }

        // cabinet outer frame
        Rectangle {
            id: frame
            anchors.fill: parent
            radius: 12
            border.width: 2
            border.color: "#c7ced9"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#3d444f" }
                GradientStop { position: 0.25; color: "#2b313a" }
                GradientStop { position: 1.0; color: "#1d222a" }
            }
        }

        // frame bevel highlight
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            anchors.topMargin: 4
            height: 2
            radius: 1
            color: "#e7eefc"
            opacity: 0.35
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 6
            anchors.rightMargin: 6
            anchors.bottomMargin: 5
            height: 2
            radius: 1
            color: "#000000"
            opacity: 0.4
        }

        // inner bezel
        Rectangle {
            anchors.fill: parent
            anchors.margins: 8
            radius: 10
            border.width: 2
            border.color: "#7f8a99"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#262c35" }
                GradientStop { position: 1.0; color: "#12161d" }
            }
        }

        // inner panel (where slots are)
        Rectangle {
            anchors.fill: parent
            anchors.margins: 14
            radius: 8
            color: "#0f131a"
            border.width: 1
            border.color: "#3e4654"
        }

        // side handle
        Rectangle {
            width: 10
            height: 64
            radius: 4
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: -6
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#cfd6e4" }
                GradientStop { position: 1.0; color: "#6b7280" }
            }
            opacity: 0.85
        }

        Rectangle {
            width: 2
            height: 46
            radius: 1
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 2
            color: "#ffffff"
            opacity: 0.18
        }

        // top title strip
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 0
            radius: 12
            color: "#00000000"

            Text {
                anchors.centerIn: parent
                text: cab.title
                color: "#eef4ff"
                font.pixelSize: 14
                font.bold: true
                visible: false
            }
        }

        Flickable {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 10
            anchors.topMargin: 18
            anchors.bottomMargin: 14
            clip: true
            contentWidth: width
            contentHeight: col.implicitHeight
            interactive: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            Column {
                id: col
                width: parent.width
                spacing: 2

                Repeater {
                    model: cab.model
                    delegate: DeviceSlot {
                        width: col.width
                        height: 66
                        deviceName: model.name
                        deviceState: model.state
                        blinking: model.blink
                    }
                }
            }
        }
    }

    component DeviceSlot: Item {
        id: slot
        property string deviceName: ""
        // 状态定义:
        // 0 = 正常未使用 (绿色)
        // 1 = 正常且在使用 (绿色闪烁)
        // 2 = 异常未使用 (黄色)
        // 3 = 异常且在使用 (红色)
        property int deviceState: 0
        property bool blinking: false

        readonly property color normalColor: "#22c55e"    // 绿色 - 正常
        readonly property color warningColor: "#f59e0b"   // 黄色 - 异常未使用
        readonly property color errorColor: "#ef4444"    // 红色 - 异常使用中

        // 状态颜色: 0,1=绿色; 2=黄色; 3=红色
        readonly property color stateColor: (slot.deviceState === 0 || slot.deviceState === 1) ? normalColor :
                                             (slot.deviceState === 2 ? warningColor : errorColor)

        Rectangle {
            anchors.fill: parent
            radius: 6
            border.width: 1
            border.color: "#6b7788"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#2b313a" }
                GradientStop { position: 1.0; color: "#1b2028" }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: 6
            color: "#000000"
            opacity: 0.22
            anchors.topMargin: 1
            anchors.leftMargin: 1
        }

        readonly property int sidePadding: 6
        readonly property int innerGap: 6
        readonly property int nameStripHeight: 18
        readonly property int topRowHeight: height - sidePadding * 2 - nameStripHeight - 4
        readonly property int statusPillarWidth: 50

        Item {
            anchors.fill: parent
            anchors.margins: slot.sidePadding

            // top row: grille + status pillar
            Row {
                id: topRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: slot.topRowHeight
                spacing: slot.innerGap

                Rectangle {
                    id: grille
                    width: Math.max(140, topRow.width - slot.statusPillarWidth - topRow.spacing)
                    height: topRow.height
                    radius: 4
                    border.width: 1
                    border.color: "#cfd6e4"
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "#14171c" }
                        GradientStop { position: 1.0; color: "#0a0c10" }
                    }

                    // brighter inner frame
                    Rectangle {
                        id: grilleInner
                        anchors.fill: parent
                        anchors.margins: 4
                        radius: 2
                        border.width: 1
                        border.color: "#9aa4b2"
                        color: "#00000000"
                    }

                    Repeater {
                        model: 24
                        delegate: Rectangle {
                            width: 5
                            height: 10
                            radius: 1
                            color: "#e5e7eb"
                            opacity: 0.62
                            readonly property int cols: 12
                            readonly property int rows: 2
                            readonly property real cellW: (grilleInner.width - 14) / cols
                            readonly property real cellH: (grilleInner.height - 12) / rows
                            x: grilleInner.x + 7 + (index % cols) * cellW
                            y: grilleInner.y + 6 + Math.floor(index / cols) * cellH
                        }
                    }
                }

                Item {
                    id: statusPillar
                    width: slot.statusPillarWidth
                    height: topRow.height

                    Rectangle {
                        anchors.fill: parent
                        radius: 4
                        border.width: 1
                        border.color: "#cfd6e4"
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#2b3038" }
                            GradientStop { position: 1.0; color: "#151a22" }
                        }
                    }

                    Item {
                        id: statusPillarContent
                        anchors.fill: parent
                        anchors.margins: 5

                        readonly property int lampSize: 24
                        readonly property int lampGap: 4
                        readonly property int barWidth: 10
                        readonly property int barGap: 3
                        readonly property int segCount: 2
                        readonly property int contentHeight: lampSize * 2 + lampGap
                        readonly property int barPadY: 2

                        // shared vertical centerline for both bar and lamps
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.verticalCenter: parent.verticalCenter
                            width: 1
                            height: parent.height - 4
                            color: "#ffffff"
                            opacity: 0.06
                        }

                        // left vertical multi-segment bar (2 segments)
                        Column {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: statusPillarContent.barGap

                            Repeater {
                                model: statusPillarContent.segCount
                                delegate: Rectangle {
                                    width: statusPillarContent.barWidth
                                    height: Math.max(
                                        6,
                                        (statusPillarContent.height - statusPillarContent.barPadY * 2 - (statusPillarContent.segCount - 1) * statusPillarContent.barGap)
                                            / statusPillarContent.segCount
                                    )
                                    radius: 2
                                    // 颜色: 0,1=绿色; 2=黄色; 3=红色
                                    color: (slot.deviceState === 0 || slot.deviceState === 1) ? "#16a34a" :
                                           (slot.deviceState === 2 ? "#f59e0b" : "#dc2626")
                                    border.width: 1
                                    border.color: "#0b0f14"

                                    // 闪烁动画: 仅状态1(正常使用中)闪烁
                                    SequentialAnimation on opacity {
                                        running: slot.deviceState === 1
                                        loops: Animation.Infinite
                                        NumberAnimation { to: 0.25; duration: 500 }
                                        NumberAnimation { to: 1.0; duration: 500 }
                                        onStopped: parent.opacity = 1.0  // 动画停止时重置
                                    }
                                }
                            }
                        }

                        // right indicator: single lamp that switches between √ and ×
                        Item {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: statusPillarContent.lampSize
                            height: statusPillarContent.lampSize

                            Rectangle {
                                id: stateLamp
                                anchors.fill: parent
                                radius: width / 2
                                // 颜色: 0,1=绿色; 2=黄色; 3=红色
                                color: (slot.deviceState === 0 || slot.deviceState === 1) ? "#16a34a" :
                                       (slot.deviceState === 2 ? "#f59e0b" : "#dc2626")
                                border.width: 1
                                border.color: "#0b0f14"

                                Text {
                                    anchors.centerIn: parent
                                    // 绿色√，黄色！，红色×
                                    text: (slot.deviceState === 0 || slot.deviceState === 1) ? "√" :
                                          (slot.deviceState === 2 ? "!" : "×")
                                    color: "white"
                                    font.pixelSize: 18
                                    font.bold: true
                                }

                                // 闪烁动画: 仅状态1(正常使用中)闪烁
                                SequentialAnimation on opacity {
                                    running: slot.deviceState === 1
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.2; duration: 500 }
                                    NumberAnimation { to: 1.0; duration: 500 }
                                    onStopped: parent.opacity = 1.0  // 动画停止时重置
                                }

                                Behavior on color { ColorAnimation { duration: 180 } }
                            }
                        }
                    }
                }
            }

            // bottom row: name strip (single line)
            Rectangle {
                id: nameStrip
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: topRow.bottom
                anchors.topMargin: 4
                height: slot.nameStripHeight
                radius: 2
                border.width: 1
                border.color: "#cfd6e4"
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#2b3038" }
                    GradientStop { position: 1.0; color: "#1a1f28" }
                }

                Text {
                    anchors.centerIn: parent
                    text: slot.deviceName
                    color: "#eef4ff"
                    font.pixelSize: 11
                    font.bold: true
                    elide: Text.ElideRight
                    width: parent.width - 10
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

}
