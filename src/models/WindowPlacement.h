#pragma once

#include <QObject>
#include <QRect>
#include <QScreen>
#include <QGuiApplication>
#include <QWindow>

/**
 * @brief 窗口放置辅助（QML 单例）
 *
 * 多显示器环境下确定"窗口模式"的几何。QML 的 Screen 附加属性在某些
 * 虚拟显示驱动下会解析成跨屏联合区域（union of screens），导致窗口
 * 横跨两台显示器；这里改用 screenAt() 对窗口中心点逐物理屏判定，
 * 返回值保证完全落在单块屏幕内。
 */
class WindowPlacement : public QObject {
    Q_OBJECT

public:
    explicit WindowPlacement(QObject *parent = nullptr) : QObject(parent) {}

    // 窗口模式几何：包含 window 中心点的物理屏幕的 80% x 85% 居中区域
    Q_INVOKABLE QRect windowedGeometry(QWindow *w) const {
        QRect cur = w->geometry();
        QScreen *scr = QGuiApplication::screenAt(cur.center());
        if (!scr) scr = w->screen();
        if (!scr) scr = QGuiApplication::primaryScreen();
        const QRect av = scr->availableGeometry();
        const int wd = av.width() * 0.8;
        const int ht = av.height() * 0.85;
        return QRect(av.x() + (av.width() - wd) / 2,
                     av.y() + (av.height() - ht) / 2, wd, ht);
    }
};
