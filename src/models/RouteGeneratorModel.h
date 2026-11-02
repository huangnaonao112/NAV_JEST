#pragma once

#include <QObject>
#include <QtQml/qqml.h>

/**
 * @brief 航路生成桥接模型：包装 navcore 的 TrajectoryEngine（切线圆弧转弯
 * + 运动约束速度剖面），向 QML 提供航路点→平滑轨迹的生成能力。
 *
 * generateDemoRoute / generateRoute 输入航路点数组，输出采样轨迹点
 * （经纬高+航向+时间）与统计信息（总长/时长/圆弧转弯数/受限转弯数）。
 * 生成结果保存在 lastGeneratedPoints 属性，供后续消费方取用。
 */
class RouteGeneratorModel : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(QVariantList lastGeneratedPoints READ lastGeneratedPoints NOTIFY routeChanged)
    Q_PROPERTY(QVariantMap lastStats READ lastStats NOTIFY routeChanged)

public:
    explicit RouteGeneratorModel(QObject *parent = nullptr);

    /**
     * @brief 生成演示航路（北京周边 7 航路点环形航线）。
     * @param originLatitudeDeg 起点纬度
     * @param originLongitudeDeg 起点经度
     * @param originAltitudeMeters 起点高度
     * @param turnRadiusMeters 目标转弯半径
     */
    Q_INVOKABLE QVariantMap generateDemoRoute(double originLatitudeDeg,
                                              double originLongitudeDeg,
                                              double originAltitudeMeters,
                                              double turnRadiusMeters);

    /**
     * @brief 按航路点数组生成航路。
     * @param waypoints [{name, lat, lon, alt, speed}, ...]，至少 2 个
     */
    Q_INVOKABLE QVariantMap generateRoute(const QVariantList &waypoints,
                                          double turnRadiusMeters,
                                          double maxAccelerationMps2,
                                          double maxDecelerationMps2,
                                          double maxLateralAccelerationMps2);

    QVariantList lastGeneratedPoints() const;
    QVariantMap lastStats() const;

signals:
    void routeChanged();

private:
    QVariantMap sampleEngine(double originLatitudeDeg, double originLongitudeDeg);
    QVariantList m_lastGeneratedPoints;
    QVariantMap m_lastStats;
};
