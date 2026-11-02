#include "MoonTrajectoryModel.h"
#include "MoonTrajectoryProvider.h"
#include "OrbitalPhysics.h"
#include "CoordinateSystem.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QtMath>
#include <algorithm>  // for std::min and std::max if needed

MoonTrajectoryModel::MoonTrajectoryModel(QObject *parent)
    : QObject(parent)
    , m_currentLat(0.0)
    , m_currentLon(0.0)
    , m_currentAlt(0.0)
    , m_currentPosition(0.0f, 0.0f, 0.0f)
    , m_emitCounter(0)
    , m_trajectoryProvider(nullptr)
{
    // 33ms 节流：推流期缓冲点，批量入库并发信号
    m_flushTimer.setInterval(33);
    m_flushTimer.setSingleShot(true);
    connect(&m_flushTimer, &QTimer::timeout, this, &MoonTrajectoryModel::flushPendingTrajectory);

    // 生成测试点（绿色参考点）
    generateTestPoints();
}

// 轨迹数量
int MoonTrajectoryModel::trajectoryCount() const {
    QMutexLocker locker(&m_trajectoryMutex);
    return m_trajectory.size();
}

// 轨迹点列表（用于QML Repeater3D）
// 完全参照测试点的实现方式
// 
// 关键：直接传 km 单位，不做转换！测试点就是这样工作的。
// 
QVariantList MoonTrajectoryModel::trajectoryPoints() const {
    QMutexLocker locker(&m_trajectoryMutex);
    QVariantList result;
    
    const int count = m_trajectory.size();
    
    for (int i = 0; i < count; ++i) {
        const auto& point = m_trajectory[i];
        QVariantMap item;
        
        // 和测试点完全一样：直接传 km 单位
        item["position"] = point.position;  // 直接使用km单位
        item["lat"] = point.latitude;
        item["lon"] = point.longitude;
        item["name"] = QString("报文点%1").arg(i + 1);
        
        // 关键区别：最新点 vs 历史点（与测试点相同的美观效果）
        bool isLatest = (i == count - 1);
        
        if (isLatest) {
            // 最新点：稍微大一点 + 波纹动画
            item["type"] = 3;           // TrajectoryPoint 类型，有波纹动画
            item["radius"] = 120.0;     // km（场景中1.2单位）
            item["color"] = QColor("#FF0000");  // 明亮红色
        } else {
            // 历史点：固定小尺寸 + 无动画
            item["type"] = 0;           // Base 类型，无波纹动画
            item["radius"] = 60.0;      // km（场景中0.6单位，固定小尺寸）
            
            // 颜色渐变：越旧的点越暗（保持颜色渐变效果）
            double fadeFactor = static_cast<double>(i) / count;
            int redValue = 100 + static_cast<int>(155 * fadeFactor);  // 100-255
            item["color"] = QColor(redValue, 0, 0);  // 渐变红色
        }
        item["errorRadius"] = 0.0;
        
        result.append(item);
    }
    
    return result;
}

// 最近轨迹点（末尾 RECENT_POINTS_LIMIT 个，供标记 Repeater 轻量绑定）
QVariantList MoonTrajectoryModel::recentPoints() const {
    QMutexLocker locker(&m_trajectoryMutex);

    const int count = m_trajectory.size();
    const int start = qMax(0, count - RECENT_POINTS_LIMIT);

    QVariantList result;
    result.reserve(count - start);
    for (int i = start; i < count; ++i) {
        const auto& point = m_trajectory[i];
        QVariantMap item;
        item["position"] = point.position;
        item["lat"] = point.latitude;
        item["lon"] = point.longitude;
        item["name"] = QString("报文点%1").arg(i + 1);

        const bool isLatest = (i == count - 1);
        if (isLatest) {
            item["type"] = 3;           // Trajectory 类型，波纹动画
            item["radius"] = 120.0;
            item["color"] = QColor("#FF0000");
        } else {
            item["type"] = 0;
            item["radius"] = 60.0;
            const double fadeFactor = static_cast<double>(i) / count;
            const int redValue = 100 + static_cast<int>(155 * fadeFactor);
            item["color"] = QColor(redValue, 0, 0);
        }
        item["errorRadius"] = 0.0;
        result.append(item);
    }
    return result;
}

// 轨迹点访问
double MoonTrajectoryModel::trajectoryLat(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].latitude;
    }
    return 0.0;
}

double MoonTrajectoryModel::trajectoryLon(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].longitude;
    }
    return 0.0;
}

double MoonTrajectoryModel::trajectoryAlt(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].altitude;
    }
    return 0.0;
}

QVector3D MoonTrajectoryModel::trajectoryPosition(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].position;
    }
    return QVector3D();
}

// 清空轨迹
void MoonTrajectoryModel::clear() {
    {
        QMutexLocker locker(&m_trajectoryMutex);
        m_trajectory.clear();
    }
    m_pendingTrajectory.clear();
    m_flushTimer.stop();
    m_currentLat = 0.0;
    m_currentLon = 0.0;
    m_currentAlt = 0.0;
    m_currentPosition = QVector3D();
    m_emitCounter = 0;

    emit trajectoryChanged();
    emit currentPositionChanged();
}

// 33ms 节流窗口到期：缓冲点批量入库，统一发一轮信号
void MoonTrajectoryModel::flushPendingTrajectory() {
    QVariantList batch;
    {
        QMutexLocker locker(&m_trajectoryMutex);
        for (MoonTrajectoryPoint &point : m_pendingTrajectory) {
            m_trajectory.append(point);
            batch.append(QVariantMap{
                {"lat", point.latitude},
                {"lon", point.longitude},
                {"alt", point.altitude},
                {"position", QVariant::fromValue(point.position)},
            });
        }
        // 超过最大数量时移除最旧的点
        while (m_trajectory.size() > MAX_TRAJECTORY_POINTS)
            m_trajectory.removeFirst();
        m_pendingTrajectory.clear();
    }

    if (batch.isEmpty())
        return;

    // 每个点仍逐个通知轨迹线几何体（Geometry 内部再缓冲），模型信号只发一轮
    for (const QVariant &item : batch) {
        const QVariantMap p = item.toMap();
        emit newTrajectoryPoint(p.value("lat").toDouble(),
                                p.value("lon").toDouble());
    }
    emit newTrajectoryPoints(batch);
    emit trajectoryChanged();
    emit currentPositionChanged();
}

// 从MNT报文更新数据
void MoonTrajectoryModel::updateFromMoonTraj(double lat, double lon, double alt) {
    // 尺度统一由 CoordinateSystem 提供：空间 1（天体局部）1 单位 = 1 km
    // 月球摆位在空间 2，x = MOON_SCENE_X
    constexpr double KM_TO_SCENE = 1.0;

    // 月球在场景中的X偏移（世界坐标系）
    constexpr double MOON_SCENE_X = CoordinateSystem::MOON_SCENE_X;

    // 1. 使用 OrbitalPhysics 转换坐标（只计算局部坐标，相对于月球中心）
    // 和地球地面点完全一样的方式！
    // 月球半径 1737.4 km，使用 MOON_TRAIL_RADIUS(1740) 确保在表面之上可见
    constexpr double MOON_RADIUS_KM = CoordinateSystem::MOON_TRAIL_RADIUS;
    QVector3D localPos = OrbitalPhysics::llaToCartesian(lat, lon, MOON_RADIUS_KM + alt);

    // 2. 更新当前位置（世界坐标，用于视角切换）
    m_currentLat = lat;
    m_currentLon = lon;
    m_currentAlt = alt;
    // currentPosition 需要世界坐标（包含80000偏移）
    m_currentPosition = QVector3D(
        localPos.x() * KM_TO_SCENE + MOON_SCENE_X,  // 加80000偏移
        localPos.y() * KM_TO_SCENE,
        localPos.z() * KM_TO_SCENE
    );

    // 3. 添加到轨迹（推流期先缓冲，33ms 批量入库）
    {
        QMutexLocker locker(&m_trajectoryMutex);

        MoonTrajectoryPoint point;
        point.latitude = lat;
        point.longitude = lon;
        point.altitude = alt;
        point.position = localPos;  // 存储局部坐标（km单位）

        m_pendingTrajectory.append(point);
    }

    // 4. 信号发射策略：推流期不再逐点发 3 个信号，
    //    由 33ms 定时器 flushPendingTrajectory() 统一通知
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

// 测试点列表（绿色参考点）
QVariantList MoonTrajectoryModel::testPoints() const {
    return m_testPoints;
}

// 生成标准测试点（绿色，用于参考）
// 生成标准测试点（绿色参考点） - 实现轨迹跟随效果
// 按顺序环绕月球生成测试点轨迹，形成完整的连接线 
void MoonTrajectoryModel::generateTestPoints() {
    m_testPoints.clear();
    m_testLinePoints.clear();
    
    // 月球半径 1740 km（确保在表面之上可见）
    constexpr double MOON_RADIUS_KM = CoordinateSystem::MOON_TRAIL_RADIUS;
    
    // 设置环月分布的参数
    const int NUM_POINTS = 36;  // 36个点分布环绕月球
    QVector<QVector3D> positions;  // 存储所有位置用于创建轨迹连接线
    
    for (int i = 0; i < NUM_POINTS; ++i) {
        // 通过将索引均匀分布在0-359度之间，创建大圆分布
        double lon = 10.0 * i;  // 每360/36 = 10度一个点，环绕整个月球
        double lat;
        
        // 在赤道附近（约22度纬度）分布，可以选择不同的模式，比如螺旋上升
        // 使用一个较为合理的月球特征纬度
        if (i < 12) {
            // 北半球点
            lat = 20.0 + (i % 3) * 2;  // 轻微变化
        } else if (i < 24) {
            // 赤道附近
            lat = 0.0 + (i % 3) * 2;
        } else {
            // 南半球点
            lat = -20.0 + (i % 3) * 2;
        }
        
        double alt = 0.0;  // 月面高度
            
        // 使用 OrbitalPhysics::llaToCartesian，返回 km 単位
        QVector3D posKm = OrbitalPhysics::llaToCartesian(lat, lon, MOON_RADIUS_KM + alt);
        
        // 存储用于后续连接线生成
        positions.append(posKm);
        
        QVariantMap item;
        item["position"] = posKm;  // 直接使用km单位，与轨迹点保持一致
        item["lat"] = lat;
        item["lon"] = lon;
        item["name"] = QString("轨迹跟踪%1").arg(i + 1);
        item["type"] = 4;  // 跟踪轨迹类型（区别于普通轨迹点））
        
        // 区分最新和非最新的点 - 最新的点更大、更亮
        if (i == NUM_POINTS - 1) {
            item["radius"] = 400.0;  // 最新跟踪点更大
            item["color"] = QColor("#00FF00");  // 明亮绿色表示当前位置
        } else {
            // 早期的轨迹点逐渐淡化显示
            double fadeFactor = static_cast<double>(i) / NUM_POINTS;
            item["radius"] = 200.0 + 200.0 * fadeFactor;    // 逐渐变小
            int greenValue = 100 + static_cast<int>(155 * fadeFactor);  // 从较暗变为明亮
            item["color"] = QColor(0, greenValue, 0);  // 渐变绿色
        }
        item["errorRadius"] = 0.0;
        
        m_testPoints.append(item);
        
        // 同时添加到测试轨迹线
        QVariantMap lineItem;
        lineItem["position"] = posKm;  // 直接使用km单位
        m_testLinePoints.append(lineItem);
    }

    // 现在生成连接相邻点的轨迹线
    for (int i = 0; i < positions.size() - 1; ++i) {
        QVector3D start = positions[i];
        QVector3D end = positions[i + 1];
        
        // 计算这两个点之间需要多少分段（根据它们之间的角度）
        double dot = QVector3D::dotProduct(start.normalized(), end.normalized());
        double angleBetween = static_cast<double>(qAcos(qBound(-1.0, dot, 1.0)));
        int segments = qMax(5, static_cast<int>(angleBetween / 0.2)); // 用固定阈值替换LINE_SEGMENT_ANGLE
        
        // 使用球面插值生成连接线
        QVariantList connectingLine = generateSlerpPoints(start, end, MOON_RADIUS_KM, segments);
        for (const auto& point : connectingLine) {
            m_testLinePoints.append(point);
        }
    }
    
    emit testPointsChanged();  // 这会使得QML端刷新testPoints和testLinePoints
    
    // 更新轨迹纹理（如果provider已设置）
    if (m_trajectoryProvider) {
        m_trajectoryProvider->setTestPoints(m_testPoints);
        m_textureVersion++;
    }
}

// 球面线性插值（Slerp）- 生成贴着球面的连线点
// 参照2D地图轨迹线的连续效果，但在3D球面上沿着大圆弧走
QVariantList MoonTrajectoryModel::generateSlerpPoints(const QVector3D& start, const QVector3D& end, 
                                                       double radius, int segments) {
    QVariantList result;
    
    if (segments < 1) segments = 1;
    
    // 确保使用归一化的方向向量
    QVector3D startPos = start.normalized();
    QVector3D endPos = end.normalized();
    
    // 计算两点间的夹角
    double dot = qBound(-1.0, QVector3D::dotProduct(startPos, endPos), 1.0);
    double theta = qAcos(dot);
    
    // 如果两点重合，则无需插值
    if (theta < 1e-6) {
        QVariantMap item;
        item["position"] = startPos * radius;
        result.append(item);
        return result;
    }
    
    double sinTheta = qSin(theta);
    
    // 从起点到终点按比例插值
    for (int i = 0; i <= segments; ++i) {
        double t = static_cast<double>(i) / segments;
        
        // 球面线性插值公式
        double weight1 = qSin((1.0 - t) * theta) / sinTheta;
        double weight2 = qSin(t * theta) / sinTheta;
        
        QVector3D interpPoint = (startPos * weight1 + endPos * weight2).normalized() * radius;
        
        QVariantMap item;
        item["position"] = interpPoint;
        result.append(item);
    }
    
    return result;
}

// 轨迹纹理URL（用于2D贴图方案）
QString MoonTrajectoryModel::trajectoryTextureUrl() const {
    return "image://moonTrajectory/texture";
}

// 设置纹理提供者（由main.cpp调用）
void MoonTrajectoryModel::setTrajectoryProvider(MoonTrajectoryProvider* provider) {
    m_trajectoryProvider = provider;
    if (m_trajectoryProvider && !m_testPoints.isEmpty()) {
        m_trajectoryProvider->setTestPoints(m_testPoints);
        m_textureVersion++;
    }
}

// 动态添加轨迹点（供QML调用）
void MoonTrajectoryModel::appendTrajectoryPoint(double lat, double lon)
{
    if (m_trajectoryProvider) {
        m_trajectoryProvider->appendTrajectoryPoint(lat, lon);
        emit trajectoryChanged();
    }
}

// 清空轨迹纹理（供QML调用）
void MoonTrajectoryModel::clearTrajectoryTexture()
{
    if (m_trajectoryProvider) {
        m_trajectoryProvider->clearTrajectory();
    }
}

// 轨迹线点列表（贴着球面，用于QML渲染）
QVariantList MoonTrajectoryModel::trajectoryLinePoints() const {
    QMutexLocker locker(&m_trajectoryMutex);
    QVariantList result;
    
    if (m_trajectory.size() < 2) return result;
    
    constexpr double MOON_RADIUS_KM = CoordinateSystem::MOON_TRAIL_RADIUS;
    
    // 为每对相邻点生成贴着球面的连线
    for (int i = 0; i < m_trajectory.size() - 1; ++i) {
        const QVector3D& start = m_trajectory[i].position;
        const QVector3D& end = m_trajectory[i + 1].position;
        
        // 检查两点间的角度（避免过度细分或生成大量短线）
        double dot = QVector3D::dotProduct(start.normalized(), end.normalized());
        double angleRad = static_cast<double>(qAcos(qBound(-1.0, dot, 1.0)));
        int segments = qMax(3, static_cast<int>(angleRad * 20)); // 确保曲线足够密 
        
        QVariantList linePoints = generateSlerpPoints(start, end, MOON_RADIUS_KM, segments);
        for (const auto& pt : linePoints) {
            result.append(pt);
        }
    }
    
    return result;
}

// 测试点轨迹线段（用于QML渲染）
// 使用球面插值生成密集点，QML端用大球体渲染形成连续线效果
QVariantList MoonTrajectoryModel::testLineSegments() const {
    QVariantList result;
    
    if (m_testPoints.size() < 2) return result;
    
    constexpr double MOON_RADIUS_KM = CoordinateSystem::MOON_TRAIL_RADIUS;
    
    for (int i = 0; i < m_testPoints.size() - 1; ++i) {
        auto currentMap = m_testPoints[i].value<QVariantMap>();
        auto nextMap = m_testPoints[i + 1].value<QVariantMap>();
        
        QVector3D startPos = currentMap["position"].value<QVector3D>();
        QVector3D endPos = nextMap["position"].value<QVector3D>();
        
        // 使用球面插值生成密集点（20段平衡性能和连续性）
        QVariantList slerpPoints = generateSlerpPoints(startPos, endPos, MOON_RADIUS_KM, 20);
        
        for (const auto& pt : slerpPoints) {
            result.append(pt);
        }
    }
    
    return result;
}