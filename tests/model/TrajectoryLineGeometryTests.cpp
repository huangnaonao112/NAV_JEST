#include <QtTest>
#include <QVector3D>
#include <QVector>
#include <cmath>

#include "models/TrajectoryLineGeometry.h"

/**
 * TrajectoryLineGeometry 顶点构建回归测试。
 *
 * 背景：曾出现 appendSegmentVertices 返回“顶点数 12”而 ptr 是 float*（每段实写
 * 36 个 float）的步进 bug，导致缓冲区后 2/3 是未初始化内存，渲染出随机超大三角形
 * （表现为整屏绿色色块）。这类错误不会崩溃、不会被编译器发现，必须靠数据校验兜住。
 */
class TrajectoryLineGeometryTests : public QObject
{
    Q_OBJECT

private slots:
    void fullyWritesBuffer();
    void verticesStayNearPolyline();
    void degenerateSegmentIsCollapsed();
    void ringThicknessIsRespected();

private:
    static QVector<QVector3D> makePolyline()
    {
        return { QVector3D(0, 0, 0), QVector3D(1000, 0, 0),
                 QVector3D(1000, 1000, 0), QVector3D(0, 1000, 0),
                 QVector3D(0, 1000, 1000) };
    }

    static bool allFinite(const QByteArray &data)
    {
        const float *f = reinterpret_cast<const float *>(data.constData());
        const int n = data.size() / int(sizeof(float));
        for (int i = 0; i < n; ++i) {
            if (!std::isfinite(f[i]))
                return false;
        }
        return true;
    }
};

void TrajectoryLineGeometryTests::fullyWritesBuffer()
{
    const QVector<QVector3D> pts = makePolyline();
    const float thickness = 20.0f;
    QVector3D minB, maxB;
    const QByteArray data = TrajectoryLineGeometry::buildVertexData(
        pts, pts.size(), thickness, &minB, &maxB);

    const int segments = pts.size() - 1;
    QCOMPARE(data.size(), segments * 12 * 3 * int(sizeof(float)));
    QVERIFY(allFinite(data));
    QCOMPARE(minB, QVector3D(0, 0, 0));
    QCOMPARE(maxB, QVector3D(1000, 1000, 1000));
}

void TrajectoryLineGeometryTests::verticesStayNearPolyline()
{
    const QVector<QVector3D> pts = makePolyline();
    const float thickness = 20.0f;
    const QByteArray data = TrajectoryLineGeometry::buildVertexData(
        pts, pts.size(), thickness);

    const float *f = reinterpret_cast<const float *>(data.constData());
    const int vertexCount = data.size() / (3 * int(sizeof(float)));
    const float halfWidth = thickness / 2.0f;
    // 两个正交面片的角点离轴心最远为本段端点 + halfWidth·√2
    const float tol = halfWidth * std::sqrt(2.0f) + 1e-3f;

    for (int v = 0; v < vertexCount; ++v) {
        const QVector3D p(f[v * 3], f[v * 3 + 1], f[v * 3 + 2]);
        const int seg = v / 12;
        const QVector3D &a = pts[seg];
        const QVector3D &b = pts[seg + 1];
        const QVector3D ab = b - a;
        const float len2 = ab.lengthSquared();
        float t = len2 > 0 ? QVector3D::dotProduct(p - a, ab) / len2 : 0.0f;
        t = qBound(0.0f, t, 1.0f);
        const QVector3D closest = a + ab * t;
        const float dist = (p - closest).length();
        QVERIFY2(dist <= tol,
                 qPrintable(QStringLiteral("vertex %1 off polyline: dist=%2 tol=%3")
                            .arg(v).arg(dist).arg(tol)));
    }
}

void TrajectoryLineGeometryTests::degenerateSegmentIsCollapsed()
{
    const QVector<QVector3D> pts = { QVector3D(5, 6, 7), QVector3D(5, 6, 7) };
    const QByteArray data = TrajectoryLineGeometry::buildVertexData(
        pts, pts.size(), 10.0f);

    QCOMPARE(data.size(), 12 * 3 * int(sizeof(float)));
    const float *f = reinterpret_cast<const float *>(data.constData());
    for (int i = 0; i < 12; ++i) {
        QCOMPARE(f[i * 3 + 0], 5.0f);
        QCOMPARE(f[i * 3 + 1], 6.0f);
        QCOMPARE(f[i * 3 + 2], 7.0f);
    }
}

void TrajectoryLineGeometryTests::ringThicknessIsRespected()
{
    // 低轨轨道环：半径 12000 km、48 段、线宽 40 km（场景 1 单位 = 1 km）
    constexpr int kSegments = 48;
    constexpr float kRadius = 12000.0f;
    constexpr float kThickness = 40.0f;

    QVector<QVector3D> ring;
    for (int i = 0; i <= kSegments; ++i) {
        const float ang = 2.0f * float(M_PI) * float(i) / kSegments;
        ring.append(QVector3D(kRadius * std::cos(ang), 0.0f, kRadius * std::sin(ang)));
    }

    QVector3D minB, maxB;
    const QByteArray data = TrajectoryLineGeometry::buildVertexData(
        ring, ring.size(), kThickness, &minB, &maxB);

    const float *f = reinterpret_cast<const float *>(data.constData());
    const int vertexCount = data.size() / (3 * int(sizeof(float)));
    for (int v = 0; v < vertexCount; ++v) {
        const QVector3D p(f[v * 3], f[v * 3 + 1], f[v * 3 + 2]);
        const float r = p.length();
        QVERIFY2(r <= kRadius + kThickness,
                 qPrintable(QStringLiteral("vertex %1 radius %2 exceeds ring+thickness")
                            .arg(v).arg(r)));
    }
}

QTEST_APPLESS_MAIN(TrajectoryLineGeometryTests)
#include "TrajectoryLineGeometryTests.moc"
