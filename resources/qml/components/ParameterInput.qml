import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property alias label: lbl.text
    property alias value: field.text
    signal editingFinished()
    width: parent.width
    
    Text { 
        id: lbl
        color: "#AAFFFFFF"
        Layout.preferredWidth: 100 
        font.pixelSize: 13
    }
    
    TextField {
        id: field
        Layout.fillWidth: true
        height: 30
        onEditingFinished: parent.editingFinished()
        background: Rectangle {
            color: "#444444"
            radius: 4
            border.color: field.activeFocus ? "#00AAFF" : "#666666"
            border.width: 1
        }
        color: "white"
        selectByMouse: true
        font.pixelSize: 12
        verticalAlignment: TextInput.AlignVCenter
        leftPadding: 8
    }
}
