// SceneListView.qml - 场景列表视图
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NavModels

Item {
    id: root
    
    signal openDetail(string sceneId, var detailData)
    signal editScene(var sceneData, int index)
    signal deleteScene(int index)
    
    property int selectedIndex: -1
    
    readonly property color primary: "#1677FF"
    readonly property color danger: "#EF4444"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // 顶部操作按钮
        Row {
            spacing: 12

            Button {
                text: "\u6DFB\u52A0"
                implicitWidth: 72
                implicitHeight: 32
                background: Rectangle {
                    color: parent.pressed ? "#0958d9" : (parent.hovered ? "#4096ff" : root.primary)
                    radius: 8
                }
                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 13
                }
                onClicked: root.editScene(null, -1)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#E5E7EB"
        }

        // 表格
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: "white"
            border.width: 1
            border.color: "#E5E7EB"
            clip: true

            ListView {
                id: sceneTable
                anchors.fill: parent
                clip: true
                model: SceneModel
                reuseItems: true
                cacheBuffer: 200

                readonly property int colSpacing: 12
                readonly property int totalSpacing: 8 * colSpacing
                readonly property int availableWidth: width - 24 - totalSpacing
                
                readonly property int colOp: Math.max(100, Math.floor(availableWidth * 0.18))
                readonly property int otherWidth: availableWidth - colOp
                
                readonly property int colIndex: Math.max(40, Math.floor(otherWidth * 0.06))
                readonly property int colSceneId: Math.max(75, Math.floor(otherWidth * 0.11))
                readonly property int colSceneType: Math.max(120, Math.floor(otherWidth * 0.19))
                readonly property int colSceneName: Math.max(100, Math.floor(otherWidth * 0.14))
                readonly property int colDevice: Math.max(80, Math.floor(otherWidth * 0.12))
                readonly property int colTestType: Math.max(80, Math.floor(otherWidth * 0.12))
                readonly property int colDesc: Math.max(100, Math.floor(otherWidth * 0.13))
                readonly property int colUpdated: Math.max(90, Math.floor(otherWidth * 0.13))

                header: Rectangle {
                    width: sceneTable.width
                    height: 44
                    color: "#F6F7F9"

                    Row {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: sceneTable.colSpacing

                        Text { text: "\u5E8F\u53F7"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colIndex; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u573A\u666F\u7F16\u53F7"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colSceneId; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u573A\u666F\u7C7B\u578B"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colSceneType; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u573A\u666F\u540D\u79F0"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colSceneName; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u63A5\u5165\u8BBE\u5907"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colDevice; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u6D4B\u8BD5\u7C7B\u578B"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colTestType; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u573A\u666F\u63CF\u8FF0"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colDesc; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u66F4\u65B0\u65F6\u95F4"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colUpdated; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: "\u64CD\u4F5C"; color: root.text2; font.pixelSize: 13; font.bold: true; width: sceneTable.colOp; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                    }
                }

                delegate: Rectangle {
                    id: delegateItem
                    width: sceneTable.width
                    height: 44
                    property color bgColor: "white"
                    color: bgColor
                    border.width: 1
                    border.color: "#E5E7EB"
                    
                    function updateColor() {
                        bgColor = ListView.isCurrentItem ? "#EAF2FF" : "white"
                    }
                    
                    Component.onCompleted: updateColor()

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            sceneTable.currentIndex = index
                            root.selectedIndex = index
                        }
                        onEntered: if (!ListView.isCurrentItem) delegateItem.bgColor = "#F5F7FF"
                        onExited: delegateItem.updateColor()
                    }

                    Row {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: sceneTable.colSpacing

                        Text { text: (index + 1).toString(); color: root.text3; width: sceneTable.colIndex; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                        Text { text: sceneId; color: root.text1; width: sceneTable.colSceneId; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: sceneType; color: root.text1; width: sceneTable.colSceneType; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: sceneName; color: root.text1; width: sceneTable.colSceneName; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: accessDevice; color: root.text2; width: sceneTable.colDevice; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: testType; color: root.text2; width: sceneTable.colTestType; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: description; color: root.text2; width: sceneTable.colDesc; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                        Text { text: updatedAt; color: root.text3; width: sceneTable.colUpdated; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }

                        Item {
                            width: sceneTable.colOp
                            height: parent.height
Row {
                                anchors.centerIn: parent
                                height: parent.height
                                spacing: 6

                                Text {
                                    text: "\u7F16\u8F91"
                                    color: root.primary
                                    font.pixelSize: 13
                                    width: 36
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.selectedIndex = index
                                            const d = SceneModel.get(index)
                                            root.editScene(d, index)
                                        }
                                    }
                                }

                                Text {
                                    text: "\u5220\u9664"
                                    color: root.danger
                                    font.pixelSize: 13
                                    width: 36
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.selectedIndex = index
                                            root.deleteScene(index)
                                        }
                                    }
                                }

                                Text {
                                    text: "\u8BE6\u60C5"
                                    color: root.primary
                                    font.pixelSize: 13
                                    width: 36
                                    height: parent.height
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignHCenter
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            root.selectedIndex = index
                                            const d = SceneModel.get(index)
                                            console.log("=== SceneListView onClick ===")
                                            console.log("  List data sceneId:", d.sceneId)
                                            console.log("  List data sceneType:", d.sceneType)
                                            console.log("  List data testType:", d.testType)

                                            // 记录QML请求日志
                                            if (typeof SceneModel !== 'undefined' && SceneModel.logQmlMessage) {
                                                SceneModel.logQmlMessage("QML_CLICK", "Detail button clicked for sceneId=" + d.sceneId)
                                            }

                                            var rawDetail = SceneModel.loadDetail(d.sceneId)
                                            // 深拷贝数据，避免直接修改 C++ 模型返回的原始对象，防止场景间数据污染
                                            var loadedData = JSON.parse(JSON.stringify(rawDetail))
                                            console.log("  loadDetail returned keys:", Object.keys(loadedData))
                                            console.log("  loadDetail sceneType:", loadedData.sceneType)
                                            console.log("  loadDetail testType:", loadedData.testType)
                                            console.log("  loadDetail channels count:", loadedData.channels ? loadedData.channels.length : 0)
                                            console.log("  loadDetail freqSelect:", loadedData.freqSelect)
                                            console.log("  loadDetail scenePath:", loadedData.scenePath)

                                            // 记录接收到的数据日志
                                            if (typeof SceneModel !== 'undefined' && SceneModel.logQmlRefresh) {
                                                SceneModel.logQmlRefresh("SceneListView", "freqSelect", loadedData.freqSelect || "undefined", "DATA_RECEIVED")
                                                SceneModel.logQmlRefresh("SceneListView", "scenePath", loadedData.scenePath || "undefined", "DATA_RECEIVED")
                                                SceneModel.logQmlRefresh("SceneListView", "channels.count", loadedData.channels ? loadedData.channels.length : 0, "DATA_RECEIVED")
                                            }

                                            // 如果从数据库加载的 sceneType 为空，使用列表数据
                                            if (!loadedData.sceneType) {
                                                console.log("  WARNING: sceneType is empty from DB, using list data")
                                                loadedData.sceneType = d.sceneType
                                            }
                                            if (!loadedData.testType) {
                                                loadedData.testType = d.testType
                                            }
                                            loadedData.sceneName = d.sceneName

                                            console.log("  Final data sceneType:", loadedData.sceneType)
                                            console.log("  Final data channels count:", loadedData.channels ? loadedData.channels.length : 0)

                                            root.openDetail(d.sceneId, loadedData)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                text: "\u6682\u65E0\u6570\u636E"
                color: root.text3
                visible: sceneTable.count === 0
            }
        }
    }
}