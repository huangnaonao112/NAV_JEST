#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QVector>
#include <QVector3D>
#include <QVariantList>
#include <QVariantMap>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>

/**
 * @brief 月球轨迹点结构（Selenographic坐标系）
 * 存储月球表面的经纬度和高度信息
 */
struct MoonTrajectoryPoint {
    double latitude;    // 月球纬度 (-90 ~ +90)
    double longitude;   // 月球经度 (-180 ~ +180)
    double altitude;    // 相对月面高度 (km)
    QVector3D position; // 场景坐标（已转换为显示坐标）
};

/**
 * @brief 月球轨迹数据模型
 * 
 * 独立的月球轨迹存储和管理，不复用PntDataModel。
 * 使用月心固定坐标系（Selenographic），支持实时数据更新。
 * 
 * 坐标系统：
 * - 纬度：从月球赤道(0°)到两极(±90°)
 * - 经度：从中央子午线(0°, 正对地球面)到±180°
 * - 月球半径：1740 km（略高于真实半径1737.4，确保轨迹点在表面之上可见）
 * - 场景位置偏移：(80000, 0, 0)
 * 
 * 轨迹点显示方法（参照 SatelliteModel::groundPoints）：
 * 1. 使用 OrbitalPhysics::llaToCartesian(lat, lon, 1740.0) 计算位置
 * 2. 参数单位：km（与地球地面点完全一致）
 * 3. 返回 QVariantList，每项包含 position, color, type, radius 等字段
 * 4. QML 使用 Repeater3D + GroundWarningItem 渲染
 * 5. 类型值含义: 0:Base, 1:Jammer(带波纹), 2:Target(带波纹), 3:Trajectory, 4:Spoofed/FollowTrack(带波纹)
 */
class MoonTrajectoryModel : public QObject {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    
    // 轨迹数据
    Q_PROPERTY(int trajectoryCount READ trajectoryCount NOTIFY trajectoryChanged)
    
    // 当前位置（最后接收的点）
    Q_PROPERTY(double currentLatitude READ currentLatitude NOTIFY currentPositionChanged)
    Q_PROPERTY(double currentLongitude READ currentLongitude NOTIFY currentPositionChanged)
    Q_PROPERTY(double currentAltitude READ currentAltitude NOTIFY currentPositionChanged)
    
    // 当前位置的场景坐标（用于QML绑定）
    Q_PROPERTY(QVector3D currentPosition READ currentPosition NOTIFY currentPositionChanged)
    
    // 轨迹点列表（用于QML Repeater3D绑定）
    // 完全参照 SatelliteModel::groundPoints 实现：
    // - 使用 km 单位调用 OrbitalPhysics::llaToCartesian
    // - 返回 QVariantList，格式与 groundPoints 一致
    // - QML 使用 Repeater3D + GroundWarningItem 渲染
    Q_PROPERTY(QVariantList trajectoryPoints READ trajectoryPoints NOTIFY trajectoryChanged)

    // 最近轨迹点（轻量：只含末尾 RECENT_POINTS_LIMIT 个，供标记 Repeater 绑定，
    // 避免万点级全量 QVariantList 重建）
    Q_PROPERTY(QVariantList recentPoints READ recentPoints NOTIFY trajectoryChanged)
    
    // 测试点列表（固定的参考点，绿色显示）
    Q_PROPERTY(QVariantList testPoints READ testPoints NOTIFY testPointsChanged)
    
    // 轨迹线点列表（贴着球面的连线，用于QML渲染）
    Q_PROPERTY(QVariantList trajectoryLinePoints READ trajectoryLinePoints NOTIFY trajectoryChanged)
    
    // 测试点轨迹线段（用于QML渲染）
    // 使用球面插值生成密集点，QML端用大球体渲染形成连续线效果
    Q_PROPERTY(QVariantList testLineSegments READ testLineSegments NOTIFY testPointsChanged)
    
    // 轨迹纹理（用于2D贴图方案）
    Q_PROPERTY(QString trajectoryTextureUrl READ trajectoryTextureUrl NOTIFY testPointsChanged)
    Q_PROPERTY(int trajectoryTextureVersion READ trajectoryTextureVersion NOTIFY testPointsChanged)
    
public:
    explicit MoonTrajectoryModel(QObject *parent = nullptr);
    
    // 轨迹数量
    int trajectoryCount() const;
    
    // 当前位置
    double currentLatitude() const { return m_currentLat; }
    double currentLongitude() const { return m_currentLon; }
    double currentAltitude() const { return m_currentAlt; }
    QVector3D currentPosition() const { return m_currentPosition; }
    
    // 轨迹点列表（用于QML）
    QVariantList trajectoryPoints() const;

    // 最近轨迹点（末尾 RECENT_POINTS_LIMIT 个，轻量）
    QVariantList recentPoints() const;
    
    // 测试点列表（用于QML）
    QVariantList testPoints() const;
    
    // 轨迹线点列表（贴着球面，用于QML渲染）
    QVariantList trajectoryLinePoints() const;
    
    // 测试点轨迹线段（用于QML渲染）
    // 使用球面插值生成密集点，QML端用大球体渲染形成连续线效果
    QVariantList testLineSegments() const;
    
    // 轨迹纹理（用于2D贴图方案）
    QString trajectoryTextureUrl() const;
    int trajectoryTextureVersion() const { return m_textureVersion; }
    
    // 轨迹点访问（QML调用）
    Q_INVOKABLE double trajectoryLat(int index) const;
    Q_INVOKABLE double trajectoryLon(int index) const;
    Q_INVOKABLE double trajectoryAlt(int index) const;
    Q_INVOKABLE QVector3D trajectoryPosition(int index) const;
    
    // 清空轨迹
    Q_INVOKABLE void clear();
    
    // 动态轨迹接口（供QML调用）
    // 添加单个轨迹点并更新纹理
    Q_INVOKABLE void appendTrajectoryPoint(double lat, double lon);
    
    // 清空轨迹纹理
    Q_INVOKABLE void clearTrajectoryTexture();
    
    // 从MNT报文更新数据
    // @param lat 月球纬度（度）
    // @param lon 月球经度（度）
    // @param alt 相对月面高度（km）
    void updateFromMoonTraj(double lat, double lon, double alt);
    
    // 设置纹理提供者（由main.cpp调用）
    void setTrajectoryProvider(class MoonTrajectoryProvider* provider);
    
signals:
    void trajectoryChanged();
    void currentPositionChanged();
    void testPointsChanged();
    void newTrajectoryPoint(double lat, double lon);  // 新轨迹点信号（用于轨迹线更新）
    void newTrajectoryPoints(const QVariantList &points);  // 批量新点（节流窗口flush后发）

private:
    // 生成标准测试点（5个关键位置）
    void generateTestPoints();

    // 33ms 节流窗口：把缓冲点入库并统一发信号（推流期避免逐点信号风暴）
    void flushPendingTrajectory();
    
    // 球面线性插值（Slerp）- 生成贴着球面的连线点
    // @param start 起点位置（单位向量）
    // @param end 终点位置（单位向量）
    // @param radius 球面半径（km）
    // @param segments 分段数
    // @return 插值点列表
    static QVariantList generateSlerpPoints(const QVector3D& start, const QVector3D& end, 
                                            double radius, int segments);
    
    // 轨迹数据存储
    QVector<MoonTrajectoryPoint> m_trajectory;
    mutable QMutex m_trajectoryMutex;
    
    // 测试点存储（固定的参考点）
    QVariantList m_testPoints;
    
    // 测试点轨迹线缓存
    QVariantList m_testLinePoints;
    
    // 当前位置
    double m_currentLat = 0.0;
    double m_currentLon = 0.0;
    double m_currentAlt = 0.0;
    QVector3D m_currentPosition;
    
    // 轨迹点最大数量限制
    static constexpr int MAX_TRAJECTORY_POINTS = 10000;

    // 标记渲染上限：Repeater 只画最近 N 个点（全量历史仍在 m_trajectory）
    static constexpr int RECENT_POINTS_LIMIT = 60;

    // 信号发射计数器（每10个点发射一次）
    int m_emitCounter = 0;

    // 33ms 节流：推流期缓冲点，定时合并发信号
    QVector<MoonTrajectoryPoint> m_pendingTrajectory;
    QTimer m_flushTimer;
    
    // 轨迹纹理提供者
    class MoonTrajectoryProvider* m_trajectoryProvider;
    int m_textureVersion = 0;
};