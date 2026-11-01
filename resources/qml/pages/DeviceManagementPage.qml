import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NavModels

Item {
    id: root

    property int selectedIndex: -1
    readonly property color primary: "#1677FF"
    readonly property color danger: "#EF4444"
    readonly property color text1: "#1F2937"
    readonly property color text2: "#6B7280"
    readonly property color text3: "#9CA3AF"

    // 阴影层
    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        anchors.topMargin: 14
        y: 2
        radius: 10
        color: "#000000"
        opacity: 0.06
        z: -1
    }

    // 主卡片
    Rectangle {
        anchors.fill: parent
        anchors.margins: 12
        color: "white"
        radius: 10
        border.width: 1
        border.color: "#E5E7EB"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 16

            Component.onCompleted: {
                if (DeviceModel && DeviceModel.load) DeviceModel.load()
            }

            // 顶部工具栏（添加）
            Row {
                id: toolbarRow
                spacing: 12

                Button {
                    text: "添加"
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
                    onClicked: {
                        editorTitle.text = "添加设备"
                        editor.editIndex = -1
                        deviceIdField.text = ""
                        nameField.text = ""
                        typeField.text = ""
                        modelField.text = ""
                        vendorField.text = ""
                        connectionField.text = "TCP/IP"
                        addressField.text = ""
                        editor.open()
                    }
                }
            }

            // 分割线
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#E5E7EB"
            }

            // 表格区域
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 10
                color: "white"
                border.width: 1
                border.color: "#E5E7EB"
                clip: true

                Flickable {
                    id: tableHScroll
                    anchors.fill: parent
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    flickableDirection: Flickable.HorizontalFlick

                    contentWidth: list.width
                    contentHeight: height

                    ListView {
                        id: list
                        width: tableHScroll.width
                        height: tableHScroll.height
                        clip: true
                        model: DeviceModel
                        
                        // 启用 delegate 复用，减少创建/销毁开销
                        reuseItems: true
                        // 增加缓存，减少滚动时的闪烁
                        cacheBuffer: 200

                        // 按比例分配列宽，填满容器
                        readonly property int colSpacing: 12
                        readonly property int totalSpacing: 9 * colSpacing
                        readonly property int availableWidth: width - 24 - totalSpacing
                        
                        // 操作列固定20%，其他列分80%
                        readonly property int colOp: Math.max(100, Math.floor(availableWidth * 0.20))
                        readonly property int otherWidth: availableWidth - colOp
                        
                        // 其他列按比例分配80%
                        readonly property int colIndex: Math.max(45, Math.floor(otherWidth * 0.07))
                        readonly property int colDeviceId: Math.max(75, Math.floor(otherWidth * 0.12))
                        readonly property int colName: Math.max(85, Math.floor(otherWidth * 0.14))
                        readonly property int colType: Math.max(75, Math.floor(otherWidth * 0.11))
                        readonly property int colModel: Math.max(75, Math.floor(otherWidth * 0.11))
                        readonly property int colVendor: Math.max(65, Math.floor(otherWidth * 0.09))
                        readonly property int colConn: Math.max(60, Math.floor(otherWidth * 0.08))
                        readonly property int colAddr: Math.max(85, Math.floor(otherWidth * 0.12))
                        readonly property int colUpdated: Math.max(100, Math.floor(otherWidth * 0.16))

                        header: Rectangle {
                            width: list.width
                            height: 44
                            color: "#F6F7F9"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: list.colSpacing

                                Text { text: "序号"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colIndex; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "设备编号"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colDeviceId; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "设备名称"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "设备类型"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colType; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "设备型号"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colModel; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "设备厂家"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colVendor; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "连接方式"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colConn; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "连接信息"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colAddr; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "更新时间"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colUpdated; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "操作"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colOp; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                            }
                        }

                        delegate: Rectangle {
                            id: delegateItem
                            width: list.width
                            height: 44
                            // 使用属性缓存颜色，避免频繁计算
                            property color bgColor: "white"
                            color: bgColor
                            border.width: 1
                            border.color: "#E5E7EB"
                            
                            // 延迟更新颜色，减少闪烁
                            onActiveFocusChanged: Qt.callLater(updateColor)
                            
                            function updateColor() {
                                if (ListView.isCurrentItem) {
                                    bgColor = "#EAF2FF"
                                } else {
                                    bgColor = "white"
                                }
                            }
                            
                            Connections {
                                target: list
                                function onCurrentIndexChanged() {
                                    delegateItem.updateColor()
                                }
                            }
                            
                            Component.onCompleted: updateColor()

                            MouseArea {
                                id: rowHover
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    list.currentIndex = index
                                    root.selectedIndex = index
                                }
                                onEntered: {
                                    if (!ListView.isCurrentItem) {
                                        delegateItem.bgColor = "#F5F7FF"
                                    }
                                }
                                onExited: {
                                    delegateItem.updateColor()
                                }
                            }

                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: list.colSpacing

                                Text { text: index + 1; color: root.text3; width: list.colIndex; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: deviceId; color: root.text1; width: list.colDeviceId; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: name; color: root.text1; width: list.colName; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: type; color: root.text2; width: list.colType; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: model; color: root.text2; width: list.colModel; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: vendor; color: root.text2; width: list.colVendor; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: connection; color: root.text2; width: list.colConn; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: address; color: root.text2; width: list.colAddr; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: updatedAt; color: root.text3; width: list.colUpdated; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }

                                Item {
                                    width: list.colOp
                                    height: parent.height
                                    Row {
                                        anchors.centerIn: parent
                                        height: parent.height
                                        spacing: 10

                                        Text {
                                            text: "编辑"
                                            color: root.primary
                                            font.pixelSize: 13
                                            width: 44
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment: Text.AlignHCenter
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    list.currentIndex = index
                                                    root.selectedIndex = index
                                                    const m = DeviceModel.get(index)
                                                    editorTitle.text = "编辑设备"
                                                    editor.editIndex = index
                                                    deviceIdField.text = m.deviceId || ""
                                                    nameField.text = m.name || ""
                                                    typeField.text = m.type || ""
                                                    modelField.text = m.model || ""
                                                    vendorField.text = m.vendor || ""
                                                    connectionField.text = m.connection || ""
                                                    addressField.text = m.address || ""
                                                    editor.open()
                                                }
                                            }
                                        }

                                        Text {
                                            text: "删除"
                                            color: root.danger
                                            font.pixelSize: 13
                                            width: 44
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                            horizontalAlignment: Text.AlignHCenter
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    list.currentIndex = index
                                                    root.selectedIndex = index
                                                    confirmDelete.open()
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 无数据占位
                    Item {
                        anchors.fill: parent
                        visible: list.count === 0
                        Text {
                            anchors.centerIn: parent
                            text: "暂无数据"
                            color: root.text3
                        }
                    }
                }
            }
        }
    }

    // 删除确认弹窗
    Popup {
        id: confirmDelete
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        parent: Overlay.overlay
        width: 360
        height: 180

        onOpened: {
            x = Math.round((parent.width - width) / 2)
            y = Math.round((parent.height - height) / 2)
        }

        background: Rectangle {
            color: "#E8F0FE"
            radius: 14
            border.width: 1
            border.color: "#BBDEFB"
        }

        contentItem: Column {
            width: parent.width
            spacing: 0

            Rectangle {
                width: parent.width
                height: 48
                color: root.danger
                radius: 14
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 14
                    color: root.danger
                }
                Text {
                    anchors.centerIn: parent
                    text: "确认删除"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                }
            }

            Column {
                width: parent.width
                leftPadding: 20
                rightPadding: 20
                topPadding: 20
                bottomPadding: 20
                spacing: 16

                Text {
                    width: parent.width - 40
                    text: "确定要删除当前选中的设备吗？此操作不可撤销。"
                    color: root.text1
                    wrapMode: Text.WordWrap
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    width: parent.width - 40
                    spacing: 12

                    Button {
                        text: "取消"
                        flat: true
                        implicitWidth: 72
                        implicitHeight: 32
                        background: Rectangle {
                            color: parent.pressed ? "#E5E7EB" : (parent.hovered ? "#F6F7F9" : "white")
                            radius: 6
                            border.width: 1
                            border.color: "#E5E7EB"
                        }
                        contentItem: Text {
                            text: parent.text
                            color: root.text2
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: 13
                        }
                        onClicked: confirmDelete.close()
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "删除"
                        implicitWidth: 72
                        implicitHeight: 32
                        background: Rectangle {
                            color: parent.pressed ? "#B91C1C" : (parent.hovered ? "#DC2626" : root.danger)
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
                            DeviceModel.removeAt(root.selectedIndex)
                            if (DeviceModel && DeviceModel.load) DeviceModel.load()
                            root.selectedIndex = -1
                            confirmDelete.close()
                        }
                    }
                }
            }
        }
    }

    // 编辑/新增设备弹窗
    Popup {
        id: editor
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        parent: Overlay.overlay
        width: 420
        height: contentItem.implicitHeight

        onOpened: {
            if (parent) {
                x = Math.round((parent.width - width) / 2)
                y = Math.round((parent.height - height) / 2)
            }
        }

        property int editIndex: -1

        function doSubmit() {
            if (editIndex < 0) {
                if (!deviceIdField.text.trim())
                    return
                DeviceModel.add(
                            deviceIdField.text.trim(),
                            nameField.text.trim(),
                            typeField.text.trim(),
                            modelField.text.trim(),
                            vendorField.text.trim(),
                            connectionField.text.trim(),
                            addressField.text.trim())
            } else {
                DeviceModel.update(
                            editIndex,
                            deviceIdField.text.trim(),
                            nameField.text.trim(),
                            typeField.text.trim(),
                            modelField.text.trim(),
                            vendorField.text.trim(),
                            connectionField.text.trim(),
                            addressField.text.trim())
            }
            if (DeviceModel && DeviceModel.load) DeviceModel.load()
            root.selectedIndex = -1
            editor.close()
        }

        background: Rectangle {
            color: "#E8F0FE"
            radius: 14
            border.width: 1
            border.color: "#BBDEFB"
        }

        contentItem: Column {
            width: parent ? parent.width : 420
            spacing: 0

            // 标题栏（可拖动）
            Rectangle {
                width: parent.width
                height: 48
                color: root.primary
                property point _dragStart: Qt.point(0, 0)
                property point _dialogStart: Qt.point(0, 0)
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.SizeAllCursor
                    onPressed: {
                        var p = mapToItem(editor.parent, mouse.x, mouse.y)
                        parent._dragStart = Qt.point(p.x, p.y)
                        parent._dialogStart = Qt.point(editor.x, editor.y)
                    }
                    onPositionChanged: {
                        if (pressed) {
                            var cur = mapToItem(editor.parent, mouse.x, mouse.y)
                            editor.x = parent._dialogStart.x + (cur.x - parent._dragStart.x)
                            editor.y = parent._dialogStart.y + (cur.y - parent._dragStart.y)
                        }
                    }
                }
                Item {
                    anchors.fill: parent
                    Text {
                        id: editorTitle
                        anchors.left: parent.left
                        anchors.leftMargin: 20
                        anchors.verticalCenter: parent.verticalCenter
                        text: "添加设备"
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
                            onClicked: editor.close()
                        }
                    }
                }
            }

            // 表单区
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

                    Text { text: "设备编号"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: deviceIdField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 CETC001"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                    Text { text: "设备名称"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: nameField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 干扰模拟器"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                    Text { text: "设备类型"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: typeField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 模拟器"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                    Text { text: "设备型号"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: modelField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 CETC002"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                    Text { text: "设备厂家"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: vendorField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 CETC"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                    Text { text: "连接方式"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: connectionField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 TCP/IP"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                    Text { text: "连接信息"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: addressField
                        width: 240
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 127.0.0.1:5000"
                        background: Rectangle { color: "#F6F7F9"; border.color: "#E5E7EB"; border.width: 1; radius: 6 }
                    }
                }

                Text {
                    text: "数据文件：" + (DeviceModel ? DeviceModel.storagePath() : "")
                    color: root.text2
                    font.pixelSize: 12
                    wrapMode: Text.WrapAnywhere
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
                        onClicked: editor.close()
                    }

                    Item { Layout.fillWidth: true; height: 1 }

                    Button {
                        text: editor.editIndex < 0 ? "添加" : "保存"
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
                        onClicked: editor.doSubmit()
                    }
                }
            }
        }
    }
}
