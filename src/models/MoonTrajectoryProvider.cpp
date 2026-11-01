#include "MoonTrajectoryProvider.h"
#include "OrbitalPhysics.h"
#include <QPen>
#include <QtMath>
#include <QCoreApplication>
#include <QDebug>

MoonTrajectoryProvider::MoonTrajectoryProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // 初始化透明纹理 2048x1024 (2:1 等距圆柱投影比例)
    // 使用非预乘格式确保alpha通道正确保存
    m_image = QImage(TEX_WIDTH, TEX_HEIGHT, QImage::Format_ARGB32);
    m_image.fill(Qt::transparent);
    
    // 初始化Painter（保持打开状态，避免重复创建）
    m_painter.begin(&m_image);
    m_painter.setRenderHint(QPainter::Antialiasing, true);
    m_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_painter.setCompositionMode(QPainter::CompositionMode_Source);
    
    // 设置默认画笔：绿色、2像素宽、实线（优化性能）
    QPen pen(QColor(0, 255, 0, 255), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    m_painter.setPen(pen);
}

QImage MoonTrajectoryProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QMutexLocker locker(&m_mutex);
    Q_UNUSED(id);
    if (size) {
        *size = m_image.size();
    }
    if (requestedSize.isValid()) {
        return m_image.scaled(requestedSize, Qt::KeepAspectRatio);
    }
    return m_image;
}

void MoonTrajectoryProvider::setTestPoints(const QVariantList &testPoints)
{
    QMutexLocker locker(&m_mutex);
    m_testPoints = testPoints;
    m_hasLastPoint = false;  // 重置最后点标记
    renderTexture();
}

void MoonTrajectoryProvider::appendTrajectoryPoint(double lat, double lon)
{
    QMutexLocker locker(&m_mutex);
    
    // 确保Painter已打开
    if (!m_painter.isActive()) {
        m_painter.begin(&m_image);
        m_painter.setRenderHint(QPainter::Antialiasing, true);
        m_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        m_painter.setPen(QPen(QColor(0, 255, 0, 255), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    }
    
    // 如果有上一个点，绘制连接线段
    if (m_hasLastPoint) {
        drawTrajectorySegment(m_lastLat, m_lastLon, lat, lon);
    }
    
    // 更新最后点坐标
    m_lastLat = lat;
    m_lastLon = lon;
    m_hasLastPoint = true;
    
    // 每次都保存纹理（确保可视化）
    QString filePath = QCoreApplication::applicationDirPath() + "/moon_trajectory_texture.png";
    m_image.save(filePath, "PNG");

    emit textureUpdated();
}

void MoonTrajectoryProvider::clearTrajectory()
{
    QMutexLocker locker(&m_mutex);
    
    // 结束Painter
    if (m_painter.isActive()) {
        m_painter.end();
    }
    
    // 重新加载月球纹理
    QString moonTexPath = QCoreApplication::applicationDirPath() + "/../resources/images/moon_color.jpg";
    QImage moonTexture;
    if (moonTexture.load(moonTexPath)) {
        m_image = moonTexture.scaled(TEX_WIDTH, TEX_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        m_image = QImage(TEX_WIDTH, TEX_HEIGHT, QImage::Format_ARGB32);
        m_image.fill(Qt::transparent);
    }
    
    m_hasLastPoint = false;
    m_testPoints.clear();
    
    // 保存清空后的纹理
    QString filePath = QCoreApplication::applicationDirPath() + "/moon_trajectory_texture.png";
    m_image.save(filePath, "PNG");

    emit textureUpdated();
}

QPointF MoonTrajectoryProvider::lonLatToUV(double lon, double lat, int width, int height) const
{
    // 规范化经度到 [-180, 180]
    while (lon > 180.0) lon -= 360.0;
    while (lon < -180.0) lon += 360.0;
    
    // 等距圆柱投影: 经度[-180,180] -> x[0,width], 纬度[-90,90] -> y[0,height]
    double u = (lon + 180.0) / 360.0;
    double v = (90.0 - lat) / 180.0;
    return QPointF(u * width, v * height);
}

void MoonTrajectoryProvider::renderTexture()
{
    // 结束之前的Painter
    if (m_painter.isActive()) {
        m_painter.end();
    }
    
    // 加载月球原始纹理
    QString moonTexPath = QCoreApplication::applicationDirPath() + "/../resources/images/moon_color.jpg";
    QImage moonTexture;
    if (!moonTexture.load(moonTexPath)) {
        m_image = QImage(TEX_WIDTH, TEX_HEIGHT, QImage::Format_ARGB32);
        m_image.fill(Qt::transparent);
    } else {
        m_image = moonTexture.scaled(TEX_WIDTH, TEX_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    
    // 开始绘制轨迹线
    m_painter.begin(&m_image);
    m_painter.setRenderHint(QPainter::Antialiasing, true);
    m_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    // 绿色画笔：2像素实线，性能优化
    QPen pen(QColor(0, 255, 0, 255), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    m_painter.setPen(pen);
    
    if (m_testPoints.size() >= 2) {
        for (int i = 0; i < m_testPoints.size() - 1; ++i) {
            auto p1Map = m_testPoints[i].value<QVariantMap>();
            auto p2Map = m_testPoints[i + 1].value<QVariantMap>();
            
            double lat1 = p1Map["lat"].toDouble();
            double lon1 = p1Map["lon"].toDouble();
            double lat2 = p2Map["lat"].toDouble();
            double lon2 = p2Map["lon"].toDouble();
            
            drawTrajectorySegment(lat1, lon1, lat2, lon2);
        }
        
        // 更新最后点坐标
        auto lastMap = m_testPoints.last().value<QVariantMap>();
        m_lastLat = lastMap["lat"].toDouble();
        m_lastLon = lastMap["lon"].toDouble();
        m_hasLastPoint = true;
    }
    
    // 结束绘制
    m_painter.end();
    
    // 保存纹理到文件
    QString filePath = QCoreApplication::applicationDirPath() + "/moon_trajectory_texture.png";
    m_image.save(filePath, "PNG");

    // 重新开始Painter以备后续动态更新
    m_painter.begin(&m_image);
    m_painter.setRenderHint(QPainter::Antialiasing, true);
    m_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
m_painter.setPen(QPen(QColor(0, 255, 0, 255), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
}

void MoonTrajectoryProvider::drawTrajectorySegment(double lat1, double lon1, double lat2, double lon2)
{
    // 处理经度跨越180°的情况（UV接缝）
    double lonDiff = lon2 - lon1;
    if (lonDiff > 180.0) lon1 += 360.0;
    else if (lonDiff < -180.0) lon2 += 360.0;
    
    // 使用球面插值生成中间点（50段确保曲线平滑）
    const int SEGMENTS = 50;
    for (int j = 0; j < SEGMENTS; ++j) {
        double t1 = static_cast<double>(j) / SEGMENTS;
        double t2 = static_cast<double>(j + 1) / SEGMENTS;
        
        double latA = lat1 + (lat2 - lat1) * t1;
        double lonA = lon1 + (lon2 - lon1) * t1;
        double latB = lat1 + (lat2 - lat1) * t2;
        double lonB = lon1 + (lon2 - lon1) * t2;
        
        QPointF uvA = lonLatToUV(lonA, latA, TEX_WIDTH, TEX_HEIGHT);
        QPointF uvB = lonLatToUV(lonB, latB, TEX_WIDTH, TEX_HEIGHT);
        
        m_painter.drawLine(uvA, uvB);
    }
}
