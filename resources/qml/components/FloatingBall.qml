import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 60
    height: 60
    
    signal clicked()

    Rectangle {
        id: ball
        anchors.fill: parent
        radius: width / 2
        color: "#00AAFF"
        border.color: "white"
        border.width: 2
        
        opacity: ma.containsMouse ? 1.0 : 0.7
        scale: ma.pressed ? 0.9 : (ma.containsMouse ? 1.1 : 1.0)
        
        Behavior on opacity { NumberAnimation { duration: 200 } }
        Behavior on scale { NumberAnimation { duration: 200 } }

        Text {
            anchors.centerIn: parent
            text: "🛰️"
            font.pixelSize: 30
        }

        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            
            property bool dragActive: false
            property point pressPos
            
            onPressed: (mouse) => {
                pressPos = Qt.point(mouse.x, mouse.y)
                dragActive = false
            }
            
            onPositionChanged: (mouse) => {
                if (pressed && !dragActive) {
                    if (Math.abs(mouse.x - pressPos.x) > 5 || Math.abs(mouse.y - pressPos.y) > 5) {
                        dragActive = true
                        // When dragging starts, clear anchors to allow free movement
                        root.anchors.right = undefined
                        root.anchors.bottom = undefined
                        root.anchors.left = undefined
                        root.anchors.top = undefined
                    }
                }
            }
            
            onClicked: {
                if (!dragActive) {
                    root.clicked()
                }
            }
            
            drag.target: root
            drag.axis: Drag.XAndYAxis
        }

        layer.enabled: true
    }
}
