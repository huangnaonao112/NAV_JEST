// SceneDetailPage.qml - 场景详情页
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import "steps" as Steps
import NavModels

Item {
    id: root

    property string sceneId: ""
    property var detailData: ({})
    property string sceneType: ""
    property string testType: ""

    // 标记是否正在加载数据，加载期间忽略子组件的更新信号
    property bool isLoading: false

    // 标记组件是否已初始化完成
    property bool isInitialized: false

    signal back()
    signal saved()

    // 信号：子组件发射此信号来通知父组件更新数据
    signal updateDetailData(string key, var value)

    // 处理子组件的数据更新信号
    onUpdateDetailData: function(key, value) {
        if (!root.isInitialized || root.isLoading) return

        var newData = JSON.parse(JSON.stringify(root.detailData))
        newData[key] = value
        root.detailData = newData
    }

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    Component.onCompleted: {
        // 不在这里设置 isInitialized = true，等待数据加载完成后再设置
    }

    onDetailDataChanged: {
        // 如果数据为空（切换场景时的清空操作），重置初始化标志
        if (Object.keys(root.detailData).length === 0) {
            root.isInitialized = false
            return
        }

        // 同步 sceneType 和 testType 到 root 属性
        if (root.detailData.sceneType && root.detailData.sceneType !== root.sceneType) {
            root.sceneType = root.detailData.sceneType
        }
        if (root.detailData.testType && root.detailData.testType !== root.testType) {
            root.testType = root.detailData.testType
        }

        // 如果数据包含有效内容（从数据库加载），标记初始化完成
        if (root.detailData.sceneType && Object.keys(root.detailData).length > 0) {
            root.isInitialized = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 16
        anchors.margins: 16

        // 顶部标题栏
        Rectangle {
            width: parent.width
            height: 56
            radius: 10
            color: root.primary

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: "‹"
                    color: "white"
                    font.pixelSize: 24
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillHeight: true
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.back()
                    }
                }

                Text {
                    id: titleText
                    text: {
                        var st = root.sceneType
                        var tt = root.testType
                        if (st && tt) return st + " - " + tt
                        if (st) return st
                        if (tt) return tt
                        return "场景详情"
                    }
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Item { width: 1; height: 1; Layout.fillWidth: true }

                Button {
                    text: "保存"
                    implicitWidth: 72
                    implicitHeight: 32
                    Layout.alignment: Qt.AlignVCenter
                    background: Rectangle {
                        color: parent.pressed ? "#0958d9" : (parent.hovered ? "#4096ff" : "white")
                        radius: 6
                    }
                    contentItem: Text {
                        text: parent.text
                        color: root.primary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }
                    onClicked: saveDetail()
                }
            }
        }

        Row {
            width: parent.width
            height: parent.height - 56 - 16
            spacing: 16

            // 左侧步骤导航
            Steps.StepNavigation {
                id: stepNavigation
                width: 220
                height: parent.height
                sceneType: root.sceneType
            }

            // 右侧参数表单
            Rectangle {
                width: parent.width - 220 - 16
                height: parent.height
                radius: 10
                color: "white"
                border.width: 1
                border.color: "#BBDEFB"

                Column {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Text { text: "步骤参数"; color: root.text1; font.pixelSize: 14; font.bold: true }

                    // 步骤参数容器 - 直接显示所有步骤，用 currentIndex 控制
                    StackLayout {
                        id: stepsStackLayout
                        width: parent.width
                        height: parent.height - 34
                        currentIndex: stepNavigation.currentIndex

                        Steps.Step0_SimulationStart {
                            id: step0
                            detailData: root.detailData
                            sceneId: root.sceneId
                            onUpdateDetailData: function(key, value) {
                                root.updateDetailData(key, value)
                            }
                        }

                        Steps.Step1_SimulatorConfig {
                            id: step1
                            detailData: root.detailData
                            sceneType: root.sceneType
                            onUpdateDetailData: function(key, value) {
                                root.updateDetailData(key, value)
                            }
                        }

                        Steps.Step2_DeviceConfig {
                            id: step2
                            detailData: root.detailData
                            onUpdateDetailData: function(key, value) {
                                root.updateDetailData(key, value)
                            }
                        }

                        Steps.Step3_Evaluation {
                            id: step3
                            detailData: root.detailData
                            onUpdateDetailData: function(key, value) {
                                root.updateDetailData(key, value)
                            }
                        }
                    }
                }

                // 保存成功提示
                Rectangle {
                    id: saveSuccessHint
                    visible: false
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 20
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 120
                    height: 36
                    radius: 6
                    color: "#22C55E"
                    Text {
                        anchors.centerIn: parent
                        text: "保存成功"
                        color: "white"
                        font.pixelSize: 13
                    }
                }
            }
        }
    }

    Timer {
        id: saveSuccessHintTimer
        interval: 2000
        onTriggered: saveSuccessHint.visible = false
    }

    // 文件对话框
    FileDialog {
        id: simSceneFileDialog
        title: "选择场景配置文件"
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON配置文件 (*.json)", "所有文件 (*)"]
        onAccepted: {
            // 更新 detailData
        }
    }

    FileDialog {
        id: resultPathDialog
        title: "选择结果存储位置"
        fileMode: FileDialog.SaveFile
        nameFilters: ["所有文件 (*)"]
        onAccepted: {
            // 更新 detailData
        }
    }

    // 保存详情
    function saveDetail() {
        var detailToSave = JSON.parse(JSON.stringify(root.detailData))
        detailToSave.testType = root.testType
        detailToSave.sceneType = root.sceneType

        if (SceneModel.saveDetail(root.sceneId, detailToSave)) {
            saveSuccessHint.visible = true
            saveSuccessHintTimer.start()

            // 保存成功后重新加载，用 isLoading 防止子组件回写
            root.isLoading = true
            var reloadedData = SceneModel.loadDetail(root.sceneId)
            if (reloadedData && Object.keys(reloadedData).length > 0) {
                root.detailData = reloadedData
            }
            root.isLoading = false

            root.saved()
        }
    }
}
