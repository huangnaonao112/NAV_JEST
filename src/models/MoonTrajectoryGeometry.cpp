#include "MoonTrajectoryGeometry.h"
#include "OrbitalPhysics.h"
#include <QByteArray>
#include <QVector3D>
#include <QtMath>
#include <QDebug>

MoonTrajectoryGeometry::MoonTrajectoryGeometry()
{
    // 设置图元类型为线条
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Lines);

    // 只设置位置属性（颜色由Material的diffuseColor控制）
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0, QQuick3DGeometry::Attribute::F32Type);

    // 设置步长：3 floats = 12 bytes（只有位置）
    setStride(3 * sizeof(float));

    // 设置初始颜色（绿色）
    m_startColor = QColor(0, 255, 0);
    m_endColor = QColor(0, 200, 255);

    // 33ms 节流：批量消化推流期间的轨迹点，避免逐点全量重建几何体
    m_flushTimer.setInterval(33);
    m_flushTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, &MoonTrajectoryGeometry::flushPendingPoints);
}

void MoonTrajectoryGeometry::addTrajectoryPoint(double lat, double lon)
{
    // 推流期高频调用：先入缓冲区，由 33ms 定时器批量刷新
    m_pendingPoints.append(qMakePair(lat, lon));
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void MoonTrajectoryGeometry::flushPendingPoints()
{
    if (m_pendingPoints.isEmpty())
        return;

    for (const auto &point : m_pendingPoints)
    {
        m_trajectoryPoints.append(point);

        QVector3D newPos = llaToCartesian(point.first, point.second, m_trajectoryRadius);

        if (!m_vertices.isEmpty())
        {
            // 与上一个点之间生成大圆弧中间点
            const QVector<QVector3D> arcPoints =
                interpolateGreatCircle(m_vertices.last(), newPos, GREAT_CIRCLE_SEGMENTS);
            for (int i = 1; i < arcPoints.size(); ++i)
                m_vertices.append(arcPoints[i]);
        }
        else
        {
            m_vertices.append(newPos);
        }
    }
    m_pendingPoints.clear();

    updateGeometry();
    emit pointCountChanged();
}

void MoonTrajectoryGeometry::clear()
{
    m_trajectoryPoints.clear();
    m_vertices.clear();
    m_vertexData.clear();
    m_pendingPoints.clear();
    m_flushTimer.stop();

    updateGeometry();

    emit pointCountChanged();
}

void MoonTrajectoryGeometry::setStartColor(const QColor &color)
{
    if (m_startColor != color) {
        m_startColor = color;
        updateGeometry();
        emit startColorChanged();
    }
}

void MoonTrajectoryGeometry::setEndColor(const QColor &color)
{
    if (m_endColor != color) {
        m_endColor = color;
        updateGeometry();
        emit endColorChanged();
    }
}

void MoonTrajectoryGeometry::setLineWidth(float width)
{
    if (!qFuzzyCompare(m_lineWidth, width)) {
        m_lineWidth = width;
        emit lineWidthChanged();
    }
}

void MoonTrajectoryGeometry::setTrajectoryRadius(double radius)
{
    if (!qFuzzyCompare(m_trajectoryRadius, radius)) {
        m_trajectoryRadius = radius;
        // 需要重新计算所有顶点
        m_vertices.clear();
        for (const auto &point : m_trajectoryPoints) {
            m_vertices.append(llaToCartesian(point.first, point.second, m_trajectoryRadius));
        }
        updateGeometry();
        emit trajectoryRadiusChanged();
    }
}

void MoonTrajectoryGeometry::updateGeometry()
{
    m_vertexData.clear();

    if (m_vertices.size() < 2) {
        setVertexData(m_vertexData);
        setBounds(QVector3D(), QVector3D());
        update();
        return;
    }
    
    // 预分配空间：每条线段2个顶点，每个顶点3个float（位置）
    int vertexCount = (m_vertices.size() - 1) * 2;
    m_vertexData.resize(vertexCount * 3 * sizeof(float));
    
    float *ptr = reinterpret_cast<float*>(m_vertexData.data());

    // 为每条线段添加2个顶点（直接使用km单位，与GroundWarningItem一致）
    for (int i = 0; i < m_vertices.size() - 1; ++i) {
        // 顶点1
        QVector3D v1 = m_vertices[i];
        *ptr++ = v1.x();
        *ptr++ = v1.y();
        *ptr++ = v1.z();
        
        // 顶点2
        QVector3D v2 = m_vertices[i + 1];
        *ptr++ = v2.x();
        *ptr++ = v2.y();
        *ptr++ = v2.z();
    }
    
    setVertexData(m_vertexData);

    // 计算包围盒
    QVector3D minBound = m_vertices[0];
    QVector3D maxBound = m_vertices[0];
    for (const QVector3D &v : m_vertices) {
        minBound.setX(qMin(minBound.x(), v.x()));
        minBound.setY(qMin(minBound.y(), v.y()));
        minBound.setZ(qMin(minBound.z(), v.z()));
        maxBound.setX(qMax(maxBound.x(), v.x()));
        maxBound.setY(qMax(maxBound.y(), v.y()));
        maxBound.setZ(qMax(maxBound.z(), v.z()));
    }
    setBounds(minBound, maxBound);

    update();
}

QVector<QVector3D> MoonTrajectoryGeometry::interpolateGreatCircle(const QVector3D &start, const QVector3D &end, int segments)
{
    QVector<QVector3D> result;
    result.append(start);
    
    // 归一化向量
    QVector3D a = start.normalized();
    QVector3D b = end.normalized();
    
    // 计算夹角
    float dot = QVector3D::dotProduct(a, b);
    dot = qBound(-1.0f, dot, 1.0f);  // 防止数值误差
    float angle = qAcos(dot);
    
    if (angle < 0.001f) {
        // 角度太小，直接返回起点和终点
        result.append(end);
        return result;
    }
    
    // 大圆弧插值
    QVector3D axis = QVector3D::crossProduct(a, b).normalized();
    
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / segments;
        float theta = angle * t;
        
        // 使用四元数旋转向量（Rodrigues旋转公式）
        QVector3D v = a * qCos(theta) + 
                      QVector3D::crossProduct(axis, a) * qSin(theta) + 
                      axis * QVector3D::dotProduct(axis, a) * (1.0f - qCos(theta));
        
        // 恢复原始半径
        float radius = start.length();
        result.append(v * radius);
    }
    
    return result;
}

QVector3D MoonTrajectoryGeometry::llaToCartesian(double lat, double lon, double radius)
{
    // 使用 OrbitalPhysics 统一的坐标转换，确保与测试点一致
    return OrbitalPhysics::llaToCartesian(lat, lon, radius);
}