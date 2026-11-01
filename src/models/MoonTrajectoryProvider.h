#pragma once

#include <QObject>
#include <QQuickImageProvider>
#include <QImage>
#include <QPainter>
#include <QVector3D>
#include <QVariantList>
#include <QVariantMap>
#include <QMutex>
#include <QString>
#include <QSize>
#include <QPointF>

/**
 * @brief 月球轨迹纹理提供者
 * 
 * 将轨迹线绘制在等距圆柱投影的2D纹理上，然后贴到3D球体表面。
 * 优点：只需1个Draw Call，完美贴合球面，抗锯齿平滑。
 * 
 * 动态更新机制：
 * - appendTrajectoryPoint(): 逐点添加，局部重绘新线段
 * - 使用QPainter绘制新增线段，避免重绘整个纹理
 */
class MoonTrajectoryProvider : public QQuickImageProvider {
    Q_OBJECT
    
public:
    MoonTrajectoryProvider();
    
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    
    // 设置测试点数据并重新生成纹理（用于初始化）
    void setTestPoints(const QVariantList &testPoints);
    
    // 动态添加轨迹点（局部重绘）
    Q_INVOKABLE void appendTrajectoryPoint(double lat, double lon);
    
    // 清空轨迹
    Q_INVOKABLE void clearTrajectory();
    
    // 获取当前纹理（用于信号通知）
    const QImage& currentImage() const { return m_image; }

signals:
    // 纹理更新信号（通知QML重新加载）
    void textureUpdated();

private:
    // 经纬度转UV坐标
    QPointF lonLatToUV(double lon, double lat, int width, int height) const;
    
    // 绘制纹理（初始化时完整绘制）
    void renderTexture();
    
    // 局部绘制单条线段（动态更新时）
    void drawTrajectorySegment(double lat1, double lon1, double lat2, double lon2);
    
    QImage m_image;
    QPainter m_painter;  // 保持Painter打开，避免重复创建
    QVariantList m_testPoints;
    
    // 最后一个轨迹点的坐标（用于连接新点）
    double m_lastLat = 0.0;
    double m_lastLon = 0.0;
    bool m_hasLastPoint = false;
    
    // 线程安全锁
    QMutex m_mutex;
    
    // 纹理尺寸常量
    static constexpr int TEX_WIDTH = 2048;
    static constexpr int TEX_HEIGHT = 1024;
};
