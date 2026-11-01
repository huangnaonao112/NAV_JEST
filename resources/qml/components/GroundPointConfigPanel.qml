import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NavModels

Rectangle {
    id: root
    width: 300
    height: mainLayout.implicitHeight + 24
    color: "#CC112233"
    radius: 12
    border.color: "#00FFFF"
    border.width: 1
    clip: true

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: (mouse) => mouse.accepted = true
        onWheel: (wheel) => wheel.accepted = true
    }

    property int currentIndex: 0

    function loadData() {
        var data = SatelliteModel.getGroundPoint(currentIndex);
        if (Object.keys(data).length === 0) return;
        
        inputLat.value = data.lat.toFixed(4);
        inputLon.value = data.lon.toFixed(4);
        inputName.text = data.name;
        inputRadius.value = data.radius.toFixed(0);
        inputError.value = (data.errorRadius || 0).toFixed(0);
        typeSelector.currentIndex = data.type;
        root.currentColor = data.color;
    }

    function applyData() {
        SatelliteModel.updateGroundPoint(
            currentIndex,
            parseFloat(inputLat.value),
            parseFloat(inputLon.value),
            inputName.text,
            typeSelector.currentIndex,
            parseFloat(inputRadius.value),
            root.currentColor,
            parseFloat(inputError.value)
        );
    }

    property color currentColor: "#00FFFF"

    Component.onCompleted: loadData()
    onCurrentIndexChanged: loadData()

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "地面点位配置"
                color: "white"
                font.bold: true
                font.pixelSize: 16
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "×"
                flat: true
                onClicked: root.visible = false
                font.pixelSize: 18
                palette.buttonText: "white"
            }
        }

        // 点位选择
        ComboBox {
            id: pointSelector
            Layout.fillWidth: true
            model: SatelliteModel.groundPoints
            textRole: "name"
            currentIndex: root.currentIndex
            onActivated: (index) => root.currentIndex = index
            
            delegate: ItemDelegate {
                width: pointSelector.width
                contentItem: Text {
                    text: modelData.name || ("点位 " + (index + 1))
                    color: "white"
                }
                background: Rectangle {
                    color: hovered ? "#3300FFFF" : "transparent"
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#33FFFFFF" }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            RowLayout {
                Text { text: "名称:"; color: "#88FFFFFF"; Layout.preferredWidth: 60 }
                TextField {
                    id: inputName
                    Layout.fillWidth: true
                    placeholderText: "点位名称"
                    color: "white"
                    background: Rectangle { color: "#22FFFFFF"; radius: 4 }
                    onEditingFinished: applyData()
                }
            }

            ParameterInput {
                id: inputLat
                label: "纬度 (Lat):"
                onEditingFinished: applyData()
            }

            ParameterInput {
                id: inputLon
                label: "经度 (Lon):"
                onEditingFinished: applyData()
            }

            RowLayout {
                Text { text: "类型:"; color: "#88FFFFFF"; Layout.preferredWidth: 60 }
                ComboBox {
                    id: typeSelector
                    Layout.fillWidth: true
                    model: ["基站", "干扰源", "目标", "轨迹", "欺骗位置"]
                    onActivated: applyData()
                }
            }

            ParameterInput {
                id: inputRadius
                label: "半径 (km):"
                onEditingFinished: applyData()
            }

            ParameterInput {
                id: inputError
                label: "误差 (km):"
                visible: typeSelector.currentIndex === 2
                onEditingFinished: applyData()
            }

            RowLayout {
                Text { text: "颜色:"; color: "#88FFFFFF"; Layout.preferredWidth: 60 }
                Row {
                    spacing: 8
                    Repeater {
                        model: ["#00FFFF", "#FF3333", "#00FF00", "#FFAA00", "#FF00FF"]
                        Rectangle {
                            width: 20; height: 20; radius: 10
                            color: modelData
                            border.color: root.currentColor == modelData ? "white" : "transparent"
                            border.width: 2
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    root.currentColor = modelData
                                    applyData()
                                }
                            }
                        }
                    }
                }
            }
        }

        Button {
            Layout.fillWidth: true
            text: "添加新点位"
            onClicked: {
                SatelliteModel.addGroundPoint(0, 0, "新点位", 0, 500, "#00FFFF", 0);
                root.currentIndex = SatelliteModel.groundPoints.length - 1;
            }
        }
    }
}
