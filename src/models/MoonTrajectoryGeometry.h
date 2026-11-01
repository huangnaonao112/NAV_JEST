#pragma once

#include <QtQuick3D/qquick3dgeometry.h>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QColor>
#include <QVector>

/**
 * @brief 月球轨迹几何体 - GPU高性能渲染
 * 
 * 使用大圆弧插值确保轨迹线贴合球面
 * 支持动态更新、颜色渐变
 */
class MoonTrajectoryGeometry : public QQuick3DGeometry {
    QML_ELEMENT
    Q_OBJECT
    
public:
    MoonTrajectoryGeometry();
    
    // 添加轨迹点（经纬度）
    Q_INVOKABLE void addTrajectoryPoint(double lat, double lon);
    
    // 清空轨迹
    Q_INVOKABLE void clear();
    
    // 设置轨迹颜色（渐变起点）
    Q_PROPERTY(QColor startColor READ startColor WRITE setStartColor NOTIFY startColorChanged)
    QColor startColor() const { return m_startColor; }
    void setStartColor(const QColor &color);
    
    // 设置轨迹颜色（渐变终点）
    Q_PROPERTY(QColor endColor READ endColor WRITE setEndColor NOTIFY endColorChanged)
    QColor endColor() const { return m_endColor; }
    void setEndColor(const QColor &color);
    
    // 设置轨迹线宽
    Q_PROPERTY(float lineWidth READ lineWidth WRITE setLineWidth NOTIFY lineWidthChanged)
    float lineWidth() const { return m_lineWidth; }
    void setLineWidth(float width);
    
    // 设置轨迹半径（km）
    Q_PROPERTY(double trajectoryRadius READ trajectoryRadius WRITE setTrajectoryRadius NOTIFY trajectoryRadiusChanged)
    double trajectoryRadius() const { return m_trajectoryRadius; }
    void setTrajectoryRadius(double radius);
    
    // 轨迹点数量
    Q_PROPERTY(int pointCount READ pointCount NOTIFY pointCountChanged)
    int pointCount() const { return m_trajectoryPoints.size(); }
    
signals:
    void startColorChanged();
    void endColorChanged();
    void lineWidthChanged();
    void trajectoryRadiusChanged();
    void pointCountChanged();
    
private:
    // 更新几何体数据
    void updateGeometry();
    
    // 大圆弧插值（球面采样）
    QVector<QVector3D> interpolateGreatCircle(const QVector3D &start, const QVector3D &end, int segments);
    
    // 经纬度转3D坐标
    QVector3D llaToCartesian(double lat, double lon, double radius);
    
    // 顶点数据
    QByteArray m_vertexData;
    
    // 轨迹点（经纬度）
    QVector<QPair<double, double>> m_trajectoryPoints;
    
    // 3D顶点序列
    QVector<QVector3D> m_vertices;
    
    // 颜色
    QColor m_startColor = QColor(0, 255, 0);      // 绿色起点
    QColor m_endColor = QColor(0, 200, 255);      // 青色终点
    
    // 线宽
    float m_lineWidth = 2.0f;
    
    // 轨迹半径（km）
    double m_trajectoryRadius = 1740.0;  // 月球表面上方
    
    // 大圆弧采样段数
    static constexpr int GREAT_CIRCLE_SEGMENTS = 15;
};