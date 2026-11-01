// StepNavigation.qml - 步骤导航
import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    
    property int currentIndex: 0
    property string sceneType: ""
    
    radius: 10
    color: "#F6F7F9"
    border.width: 1
    border.color: "#BBDEFB"
    
    readonly property color primary: "#1677FF"
    readonly property color text1: "#1F2937"
    
    // 普通场景步骤名
    readonly property var stepNamesNormal: [
        "\u4EFF\u771F\u5F00\u59CB",
        "\u6A21\u62DF\u5668\u914D\u7F6E",
        "\u5F85\u6D4B\u8BBE\u5907\u914D\u7F6E",
        "\u8BC4\u4F30\u914D\u7F6E"
    ]

    // 中高轨场景步骤名
    readonly property var stepNamesMediumOrbit: [
        "\u573A\u666F\u914D\u7F6E",
        "\u6A21\u62DF\u5668\u914D\u7F6E",
        "\u5F85\u6D4B\u8BBE\u5907\u914D\u7F6E",
        "\u8BC4\u4F30\u914D\u7F6E"
    ]
    
    // 根据场景类型返回步骤名列表
    property var stepNames: {
        if (root.sceneType === "\u4E2D\u9AD8\u8F68\u5BFC\u822A\u7EC8\u7AEF\u6027\u80FD\u6D4B\u8BD5") {
            return stepNamesMediumOrbit
        } else {
            return stepNamesNormal
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Text { 
            text: "\u6D4B\u8BD5\u6B65\u9AA4"
            color: root.text1
            font.pixelSize: 14
            font.bold: true
        }
        
        Repeater {
            model: root.stepNames
            
            delegate: Rectangle {
                width: parent.width
                height: 34
                radius: 8
                color: index === root.currentIndex ? root.primary : "white"
                border.width: 1
                border.color: index === root.currentIndex ? "transparent" : "#E5E7EB"
                
                Text {
                    anchors.centerIn: parent
                    text: modelData
                    color: index === root.currentIndex ? "white" : root.text1
                    font.pixelSize: 13
                    font.bold: index === root.currentIndex
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentIndex = index
                }
            }
        }
    }
}