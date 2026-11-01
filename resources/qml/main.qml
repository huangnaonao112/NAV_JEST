import QtQuick
import QtQuick.Window
import NavModels
import "."

Window {
    id: rootWindow
    width: 520
    height: 720
    minimumWidth: 520
    minimumHeight: 720
    visible: true
    title: ""
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    property bool isLoggedIn: false

    // 窗口在屏幕居中（登录界面）
    Component.onCompleted: {
        x = Screen.width / 2 - width / 2
        y = Screen.height / 2 - height / 2
    }

    // 登录成功后进入窗口模式（不是全屏）。
    // 多显示器注意：本机虚拟显示驱动会让 QML 的 Screen 附加属性解析成
    // 跨屏联合区域（窗口因此横跨两块屏），所以几何必须经 C++ 的
    // screenAt() 按窗口中心点落到单块物理屏内计算。
    // 全屏只由用户双击顶栏进入；showFullScreen 会记住此刻的窗口几何，
    // 再次双击 showNormal 即精确还原“刚才的窗口模式”
    onIsLoggedInChanged: {
        if (isLoggedIn) {
            const g = WindowPlacement.windowedGeometry(rootWindow)
            width = g.width
            height = g.height
            x = g.x
            y = g.y
        }
    }

    // 登录界面 —— z 序钉在顶层：预热渲染的主界面在其后声明（默认盖住它），
    // 必须显式抬高才能保证启动时用户看到并操作登录页
    Loader {
        id: loginLoader
        anchors.fill: parent
        z: 10
        visible: !isLoggedIn
        source: "qrc:/resources/qml/LoginScreen.qml"
        onLoaded: {
            if (item && item.hasOwnProperty("loginSuccess")) {
                item.loginSuccess.connect(function(username) {
                    isLoggedIn = true
                })
            }
        }
    }

    // 主界面 - 预加载但不显示：active: true 使对象实例化/模型装配/数据库读取
    // 在登录期间完成，但 visible: false 期间不参与渲染 —— 登录界面前台保持
    // 干净（透明窗口露出桌面），只有登录验证通过后才显示主界面
    Loader {
        id: appShellLoader
        anchors.fill: parent
        visible: isLoggedIn
        active: true
        asynchronous: true
        source: "qrc:/resources/qml/AppShell.qml"
        onLoaded: {
            if (item && item.hasOwnProperty("window")) {
                item.window = rootWindow
            }
        }
    }
}
