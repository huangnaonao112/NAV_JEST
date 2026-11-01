// Step0_SimulationStart.qml - 步骤0：仿真开始
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import NavModels

Item {
    id: root

    property var detailData: ({})
    property string sceneId: ""

    signal updateDetailData(string key, var value)

    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Flickable {
            id: flickable
            anchors.fill: parent
            clip: true
            contentHeight: contentColumn.height + 20
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentColumn
                width: flickable.width - 16
                anchors.left: parent.left
                anchors.leftMargin: 8
                spacing: 16

                // 开始时间
                Row {
                    width: parent.width
                    spacing: 10
                    Text {
                        text: "开始时间"
                        color: root.text2
                        font.pixelSize: 13
                        width: 80
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                    }
                    TextField {
                        id: startTimeField
                        text: root.detailData.startTime || "2025/12/16 12:00:00"
                        width: Math.max(260, Math.min(420, parent.width - 120))
                        height: 32
                        color: root.text1
                        font.pixelSize: 13
                        background: Rectangle {
                            color: "white"
                            radius: 6
                            border.width: 1
                            border.color: "#BBDEFB"
                        }
                        onEditingFinished: {
                            if (root.detailData && text !== (root.detailData.startTime || "")) {
                                root.updateDetailData("startTime", text)
                            }
                        }
                    }
                }

                // 结束时间
                Row {
                    width: parent.width
                    spacing: 10
                    Text {
                        text: "结束时间"
                        color: root.text2
                        font.pixelSize: 13
                        width: 80
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                    }
                    TextField {
                        id: endTimeField
                        text: root.detailData.endTime || "2025/12/16 14:00:00"
                        width: Math.max(260, Math.min(420, parent.width - 120))
                        height: 32
                        color: root.text1
                        font.pixelSize: 13
                        background: Rectangle {
                            color: "white"
                            radius: 6
                            border.width: 1
                            border.color: "#BBDEFB"
                        }
                        onEditingFinished: {
                            if (root.detailData && text !== (root.detailData.endTime || "")) {
                                root.updateDetailData("endTime", text)
                            }
                        }
                    }
                }

                // 场景存储
                Row {
                    width: parent.width
                    spacing: 10
                    Text {
                        text: "场景存储"
                        color: root.text2
                        font.pixelSize: 13
                        width: 80
                        height: 32
                        verticalAlignment: Text.AlignVCenter
                    }
                    TextField {
                        id: scenePathField
                        text: root.detailData.scenePath || ""
                        width: Math.max(260, Math.min(420, parent.width - 210))
                        height: 32
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: ""
                        readOnly: true
                        background: Rectangle {
                            color: "#F5F5F5"
                            radius: 6
                            border.width: 1
                            border.color: "#BBDEFB"
                        }
                    }
                    Button {
                        text: "浏览"
                        implicitWidth: 70
                        implicitHeight: 32
                        background: Rectangle {
                            color: root.primary
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 13
                        }
                        onClicked: {
                            if (scenePathField.text !== "") {
                                Qt.openUrlExternally("file:///" + scenePathField.text)
                            }
                        }
                    }
                    Button {
                        text: "导入"
                        implicitWidth: 70
                        implicitHeight: 32
                        background: Rectangle {
                            color: "#FA8C16"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 13
                        }
                        onClicked: importFileDialog.open()
                    }
                    Button {
                        text: "导出"
                        implicitWidth: 70
                        implicitHeight: 32
                        background: Rectangle {
                            color: "#52C41A"
                            radius: 6
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 13
                        }
                        onClicked: {
                            exportFileDialog.open()
                        }
                    }
                }
            }
        }
    }

    FileDialog {
        id: importFileDialog
        title: "导入场景配置"
        fileMode: FileDialog.OpenFile
        nameFilters: ["JSON文件 (*.json)", "所有文件 (*)"]
        onAccepted: {
            var filePath = selectedFile.toString()
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }

            if (typeof SceneModel !== 'undefined' && SceneModel.importFromJson) {
                var importedData = SceneModel.importFromJson(filePath)
                if (importedData && Object.keys(importedData).length > 0) {
                    startTimeField.text = importedData.startTime || startTimeField.text
                    endTimeField.text = importedData.endTime || endTimeField.text
                    scenePathField.text = importedData.scenePath || scenePathField.text

                    for (var key in importedData) {
                        if (importedData.hasOwnProperty(key)) {
                            root.updateDetailData(key, importedData[key])
                        }
                    }

                    if (root.sceneId) {
                        var dataToSave = JSON.parse(JSON.stringify(root.detailData))
                        dataToSave.sceneType = root.detailData.sceneType || importedData.sceneType || ""
                        dataToSave.testType = root.detailData.testType || importedData.testType || ""
                        SceneModel.saveDetail(root.sceneId, dataToSave)
                    }
                }
            }
        }
    }

    FileDialog {
        id: exportFileDialog
        title: "导出场景配置"
        fileMode: FileDialog.SaveFile
        nameFilters: ["JSON文件 (*.json)", "所有文件 (*)"]
        defaultSuffix: ".json"
        onAccepted: {
            var filePath = selectedFile.toString()
            if (filePath.startsWith("file:///")) {
                filePath = filePath.substring(8)
            }

            root.updateDetailData("startTime", startTimeField.text)
            root.updateDetailData("endTime", endTimeField.text)
            root.updateDetailData("scenePath", scenePathField.text)

            if (typeof SceneModel !== 'undefined' && root.sceneId) {
                var dataToSave = JSON.parse(JSON.stringify(root.detailData))
                dataToSave.sceneType = root.detailData.sceneType || ""
                dataToSave.testType = root.detailData.testType || ""

                SceneModel.saveDetail(root.sceneId, dataToSave)

                var loadedData = SceneModel.loadDetail(root.sceneId)
                if (loadedData && Object.keys(loadedData).length > 0) {
                    SceneModel.exportToJson(filePath, loadedData)
                } else {
                    SceneModel.exportToJson(filePath, root.detailData)
                }
            }
        }
    }
}
