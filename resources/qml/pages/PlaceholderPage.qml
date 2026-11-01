import QtQuick
import QtQuick.Controls

Item {
    id: root
    anchors.fill: parent

    property string title: ""

    Rectangle {
        anchors.fill: parent
        color: "#2b2f36"
    }

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 16
        text: root.title
        color: "white"
        font.pixelSize: 20
        font.bold: true
    }
}
