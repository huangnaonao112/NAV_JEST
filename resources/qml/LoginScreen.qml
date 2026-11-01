import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * 登录界面
 * 美观的深色主题登录页面，与应用整体风格一致
 */
Item {
    id: root
    
    // 登录成功信号
    signal loginSuccess(string username)
    
    // 中心登录卡片
    Rectangle {
        id: loginCard
        anchors.centerIn: parent
        width: 420
        height: 540
        radius: 20
        
        color: "#0B2B5B"  // 深蓝色
        border.width: 1
        border.color: Qt.rgba(255/255, 255/255, 255/255, 0.10)
        
        // 关闭按钮（右上角）
        Text {
            id: closeBtn
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 16
            anchors.rightMargin: 16
            text: "×"
            color: Qt.rgba(180/255, 200/255, 220/255, 0.8)
            font.pixelSize: 24
            font.bold: true
            
            MouseArea {
                anchors.fill: parent
                anchors.margins: -8
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                onEntered: closeBtn.color = "#f87171"
                onExited: closeBtn.color = Qt.rgba(180/255, 200/255, 220/255, 0.8)
                onClicked: Qt.quit()
            }
        }
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 36
            spacing: 18
            
            // Logo 和标题
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 16
                
                // Logo 图片（CETC）
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "qrc:/resources/images/logo_cetic.png"
                    Layout.preferredWidth: 280
                    Layout.preferredHeight: 80
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    mipmap: true
                }
                
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "导航场景模拟仿真软件"
                    color: "#ffffff"
                    font.pixelSize: 24
                    font.bold: true
                    font.family: "Microsoft YaHei"
                }
            }
            
            // 分隔线
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                color: Qt.rgba(100/255, 150/255, 200/255, 0.2)
            }
            
            // 登录表单
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                spacing: 16
                
                // 用户名输入
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Text {
                        text: "用户名"
                        color: Qt.rgba(180/255, 200/255, 220/255, 0.9)
                        font.pixelSize: 13
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 44
                        radius: 8
                        color: Qt.rgba(20/255, 35/255, 60/255, 0.8)
                        border.width: usernameInput.activeFocus ? 2 : 1
                        border.color: usernameInput.activeFocus ? "#22d3ee" : Qt.rgba(80/255, 120/255, 160/255, 0.4)
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            
                            // 用户图标
                            Canvas {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                
                                onPaint: {
                                    var ctx = getContext('2d')
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.strokeStyle = Qt.rgba(150/255, 180/255, 210/255, 0.7)
                                    ctx.lineWidth = 1.5
                                    
                                    // 头部圆形
                                    ctx.beginPath()
                                    ctx.arc(width/2, 6, 5, 0, Math.PI * 2)
                                    ctx.stroke()
                                    
                                    // 身体
                                    ctx.beginPath()
                                    ctx.arc(width/2, 18, 8, Math.PI, 0)
                                    ctx.stroke()
                                }
                            }
                            
                            TextField {
                                id: usernameInput
                                Layout.fillWidth: true
                                text: "admin"
                                placeholderText: "请输入用户名"
                                placeholderTextColor: Qt.rgba(120/255, 150/255, 180/255, 0.5)
                                color: "#ffffff"
                                font.pixelSize: 14
                                background: null
                                selectByMouse: true
                                
                                Keys.onReturnPressed: passwordInput.forceActiveFocus()
                                Keys.onEnterPressed: passwordInput.forceActiveFocus()
                            }
                        }
                    }
                }
                
                // 密码输入
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    Text {
                        text: "密码"
                        color: Qt.rgba(180/255, 200/255, 220/255, 0.9)
                        font.pixelSize: 13
                    }
                    
                    Rectangle {
                        Layout.fillWidth: true
                        height: 44
                        radius: 8
                        color: Qt.rgba(20/255, 35/255, 60/255, 0.8)
                        border.width: passwordInput.activeFocus ? 2 : 1
                        border.color: passwordInput.activeFocus ? "#22d3ee" : Qt.rgba(80/255, 120/255, 160/255, 0.4)
                        
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            
                            // 锁图标
                            Canvas {
                                Layout.preferredWidth: 18
                                Layout.preferredHeight: 18
                                
                                onPaint: {
                                    var ctx = getContext('2d')
                                    ctx.clearRect(0, 0, width, height)
                                    ctx.strokeStyle = Qt.rgba(150/255, 180/255, 210/255, 0.7)
                                    ctx.lineWidth = 1.5
                                    
                                    // 锁身 - 手动绘制圆角矩形
                                    ctx.fillStyle = Qt.rgba(20/255, 35/255, 60/255, 1)
                                    ctx.strokeStyle = Qt.rgba(150/255, 180/255, 210/255, 0.7)
                                    ctx.beginPath()
                                    var x = 3, y = 8, w = 12, h = 10, r = 2
                                    ctx.moveTo(x + r, y)
                                    ctx.lineTo(x + w - r, y)
                                    ctx.arcTo(x + w, y, x + w, y + r, r)
                                    ctx.lineTo(x + w, y + h - r)
                                    ctx.arcTo(x + w, y + h, x + w - r, y + h, r)
                                    ctx.lineTo(x + r, y + h)
                                    ctx.arcTo(x, y + h, x, y + h - r, r)
                                    ctx.lineTo(x, y + r)
                                    ctx.arcTo(x, y, x + r, y, r)
                                    ctx.closePath()
                                    ctx.fill()
                                    ctx.stroke()
                                    
                                    // 锁扣
                                    ctx.beginPath()
                                    ctx.moveTo(5, 8)
                                    ctx.lineTo(5, 5)
                                    ctx.arc(9, 5, 4, Math.PI, 0)
                                    ctx.lineTo(13, 8)
                                    ctx.stroke()
                                }
                            }
                            
                            TextField {
                                id: passwordInput
                                Layout.fillWidth: true
                                text: "123456"
                                placeholderText: "请输入密码"
                                placeholderTextColor: Qt.rgba(120/255, 150/255, 180/255, 0.5)
                                color: "#ffffff"
                                font.pixelSize: 14
                                echoMode: TextInput.Password
                                background: null
                                selectByMouse: true
                                
                                Keys.onReturnPressed: attemptLogin()
                                Keys.onEnterPressed: attemptLogin()
                            }
                            
                            // 显示/隐藏密码按钮
                            Rectangle {
                                Layout.preferredWidth: 24
                                Layout.preferredHeight: 24
                                radius: 4
                                color: showPasswordBtn.containsMouse ? Qt.rgba(50/255, 80/255, 120/255, 0.5) : "transparent"
                                
                                Canvas {
                                    anchors.centerIn: parent
                                    width: 16
                                    height: 12
                                    
                                    property bool showPassword: false
                                    
                                    onPaint: {
                                        var ctx = getContext('2d')
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.strokeStyle = Qt.rgba(150/255, 180/255, 210/255, 0.7)
                                        ctx.lineWidth = 1.5
                                        
                                        if (showPassword) {
                                            // 眼睛打开
                                            ctx.beginPath()
                                            ctx.ellipse(8, 6, 7, 4)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.arc(8, 6, 2, 0, Math.PI * 2)
                                            ctx.fill()
                                        } else {
                                            // 眼睛关闭
                                            ctx.beginPath()
                                            ctx.ellipse(8, 6, 7, 4)
                                            ctx.stroke()
                                            ctx.beginPath()
                                            ctx.moveTo(2, 10)
                                            ctx.lineTo(14, 2)
                                            ctx.stroke()
                                        }
                                    }
                                }
                                
                                MouseArea {
                                    id: showPasswordBtn
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    
                                    onPressedChanged: {
                                        passwordInput.echoMode = pressed ? TextInput.Normal : TextInput.Password
                                    }
                                }
                            }
                        }
                    }
                }
                
                // 记住密码 & 忘记密码
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    
                    CheckBox {
                        id: rememberMeCheck
                        text: "记住密码"
                        checked: true
                        
                        indicator: Rectangle {
                            implicitWidth: 18
                            implicitHeight: 18
                            x: rememberMeCheck.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: 4
                            border.width: 1
                            border.color: rememberMeCheck.checked ? "#22d3ee" : Qt.rgba(100/255, 140/255, 180/255, 0.5)
                            color: rememberMeCheck.checked ? Qt.rgba(34/255, 211/255, 238/255, 0.2) : "transparent"
                            
                            Text {
                                anchors.centerIn: parent
                                text: "✓"
                                color: "#22d3ee"
                                font.pixelSize: 12
                                font.bold: true
                                visible: rememberMeCheck.checked
                            }
                        }
                        
                        contentItem: Text {
                            leftPadding: rememberMeCheck.indicator.width + 6
                            text: rememberMeCheck.text
                            color: Qt.rgba(160/255, 185/255, 210/255, 0.8)
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Text {
                        text: "忘记密码?"
                        color: Qt.rgba(100/255, 160/255, 220/255, 0.8)
                        font.pixelSize: 12
                        
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            
                            onEntered: parent.color = "#22d3ee"
                            onExited: parent.color = Qt.rgba(100/255, 160/255, 220/255, 0.8)
                            onClicked: Qt.openUrlExternally("mailto:admin@example.com")
                        }
                    }
                }
            }
            
            // 错误提示
            Text {
                id: errorText
                Layout.alignment: Qt.AlignHCenter
                text: ""
                color: "#f87171"
                font.pixelSize: 12
                visible: text !== ""
                
                Behavior on opacity {
                    NumberAnimation { duration: 200 }
                }
            }
            
            // 登录按钮
            Rectangle {
                id: loginButton
                Layout.fillWidth: true
                Layout.preferredHeight: 48
                Layout.topMargin: 8
                radius: 10
                
                // 按钮渐变背景
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { 
                        position: 0.0
                        color: loginButton.enabled ? (loginButtonMouse.pressed ? "#1d4ed8" : (loginButtonMouse.containsMouse ? "#2563eb" : "#3b82f6")) : "#1e3a5f"
                    }
                    GradientStop { 
                        position: 1.0
                        color: loginButton.enabled ? (loginButtonMouse.pressed ? "#1e40af" : (loginButtonMouse.containsMouse ? "#1d4ed8" : "#2563eb")) : "#1e3a5f"
                    }
                }
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    
                    // 加载指示器
                    BusyIndicator {
                        running: loginButton.enabled === false
                        visible: running
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                    }
                    
                    Text {
                        text: loginButton.enabled ? "登 录" : "登录中..."
                        color: "#ffffff"
                        font.pixelSize: 16
                        font.bold: true
                    }
                }
                
                MouseArea {
                    id: loginButtonMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    enabled: loginButton.enabled
                    
                    onClicked: attemptLogin()
                }
                
                // 启用键盘回车触发
                Keys.onReturnPressed: attemptLogin()
                Keys.onEnterPressed: attemptLogin()
            }
            
            // 版本信息
            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                text: "v1.0.0"
                color: Qt.rgba(120/255, 150/255, 180/255, 0.5)
                font.pixelSize: 11
            }
        }
    }
    
    // 底部版权信息
    Text {
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        text: "© 2024 Navigation Simulation System. All rights reserved."
        color: Qt.rgba(100/255, 130/255, 160/255, 0.4)
        font.pixelSize: 11
    }
    
    // 登录逻辑
    property bool isLoggingIn: false
    
    function attemptLogin() {
        if (isLoggingIn) return
        
        var username = usernameInput.text.trim()
        var password = passwordInput.text
        
        // 验证输入
        if (username === "") {
            showError("请输入用户名")
            usernameInput.forceActiveFocus()
            return
        }
        
        if (password === "") {
            showError("请输入密码")
            passwordInput.forceActiveFocus()
            return
        }
        
        // 开始登录
        isLoggingIn = true
        loginButton.enabled = false
        errorText.text = ""
        
        // 调用登录 API
        var xhr = new XMLHttpRequest()
        var json = JSON.stringify({
            username: username,
            password: password
        })
        
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                isLoggingIn = false
                loginButton.enabled = true
                
                if (xhr.status === 200) {
                    try {
                        var response = JSON.parse(xhr.responseText)
                        if (response.ok === true) {
                            // 登录成功
                            console.log("Login success:", username)
                            
                            // 保存记住密码
                            if (rememberMeCheck.checked) {
                                // TODO: 使用 QSettings 保存
                            }
                            
                            root.loginSuccess(username)
                        } else {
                            showError(response.message || "登录失败")
                        }
                    } catch (e) {
                        showError("服务器响应格式错误")
                    }
                } else if (xhr.status === 401) {
                    showError("用户名或密码错误")
                } else {
                    showError("无法连接到服务器 (" + xhr.status + ")")
                }
            }
        }
        
        xhr.onerror = function() {
            isLoggingIn = false
            loginButton.enabled = true
            showError("网络连接失败，请检查服务器是否启动")
        }
        
        xhr.open("POST", "http://localhost:18080/api/v1/login", true)
        xhr.setRequestHeader("Content-Type", "application/json")
        xhr.timeout = 10000
        xhr.send(json)
    }
    
    function showError(message) {
        errorText.text = message
        errorText.opacity = 1
        
        // 3秒后自动清除错误
        errorTimer.restart()
    }
    
    Timer {
        id: errorTimer
        interval: 3000
        repeat: false
        onTriggered: {
            errorText.text = ""
        }
    }
    
    // 初始化焦点
    Component.onCompleted: {
        usernameInput.forceActiveFocus()
    }
}