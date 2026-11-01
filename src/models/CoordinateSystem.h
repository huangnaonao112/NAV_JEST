#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QVector3D>
/**
 * @brief 坐标系统 - 真实大小 + 压缩距离
 * 
 * 天体大小：真实值（场景单位 = km）
 * - 地球半径：6371 km
 * - 月球半径：1737 km
 * 
 * 距离：压缩显示
 * - 真实地月距离：384400 km
 * - 显示距离：50000 km
 * 
 * 轨迹：根据距离分段处理
 * - 近地轨道（< 100000 km）：保持真实坐标
 * - 地月转移（> 100000 km）：压缩坐标
 */
class CoordinateSystem : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(double earthRadius READ earthRadius CONSTANT)
    Q_PROPERTY(double moonRadius READ moonRadius CONSTANT)
    Q_PROPERTY(double moonDistance READ moonDistance CONSTANT)
public:
    static CoordinateSystem* instance();
    // 真实物理参数 (km)
    static constexpr double EARTH_RADIUS = 6371.0;
    static constexpr double MOON_RADIUS = 1737.4;
    static constexpr double MOON_DISTANCE_REAL = 384400.0;
    
    // 显示距离（压缩）- 与SceneViewItem.qml一致
    static constexpr double MOON_DISTANCE_DISPLAY = 80000.0;
    
    // 场景缩放比例
    static constexpr double DISTANCE_SCALE = MOON_DISTANCE_DISPLAY / MOON_DISTANCE_REAL;  // ~0.208
    
    // 过渡距离：在此距离内保持真实，之外压缩
    static constexpr double TRANSITION_DISTANCE = 100000.0;  // km
    // 天体半径：真实值
    double earthRadius() const { return EARTH_RADIUS; }
    double moonRadius() const { return MOON_RADIUS; }
    
    // 地月距离：压缩后的显示值
    double moonDistance() const { return MOON_DISTANCE_DISPLAY; }
    
    // 坐标转换：真实(km) -> 场景(显示)
    // 统一使用压缩比例，确保轨迹连续
    QVector3D toScene(const QVector3D& realKm) const {
        // 统一压缩所有坐标
        return realKm * DISTANCE_SCALE;
    }
    
    QVector<QVector3D> toSceneList(const QVector<QVector3D>& realPoints) const {
        QVector<QVector3D> result;
        result.reserve(realPoints.size());
        for (const auto& p : realPoints) {
            result.append(toScene(p));
        }
        return result;
    }
private:
    CoordinateSystem(QObject* parent = nullptr) : QObject(parent) {}
};