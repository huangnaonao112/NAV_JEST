#include "TrajectoryLineGeometry.h"

#include <QMatrix4x4>
#include <QtMath>

namespace
{

constexpr float kMinSegmentLength = 1e-6f;

// 求与 dir 正交的一组基（u, v），dir 需为单位向量
void perpendicularBasis(const QVector3D &dir, QVector3D &u, QVector3D &v)
{
    QVector3D ref(1.0f, 0.0f, 0.0f);
    if (qAbs(QVector3D::dotProduct(dir, ref)) > 0.9f)
        ref = QVector3D(0.0f, 1.0f, 0.0f);
    u = QVector3D::crossProduct(dir, ref).normalized();
    v = QVector3D::crossProduct(dir, u).normalized();
}

} // namespace

TrajectoryLineGeometry::TrajectoryLineGeometry(QQuick3DObject *parent)
    : QQuick3DGeometry(parent)
{
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic, 0,
                 QQuick3DGeometry::Attribute::F32Type);
    setStride(3 * sizeof(float));

    m_flushTimer.setInterval(33);
    m_flushTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, [this]() {
        if (m_pending.isEmpty())
            return;
        m_points.append(m_pending);
        m_pending.clear();
        rebuild();
        emit pointsChanged();
    });
}

QVariantList TrajectoryLineGeometry::points() const
{
    QVariantList result;
    result.reserve(m_points.size());
    for (const QVector3D &p : m_points)
        result.append(QVariant::fromValue(p));
    return result;
}

void TrajectoryLineGeometry::setPoints(const QVariantList &points)
{
    m_pending.clear();
    m_flushTimer.stop();
    m_points.clear();
    m_points.reserve(points.size());
    for (const QVariant &v : points)
        m_points.append(v.value<QVector3D>());
    rebuild();
    emit pointsChanged();
}

void TrajectoryLineGeometry::setThickness(float thickness)
{
    if (qFuzzyCompare(m_thickness, thickness))
        return;
    m_thickness = thickness;
    rebuild();
    emit thicknessChanged();
}

void TrajectoryLineGeometry::setDrawCount(int count)
{
    const int clamped = qMax(-1, count);
    if (m_drawCount == clamped)
        return;
    m_drawCount = clamped;
    rebuild();
    emit drawCountChanged();
}

void TrajectoryLineGeometry::appendPoint(const QVector3D &position)
{
    m_pending.append(position);
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void TrajectoryLineGeometry::clear()
{
    m_pending.clear();
    m_flushTimer.stop();
    m_points.clear();
    rebuild();
    emit pointsChanged();
}

void TrajectoryLineGeometry::rebuild()
{
    const int total = m_points.size();
    const int visible = (m_drawCount < 0) ? total : qMin(m_drawCount, total);

    QByteArray vertexData;
    if (visible < 2) {
        setVertexData(vertexData);
        setBounds(QVector3D(), QVector3D());
        update();
        return;
    }

    const float halfWidth = m_thickness / 2.0f;
    // 每段 12 顶点（两片四边形 × 2 三角形 × 3 顶点）
    const int segmentCount = visible - 1;
    const int vertexCount = segmentCount * 12;
    vertexData.resize(vertexCount * 3 * sizeof(float));
    float *ptr = reinterpret_cast<float *>(vertexData.data());

    QVector3D minBound = m_points[0];
    QVector3D maxBound = m_points[0];

    for (int i = 0; i < segmentCount; ++i) {
        const QVector3D &a = m_points[i];
        const QVector3D &b = m_points[i + 1];
        ptr += appendSegmentVertices(ptr, a, b, halfWidth);

        minBound.setX(qMin(minBound.x(), qMin(a.x(), b.x())));
        minBound.setY(qMin(minBound.y(), qMin(a.y(), b.y())));
        minBound.setZ(qMin(minBound.z(), qMin(a.z(), b.z())));
        maxBound.setX(qMax(maxBound.x(), qMax(a.x(), b.x())));
        maxBound.setY(qMax(maxBound.y(), qMax(a.y(), b.y())));
        maxBound.setZ(qMax(maxBound.z(), qMax(a.z(), b.z())));
    }
    setBounds(minBound, maxBound);

    setVertexData(vertexData);
    update();
}

int TrajectoryLineGeometry::appendSegmentVertices(float *ptr, const QVector3D &a,
                                                  const QVector3D &b,
                                                  float halfWidth)
{
    const QVector3D diff = b - a;
    const float len = diff.length();
    if (len < kMinSegmentLength) {
        // 退化段：写入 12 个重合顶点，保持顶点数对齐（不可见）
        for (int i = 0; i < 12; ++i) {
            *ptr++ = a.x(); *ptr++ = a.y(); *ptr++ = a.z();
        }
        return 12;
    }

    const QVector3D dir = diff / len;
    QVector3D u, v;
    perpendicularBasis(dir, u, v);

    const QVector3D aU = a + u * halfWidth, aVU = a - u * halfWidth;
    const QVector3D bU = b + u * halfWidth, bVU = b - u * halfWidth;
    const QVector3D aV = a + v * halfWidth, aVV = a - v * halfWidth;
    const QVector3D bV = b + v * halfWidth, bVV = b - v * halfWidth;

    // 第一片（u 方向）：两个三角形
    *ptr++ = aU.x(); *ptr++ = aU.y(); *ptr++ = aU.z();
    *ptr++ = bU.x(); *ptr++ = bU.y(); *ptr++ = bU.z();
    *ptr++ = aVU.x(); *ptr++ = aVU.y(); *ptr++ = aVU.z();

    *ptr++ = bU.x(); *ptr++ = bU.y(); *ptr++ = bU.z();
    *ptr++ = bVU.x(); *ptr++ = bVU.y(); *ptr++ = bVU.z();
    *ptr++ = aVU.x(); *ptr++ = aVU.y(); *ptr++ = aVU.z();

    // 第二片（v 方向，正交于第一片，任意视角可见）
    *ptr++ = aV.x(); *ptr++ = aV.y(); *ptr++ = aV.z();
    *ptr++ = bV.x(); *ptr++ = bV.y(); *ptr++ = bV.z();
    *ptr++ = aVV.x(); *ptr++ = aVV.y(); *ptr++ = aVV.z();

    *ptr++ = bV.x(); *ptr++ = bV.y(); *ptr++ = bV.z();
    *ptr++ = bVV.x(); *ptr++ = bVV.y(); *ptr++ = bVV.z();
    *ptr++ = aVV.x(); *ptr++ = aVV.y(); *ptr++ = aVV.z();

    return 12;
}
