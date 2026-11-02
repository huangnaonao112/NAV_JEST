#include "TrajectoryLineGeometry.h"
#include <QDebug>
#include <QFile>

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
    // 兼容三种元素形态：QVector3D / [x,y,z] 列表 / {x,y,z} Map
    // （QML JS 层传递后 vector3d 可能退化为 list 或 map）
    for (const QVariant &v : points) {
        if (v.canConvert<QVector3D>()) {
            m_points.append(v.value<QVector3D>());
        } else if (v.type() == QVariant::List) {
            const QVariantList xyz = v.toList();
            if (xyz.size() >= 3)
                m_points.append(QVector3D(xyz[0].toFloat(), xyz[1].toFloat(),
                                          xyz[2].toFloat()));
        } else if (v.type() == QVariant::Map) {
            const QVariantMap m = v.toMap();
            m_points.append(QVector3D(m.value(QStringLiteral("x")).toFloat(),
                                      m.value(QStringLiteral("y")).toFloat(),
                                      m.value(QStringLiteral("z")).toFloat()));
        }
    }
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

    if (visible < 2) {
        setVertexData(QByteArray());
        setBounds(QVector3D(), QVector3D());
        update();
        return;
    }

    QVector3D minBound, maxBound;
    const QByteArray vertexData = buildVertexData(m_points, visible, m_thickness,
                                                  &minBound, &maxBound);
    setBounds(minBound, maxBound);

    // 诊断钩子: NAVJEST_GEOM_DEBUG=1 时把几何体关键数值追加到 geom_debug.txt
    // （Release GUI 下 qWarning 走 OutputDebugString 不进 stderr，故直接写文件）
    static const bool geomDebug = !qEnvironmentVariableIsEmpty("NAVJEST_GEOM_DEBUG");
    if (geomDebug) {
        QFile dbg(QStringLiteral("geom_debug.txt"));
        if (dbg.open(QIODevice::Append | QIODevice::Text)) {
            dbg.write(QString("TrajGeom: pts=%1 visible=%2 bytes=%3 thickness=%4 "
                              "bounds=(%5,%6,%7)-(%8,%9,%10)\n")
                .arg(total).arg(visible).arg(vertexData.size())
                .arg(m_thickness, 0, 'f', 3)
                .arg(minBound.x(), 0, 'f', 1).arg(minBound.y(), 0, 'f', 1)
                .arg(minBound.z(), 0, 'f', 1)
                .arg(maxBound.x(), 0, 'f', 1).arg(maxBound.y(), 0, 'f', 1)
                .arg(maxBound.z(), 0, 'f', 1).toUtf8());
        }
    }

    setVertexData(vertexData);
    update();
}

QByteArray TrajectoryLineGeometry::buildVertexData(const QVector<QVector3D> &points,
                                                   int visibleCount, float thickness,
                                                   QVector3D *minOut, QVector3D *maxOut)
{
    const int count = qMin(visibleCount, points.size());
    if (count < 2)
        return QByteArray();

    const float halfWidth = thickness / 2.0f;
    const int segmentCount = count - 1;
    const int vertexCount = segmentCount * 12;

    QByteArray vertexData(vertexCount * 3 * sizeof(float), Qt::Uninitialized);
    float *ptr = reinterpret_cast<float *>(vertexData.data());

    QVector3D minBound = points[0];
    QVector3D maxBound = points[0];

    for (int i = 0; i < segmentCount; ++i) {
        const QVector3D &a = points[i];
        const QVector3D &b = points[i + 1];
        // appendSegmentVertices 每段写 36 个 float；ptr 是 float*，按 float 个数步进
        ptr += appendSegmentVertices(ptr, a, b, halfWidth);

        minBound.setX(qMin(minBound.x(), qMin(a.x(), b.x())));
        minBound.setY(qMin(minBound.y(), qMin(a.y(), b.y())));
        minBound.setZ(qMin(minBound.z(), qMin(a.z(), b.z())));
        maxBound.setX(qMax(maxBound.x(), qMax(a.x(), b.x())));
        maxBound.setY(qMax(maxBound.y(), qMax(a.y(), b.y())));
        maxBound.setZ(qMax(maxBound.z(), qMax(a.z(), b.z())));
    }

    // 断言式校验：所有 float 必须被写满（ptr 正好走到缓冲末尾）。
    // 这里用 Q_ASSERT 兜住“步进单位写错”这类回归（之前 12 vs 36 的 bug 即此）。
    Q_ASSERT(ptr == reinterpret_cast<const float *>(vertexData.constData()) + vertexCount * 3);

    if (minOut)
        *minOut = minBound;
    if (maxOut)
        *maxOut = maxBound;
    return vertexData;
}

int TrajectoryLineGeometry::appendSegmentVertices(float *ptr, const QVector3D &a,
                                                  const QVector3D &b,
                                                  float halfWidth)
{
    const QVector3D diff = b - a;
    const float len = diff.length();
    if (len < kMinSegmentLength) {
        // 退化段：写入 12 个重合顶点，返回写入的 float 个数
        for (int i = 0; i < 12; ++i) {
            *ptr++ = a.x(); *ptr++ = a.y(); *ptr++ = a.z();
        }
        return 36;
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

    return 36;
}
