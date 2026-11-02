#pragma once

#include <QtQuick3D/qquick3dgeometry.h>
#include <QVector3D>
#include <QVariantList>
#include <QTimer>

/**
 * @brief 统一轨迹线几何体 —— 交叉双四边形 ribbon（Triangle 拓扑）。
 *
 * 设计目标（替代 OrbitLine Cube 链 / MoonTrajectoryGeometry Lines 等多套实现）：
 *  - 单节点单 draw call：万点级全量渲染，无需跳点采样
 *  - 每段两片正交四边形：任意视角可见（与 Cube 观感等效，无立方体接缝）
 *  - thickness 控制线宽（取所用坐标系单位，如 km 轨迹即 km）
 *  - drawCount 支持回放动画的部分显示（-1 = 全部）
 *  - appendPoint 流式追加（内部 33ms 节流批量重建，推流期 O(n)）
 *
 * 顶点布局：仅位置（3 float），颜色由 Material 的 diffuseColor 控制。
 */
class TrajectoryLineGeometry : public QQuick3DGeometry {
    QML_ELEMENT
    Q_OBJECT

    // 轨迹点（与组件同坐标系；QVector3D 列表）
    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
    // 线宽（坐标系单位下的全宽）
    Q_PROPERTY(float thickness READ thickness WRITE setThickness NOTIFY thicknessChanged)
    // 部分显示的点数（回放动画）；-1 表示全部
    Q_PROPERTY(int drawCount READ drawCount WRITE setDrawCount NOTIFY drawCountChanged)

public:
    explicit TrajectoryLineGeometry(QQuick3DObject *parent = nullptr);

    QVariantList points() const;
    void setPoints(const QVariantList &points);

    float thickness() const { return m_thickness; }
    void setThickness(float thickness);

    int drawCount() const { return m_drawCount; }
    void setDrawCount(int count);

    // 流式追加（内部 33ms 节流，批量重建）
    Q_INVOKABLE void appendPoint(const QVector3D &position);
    Q_INVOKABLE void clear();

    int pointCount() const { return m_points.size(); }

signals:
    void pointsChanged();
    void thicknessChanged();
    void drawCountChanged();

private:
    // 依据 m_points + m_drawCount 全量重建顶点缓冲
    void rebuild();

    // 单段(a→b)生成 12 顶点（两片正交四边形）写入 ptr，返回写入的顶点数
    static int appendSegmentVertices(float *ptr, const QVector3D &a,
                                     const QVector3D &b, float halfWidth);

    QVector<QVector3D> m_points;
    float m_thickness = 0.4f;
    int m_drawCount = -1;

    // 流式追加缓冲
    QVector<QVector3D> m_pending;
    QTimer m_flushTimer;
};
