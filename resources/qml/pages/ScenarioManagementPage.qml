// ScenarioManagementPage.qml - 主入口文件
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import "scenario"
import NavModels

Item {
    id: root

    property int selectedIndex: -1
    property bool showDetail: false
    property var detailData: ({})
    property string currentDetailSceneId: ""

    // 标记是否正在加载数据，加载期间忽略子组件的更新信号
    property bool isLoading: false

    // 单独的属性来确保数据正确传递
    property string currentSceneType: ""
    property string currentTestType: ""

    // ★ 添加唯一标识符，用于强制刷新 Loader
    property int detailLoadKey: 0

    readonly property color primary: "#1677FF"
    readonly property color danger: "#EF4444"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 场景类型选项
    readonly property var sceneTypeOptions: [
        "中高轨导航终端性能测试",
        "低轨导航终端性能测试",
        "地月空间终端设备性能测试",
        "安全认证测试终端试验",
        "导航用户装备抗干扰性能试验"
    ]
    // 测试类型选项
    readonly property var testTypeOptions: [
        "定位精度", "测速精度", "首次定位时间",
        "干扰定位", "干扰测速", "干扰首次定位时间",
        "抗干扰性能测试", "安全认证性能测试", "地月空间定位能力测试"
    ]

    Component.onCompleted: {
        if (SceneModel && SceneModel.load) SceneModel.load()
    }

    function isJammingType(type) {
        return type && type.indexOf("干扰") >= 0
    }

    // 白色卡片容器阴影
    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        anchors.topMargin: 14
        radius: 10
        color: "#000000"
        opacity: 0.06
        z: -1
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        color: "white"
        radius: 10
        border.width: 1
        border.color: "#E5E7EB"

        // 列表页
        SceneListView {
            id: sceneListView
            anchors.fill: parent
            visible: !root.showDetail
            onOpenDetail: function(sceneId, data) {
                console.log("=== ScenarioManagementPage onOpenDetail ===")
                console.log("  sceneId:", sceneId)
                console.log("  data keys:", Object.keys(data))
                console.log("  data.sceneType:", data.sceneType)
                console.log("  data.testType:", data.testType)
                console.log("  data.channels count:", data.channels ? data.channels.length : "undefined")
                console.log("  data.freqSelect:", data.freqSelect)
                console.log("  data.scenePath:", data.scenePath)
                console.log("  data.trackOutput:", data.trackOutput)
                // 输出前3个通道
                if (data.channels && data.channels.length > 0) {
                    for (var i = 0; i < Math.min(3, data.channels.length); i++) {
                        console.log("  data.channel[" + i + "]:", JSON.stringify(data.channels[i]))
                    }
                }

                // ★ 检查是否是同一个场景且同一种类型，如果是则不需要重新加载
                var isSameScene = root.currentDetailSceneId === sceneId && root.currentSceneType === data.sceneType
                console.log("  isSameScene:", isSameScene)

                // ★ 先隐藏详情页，触发 Loader 卸载组件
                root.showDetail = false
                root.isLoading = true

                // 使用 Timer 延迟设置新数据，确保组件完全卸载
                openDetailTimer.sceneId = sceneId
                openDetailTimer.sceneType = data.sceneType || ""
                openDetailTimer.testType = data.testType || ""
                openDetailTimer.dataToLoad = JSON.parse(JSON.stringify(data))
                openDetailTimer.start()
            }
            onEditScene: function(sceneData, index) {
                root.selectedIndex = index
                sceneEditorDialog.openDialog(sceneData, index)
            }
            onDeleteScene: function(index) {
                root.selectedIndex = index
                deleteConfirmDialog.open()
            }
        }

        // 详情页 - 使用 Loader 强制在切换场景时重新创建组件
        Loader {
            id: sceneDetailLoader
            anchors.fill: parent
            active: root.showDetail
            // ★ 使用 detailLoadKey 作为 key，当 key 变化时强制重新创建组件
            sourceComponent: detailPageComponent

            property string loaderSceneId: root.currentDetailSceneId
            property var loaderDetailData: root.detailData
            property string loaderSceneType: root.currentSceneType
            property string loaderTestType: root.currentTestType
            property bool loaderIsLoading: root.isLoading
        }

        Component {
            id: detailPageComponent
            SceneDetailPage {
                id: sceneDetailPage
                anchors.fill: parent
                sceneId: sceneDetailLoader.loaderSceneId
                detailData: sceneDetailLoader.loaderDetailData
                sceneType: sceneDetailLoader.loaderSceneType
                testType: sceneDetailLoader.loaderTestType
                isLoading: sceneDetailLoader.loaderIsLoading

                Component.onCompleted: {
                    console.log("=== SceneDetailPage (in Loader) Component.onCompleted ===")
                    console.log("  sceneId:", sceneId)
                    console.log("  sceneType:", sceneType)
                    console.log("  testType:", testType)
                    console.log("  detailData keys:", Object.keys(detailData))
                }

                onBack: {
                    root.showDetail = false
                    // 清空 detailData，避免不同场景数据混淆
                    root.detailData = ({})
                    root.currentSceneType = ""
                    root.currentTestType = ""
                    root.isLoading = false
                    // ★ 增加 loadKey 强制下次重新创建组件
                    root.detailLoadKey++
                }
                onSaved: {
                    if (SceneModel && SceneModel.load) SceneModel.load()
                }
            }
        }

        // 延迟加载场景数据的定时器 - 确保先清空旧数据再加载新数据
        Timer {
            id: openDetailTimer
            interval: 50
            property string sceneId: ""
            property string sceneType: ""
            property string testType: ""
            property var dataToLoad: ({})
            onTriggered: {
                console.log("=== openDetailTimer triggered ===")
                console.log("  sceneId:", sceneId)
                console.log("  sceneType:", sceneType)
                console.log("  dataToLoad keys:", Object.keys(dataToLoad))
                console.log("  dataToLoad.channels:", dataToLoad.channels ? dataToLoad.channels.length : "undefined")

                // ★ 检查是否有场景特定的通道数据
                console.log("  dataToLoad.lowOrbitChannels:", dataToLoad.lowOrbitChannels ? dataToLoad.lowOrbitChannels.length : "undefined")
                console.log("  dataToLoad.antiJammingChannels:", dataToLoad.antiJammingChannels ? dataToLoad.antiJammingChannels.length : "undefined")
                console.log("  dataToLoad.securityAuthChannels:", dataToLoad.securityAuthChannels ? dataToLoad.securityAuthChannels.length : "undefined")
                console.log("  dataToLoad.lunarSpaceChannels:", dataToLoad.lunarSpaceChannels ? dataToLoad.lunarSpaceChannels.length : "undefined")

                // ★ 设置新数据
                root.currentDetailSceneId = sceneId
                root.currentSceneType = sceneType
                root.currentTestType = testType
                root.detailData = dataToLoad

                // ★ 增加 loadKey 强制 Loader 重新创建组件
                root.detailLoadKey++

                // ★ 显示详情页
                root.showDetail = true
                root.isLoading = false

                console.log("=== After setting root.detailData in timer ===")
                console.log("  root.detailData keys:", Object.keys(root.detailData))
                console.log("  root.detailData.channels:", root.detailData.channels ? root.detailData.channels.length : "undefined")
            }
        }
    }

    // 编辑弹窗
    SceneEditorDialog {
        id: sceneEditorDialog
        sceneTypeOptions: root.sceneTypeOptions
        testTypeOptions: root.testTypeOptions
        onSaved: {
            if (SceneModel && SceneModel.load) SceneModel.load()
            root.selectedIndex = -1
        }
    }

    // 删除确认弹窗
    DeleteConfirmDialog {
        id: deleteConfirmDialog
        onConfirmed: {
            SceneModel.removeAt(root.selectedIndex)
            if (SceneModel && SceneModel.load) SceneModel.load()
            root.selectedIndex = -1
        }
    }
}