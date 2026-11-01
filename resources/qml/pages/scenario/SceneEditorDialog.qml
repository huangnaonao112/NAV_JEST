// SceneEditorDialog.qml - 场景编辑弹窗
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NavModels

Popup {
    id: root
    
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    parent: Overlay.overlay
    width: 400
    height: contentItem.implicitHeight
    
    property int editIndex: -1
    property var sceneTypeOptions: [
        "中高轨导航终端性能测试",
        "低轨导航终端性能测试",
        "地月空间终端设备性能测试",
        "安全认证测试终端试验",
        "导航用户设备抗干扰性能试验"
    ]
    property var testTypeOptions: [
        "定位精度", "测速精度", "首次定位时间",
        "干扰定位", "干扰测速", "干扰首次定位时间",
        "抗干扰性能测试", "安全认证性能测试", "地月空间定位能力测试"
    ]

    signal saved()

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 警告对话框
    Dialog {
        id: warnDialog
        anchors.centerIn: parent
        title: "警告"
        modal: true
        standardButtons: Dialog.Ok
        property string message: ""
        contentItem: Text {
            text: warnDialog.message
            color: "#1F2937"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }
        background: Rectangle {
            color: "white"
            radius: 8
            border.color: "#E5E7EB"
            border.width: 1
        }
    }

    onOpened: {
        if (parent) {
            x = Math.round((parent.width - width) / 2)
            y = Math.round((parent.height - height) / 2)
        }
    }

    function openDialog(sceneData, index) {
        editIndex = index
        if (sceneData) {
            sceneIdField.text = sceneData.sceneId || ""
            sceneNameField.text = sceneData.sceneName || ""
            sceneTypeCombo.currentIndex = Math.max(0, sceneTypeOptions.indexOf(sceneData.sceneType || ""))
            accessDeviceCombo.currentIndex = Math.max(0, SceneModel.deviceTypeList().indexOf(sceneData.accessDevice || ""))
            testTypeCombo.currentIndex = Math.max(0, testTypeOptions.indexOf(sceneData.testType || "定位精度"))
            descField.text = sceneData.description || ""
            editorTitle.text = "编辑场景"
        } else {
            sceneIdField.text = ""
            sceneNameField.text = ""
            sceneTypeCombo.currentIndex = 0
            accessDeviceCombo.currentIndex = 0
            testTypeCombo.currentIndex = 0
            descField.text = ""
            editorTitle.text = "添加场景"
        }
        root.open()
    }

    function doSubmit() {
        // 验证所有字段不能为空
        var sceneId = sceneIdField.text.trim()
        var sceneName = sceneNameField.text.trim()
        var sceneType = sceneTypeCombo.currentText
        var accessDevice = accessDeviceCombo.currentText
        var testType = testTypeCombo.currentText
        var description = descField.text.trim()

        if (!sceneId) {
            warnDialog.message = "场景编号不能为空！"
            warnDialog.open()
            return
        }
        if (!sceneName) {
            warnDialog.message = "场景名称不能为空！"
            warnDialog.open()
            return
        }
        if (!sceneType) {
            warnDialog.message = "请选择场景类型！"
            warnDialog.open()
            return
        }
        if (!accessDevice) {
            warnDialog.message = "请选择接入设备！"
            warnDialog.open()
            return
        }
        if (!testType) {
            warnDialog.message = "请选择测试类型！"
            warnDialog.open()
            return
        }
        if (!description) {
            warnDialog.message = "场景描述不能为空！"
            warnDialog.open()
            return
        }

        if (editIndex < 0) {
            if (!SceneModel.add(sceneId, sceneName, sceneType, accessDevice, testType, description)) {
                warnDialog.message = "添加场景失败，场景编号可能已存在！"
                warnDialog.open()
                return
            }
        } else {
            if (!SceneModel.update(editIndex, sceneId, sceneName, sceneType, accessDevice, testType, description)) {
                warnDialog.message = "修改场景失败！"
                warnDialog.open()
                return
            }
        }

        root.saved()
        root.close()
    }

    background: Rectangle {
        color: "#E8F0FE"
        radius: 14
        border.width: 1
        border.color: "#BBDEFB"
    }

    contentItem: Column {
        width: root.width
        spacing: 0

        // 标题栏
        Rectangle {
            width: parent.width
            height: 48
            color: root.primary
            radius: 14
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 14
                color: root.primary
            }
            
            Text {
                id: editorTitle
                anchors.left: parent.left
                anchors.leftMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                text: "添加场景"
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }
            
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                text: "×"
                color: "white"
                font.pixelSize: 22
                font.bold: true
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        // 表单内容
        Column {
            width: parent.width
            leftPadding: 20
            rightPadding: 20
            topPadding: 20
            bottomPadding: 20
            spacing: 16

            Grid {
                columns: 2
                columnSpacing: 16
                rowSpacing: 14

                Text { text: "场景编号"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                TextField {
                    id: sceneIdField
                    width: 240
                    color: root.text1
                    font.pixelSize: 13
                    placeholderText: "例如 DYPOS（可修改）"
                    background: Rectangle { color: "white"; border.color: "#BBDEFB"; border.width: 1; radius: 6 }
                }
                
                Text { text: "场景类型"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                ComboBox {
                    id: sceneTypeCombo
                    width: 240
                    height: 32
                    implicitHeight: 32
                    font.pixelSize: 13
                    model: root.sceneTypeOptions
                    delegate: ItemDelegate {
                        width: sceneTypeCombo.width
                        contentItem: Text {
                            text: modelData
                            color: (sceneTypeCombo.highlightedIndex === index) ? "white" : root.text1
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        background: Rectangle {
                            radius: 6
                            color: (sceneTypeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                        }
                    }
                    contentItem: Text {
                        leftPadding: 10
                        rightPadding: 28
                        text: sceneTypeCombo.displayText
                        color: root.text1
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    indicator: Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "▾"
                        color: root.text2
                        font.pixelSize: 14
                    }
                    background: Rectangle {
                        color: "white"
                        border.color: "#BBDEFB"
                        border.width: 1
                        radius: 6
                    }
                    popup: Popup {
                        y: sceneTypeCombo.height + 4
                        width: sceneTypeCombo.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 6
                        background: Rectangle {
                            color: "white"
                            radius: 8
                            border.width: 1
                            border.color: "#BBDEFB"
                        }
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: sceneTypeCombo.delegateModel
                        }
                    }
                }
                
                Text { text: "场景名称"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                TextField {
                    id: sceneNameField
                    width: 240
                    color: root.text1
                    font.pixelSize: 13
                    placeholderText: "请输入场景名称"
                    background: Rectangle { color: "white"; border.color: "#BBDEFB"; border.width: 1; radius: 6 }
                }
                
                Text { text: "接入设备"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                ComboBox {
                    id: accessDeviceCombo
                    width: 240
                    height: 32
                    implicitHeight: 32
                    font.pixelSize: 13
                    model: SceneModel ? SceneModel.deviceTypeList() : []
                    delegate: ItemDelegate {
                        width: accessDeviceCombo.width
                        contentItem: Text {
                            text: modelData
                            color: (accessDeviceCombo.highlightedIndex === index) ? "white" : root.text1
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 6
                            color: (accessDeviceCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                        }
                    }
                    contentItem: Text {
                        leftPadding: 10
                        rightPadding: 28
                        text: accessDeviceCombo.displayText
                        color: root.text1
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "▾"
                        color: root.text2
                        font.pixelSize: 14
                    }
                    background: Rectangle {
                        color: "white"
                        border.color: "#BBDEFB"
                        border.width: 1
                        radius: 6
                    }
                    popup: Popup {
                        y: accessDeviceCombo.height + 4
                        width: accessDeviceCombo.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 6
                        background: Rectangle {
                            color: "white"
                            radius: 8
                            border.width: 1
                            border.color: "#BBDEFB"
                        }
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: accessDeviceCombo.delegateModel
                        }
                    }
                }
                
                Text { text: "测试类型"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                ComboBox {
                    id: testTypeCombo
                    width: 240
                    height: 32
                    implicitHeight: 32
                    font.pixelSize: 13
                    model: root.testTypeOptions
                    delegate: ItemDelegate {
                        width: testTypeCombo.width
                        contentItem: Text {
                            text: modelData
                            color: (testTypeCombo.highlightedIndex === index) ? "white" : root.text1
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 6
                            color: (testTypeCombo.highlightedIndex === index) ? root.primary : (hovered ? "#EEF2FF" : "transparent")
                        }
                    }
                    contentItem: Text {
                        leftPadding: 10
                        rightPadding: 28
                        text: testTypeCombo.displayText
                        color: root.text1
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                    indicator: Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: "▾"
                        color: root.text2
                        font.pixelSize: 14
                    }
                    background: Rectangle {
                        color: "white"
                        border.color: "#BBDEFB"
                        border.width: 1
                        radius: 6
                    }
                    popup: Popup {
                        y: testTypeCombo.height + 4
                        width: testTypeCombo.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 6
                        background: Rectangle {
                            color: "white"
                            radius: 8
                            border.width: 1
                            border.color: "#BBDEFB"
                        }
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: testTypeCombo.delegateModel
                        }
                    }
                }
                
                Text { text: "场景描述"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                TextField {
                    id: descField
                    width: 240
                    color: root.text1
                    font.pixelSize: 13
                    placeholderText: "请输入场景描述"
                    background: Rectangle { color: "white"; border.color: "#BBDEFB"; border.width: 1; radius: 6 }
                }
            }

            RowLayout {
                width: parent.width - 40
                spacing: 12

                Button {
                    text: "取消"
                    flat: true
                    font.pixelSize: 13
                    implicitWidth: 72
                    implicitHeight: 32
                    contentItem: Text {
                        text: parent.text
                        color: root.text2
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }
                    background: Rectangle {
                        color: parent.pressed ? "#E5E7EB" : (parent.hovered ? "#F6F7F9" : "transparent")
                        radius: 6
                    }
                    onClicked: root.close()
                }

                Item { Layout.fillWidth: true; height: 1 }

                Button {
                    text: root.editIndex < 0 ? "添加" : "保存"
                    font.pixelSize: 13
                    implicitWidth: 72
                    implicitHeight: 32
                    contentItem: Text {
                        text: parent.text
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 13
                    }
                    background: Rectangle {
                        color: parent.pressed ? "#0958d9" : (parent.hovered ? "#4096ff" : root.primary)
                        radius: 6
                    }
                    onClicked: root.doSubmit()
                }
            }
        }
    }
}