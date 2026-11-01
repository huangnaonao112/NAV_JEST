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
    readonly property var userModel: UserModel

    Component.onCompleted: {
        console.log("[UserManagementPage] completed, width=" + width + " height=" + height)
        if (root.userModel) root.userModel.load()
    }
    onWidthChanged: console.log("[UserManagementPage] width=" + width)
    onHeightChanged: console.log("[UserManagementPage] height=" + height)

    // 白色卡片容器
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

            // 顶部操作按钮（只保留“添加”）
            Row {
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
                        editorTitle.text = "添加用户"
                        editor.editIndex = -1
                        usernameField.text = ""
                        passwordField.text = ""
                        phoneField.text = ""
                        deptField.text = ""
                        statusCombo.currentIndex = 0
                        editor.open()
                    }
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
                        model: root.userModel
                        
                        // 启用 delegate 复用，减少创建/销毁开销
                        reuseItems: true
                        // 增加缓存，减少滚动时的闪烁
                        cacheBuffer: 200

                        // 按比例分配列宽，填满容器
                        readonly property int colSpacing: 12
                        readonly property int totalSpacing: 7 * colSpacing
                        readonly property int availableWidth: width - 24 - totalSpacing
                        
                        // 操作列固定20%，其他列分80%
                        readonly property int colOp: Math.max(100, Math.floor(availableWidth * 0.20))
                        readonly property int otherWidth: availableWidth - colOp
                        
                        // 其他列按比例分配80%
                        readonly property int colIndex: Math.max(45, Math.floor(otherWidth * 0.08))
                        readonly property int colUsername: Math.max(90, Math.floor(otherWidth * 0.16))
                        readonly property int colPassword: Math.max(55, Math.floor(otherWidth * 0.08))
                        readonly property int colPhone: Math.max(110, Math.floor(otherWidth * 0.21))
                        readonly property int colDept: Math.max(85, Math.floor(otherWidth * 0.15))
                        readonly property int colStatus: Math.max(60, Math.floor(otherWidth * 0.10))
                        readonly property int colUpdated: Math.max(100, Math.floor(otherWidth * 0.22))

                        header: Rectangle {
                            width: list.width
                            height: 44
                            color: "#F6F7F9"

                            Row {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: list.colSpacing

                                Text { text: "序号"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colIndex; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "用户名"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colUsername; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "密码"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colPassword; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "联系方式"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colPhone; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "部门"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colDept; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "状态"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colStatus; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: "更新时间"; color: root.text2; font.pixelSize: 13; font.bold: true; width: list.colUpdated; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Item {
                                    width: list.colOp
                                    height: parent.height
                                    Text {
                                        anchors.centerIn: parent
                                        text: "操作"
                                        color: root.text2
                                        font.pixelSize: 13
                                        font.bold: true
                                    }
                                }
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
                                enabled: username !== "admin"
                                cursorShape: username === "admin" ? Qt.ArrowCursor : Qt.PointingHandCursor
                                onClicked: {
                                    list.currentIndex = index
                                    root.selectedIndex = index
                                }
                                onEntered: {
                                    if (!ListView.isCurrentItem && username !== "admin") {
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

                                Text { text: rowNumber; color: root.text3; width: list.colIndex; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter }
                                Text { text: username; color: root.text1; width: list.colUsername; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: "***"; color: root.text2; width: list.colPassword; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: phone; color: root.text2; width: list.colPhone; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: dept; color: root.text2; width: list.colDept; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: status; color: root.text2; width: list.colStatus; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                Text { text: updatedAt; color: root.text3; width: list.colUpdated; height: parent.height; verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
                                // admin 行操作栏空白，其他行显示 编辑、删除
                                Item {
                                    width: list.colOp
                                    height: parent.height
                                    Row {
                                        visible: username !== "admin"
                                        anchors.centerIn: parent
                                        height: parent.height
                                        spacing: 12
                                        Text {
                                            text: "编辑"
                                            color: root.primary
                                            font.pixelSize: 13
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    root.selectedIndex = index
                                                    const u = root.userModel.get(index)
                                                    editorTitle.text = "编辑用户"
                                                    editor.editIndex = index
                                                    usernameField.text = u.username || ""
                                                    passwordField.text = ""
                                                    phoneField.text = u.phone || ""
                                                    deptField.text = u.dept || ""
                                                    statusCombo.currentIndex = (u.status === "正常" ? 0 : 1)
                                                    editor.open()
                                                }
                                            }
                                        }
                                        Text {
                                            text: "删除"
                                            color: root.danger
                                            font.pixelSize: 13
                                            height: parent.height
                                            verticalAlignment: Text.AlignVCenter
                                            MouseArea {
                                                anchors.fill: parent
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
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
                }

                Item {
                    anchors.fill: parent
                    visible: !root.userModel || list.count === 0
                    Text {
                        anchors.centerIn: parent
                        text: "暂无数据"
                        color: root.text3
                    }
                }
            }
        }
    }

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
                    text: "确定要删除当前选中的用户吗？此操作不可撤销。"
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
                            if (root.userModel) {
                                root.userModel.removeAt(root.selectedIndex)
                                root.userModel.load()
                            }
                            root.selectedIndex = -1
                            confirmDelete.close()
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: editor
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        parent: Overlay.overlay
        width: 380
        height: contentItem.implicitHeight

        onOpened: {
            // 居中显示（每次打开都居中，避免跑偏）
            if (parent) {
                x = Math.round((parent.width - width) / 2)
                y = Math.round((parent.height - height) / 2)
            }
        }

        property int editIndex: -1

        function doSubmit() {
            if (editIndex < 0) {
                if (!usernameField.text.trim()) return
                if (root.userModel) {
                    root.userModel.add(usernameField.text.trim(), passwordField.text, phoneField.text.trim(), deptField.text.trim(), statusCombo.currentText)
                    root.userModel.load()
                }
            } else {
                if (root.userModel) {
                    root.userModel.update(editIndex, usernameField.text.trim(), passwordField.text, phoneField.text.trim(), deptField.text.trim(), statusCombo.currentText)
                    root.userModel.load()
                }
            }
            editor.close()
        }

        background: Rectangle {
            color: "#E8F0FE"
            radius: 14
            border.width: 1
            border.color: "#BBDEFB"
        }

        contentItem: Column {
            width: parent ? parent.width : 380
            spacing: 0

            // 标题栏（企业蓝）+ 可拖动 + 右上角关闭
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
                        text: "添加用户"
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

            // 表单区（与表格列一致：用户名、密码、联系方式、部门、状态）
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

                    Text { text: "用户名"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: usernameField
                        width: 220
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "请输入用户名"
                        background: Rectangle {
                            color: "white"
                            border.color: "#BBDEFB"
                            border.width: 1
                            radius: 6
                        }
                    }

                    Text { text: "密码"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: passwordField
                        width: 220
                        color: root.text1
                        font.pixelSize: 13
                        echoMode: TextInput.Password
                        placeholderText: editor.editIndex < 0 ? "请输入密码" : "留空则不修改"
                        background: Rectangle {
                            color: "white"
                            border.color: "#BBDEFB"
                            border.width: 1
                            radius: 6
                        }
                    }

                    Text { text: "联系方式"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: phoneField
                        width: 220
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 189XXXXXXXX"
                        background: Rectangle {
                            color: "white"
                            border.color: "#BBDEFB"
                            border.width: 1
                            radius: 6
                        }
                    }

                    Text { text: "部门"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    TextField {
                        id: deptField
                        width: 220
                        color: root.text1
                        font.pixelSize: 13
                        placeholderText: "例如 研发部"
                        background: Rectangle {
                            color: "white"
                            border.color: "#BBDEFB"
                            border.width: 1
                            radius: 6
                        }
                    }

                    Text { text: "状态"; color: root.text1; font.pixelSize: 13; height: 32; verticalAlignment: Text.AlignVCenter }
                    ComboBox {
                        id: statusCombo
                        width: 220
                        height: 32
                        implicitHeight: 32
                        font.pixelSize: 13
                        model: ["正常", "锁定"]
                        delegate: ItemDelegate {
                            width: statusCombo.width
                            contentItem: Text {
                                text: modelData
                                color: (statusCombo.highlightedIndex === index) ? "white" : root.text1
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                            background: Rectangle {
                                radius: 6
                                color: (statusCombo.highlightedIndex === index)
                                    ? root.primary
                                    : (hovered ? "#EEF2FF" : "transparent")
                            }
                        }
                        contentItem: Text {
                            leftPadding: 10
                            rightPadding: 28
                            text: statusCombo.displayText
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
                            y: statusCombo.height + 4
                            width: statusCombo.width
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
                                model: statusCombo.delegateModel
                            }
                        }
                    }
                }

                // 底部按钮：添加 / 保存、取消
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
