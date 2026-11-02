#include "RouteGeneratorModel.h"

#include "TrajectoryEngine.h"

#include <QString>

namespace
{

// 把引擎采样结果打包为 QML 友好的返回结构
QVariantMap buildResult(bool valid, const QString &message,
                        QVariantList points,
                        const NavJest::TrajectoryEngine &engine)
{
    QVariantMap result;
    result.insert(QStringLiteral("valid"), valid);
    result.insert(QStringLiteral("message"), message);
    result.insert(QStringLiteral("points"), points);
    result.insert(QStringLiteral("lengthMeters"), engine.routeLengthMeters());
    result.insert(QStringLiteral("durationSeconds"), engine.routeDurationSeconds());
    result.insert(QStringLiteral("roundedTurnCount"),
                  static_cast<int>(engine.roundedTurnCount()));
    result.insert(QStringLiteral("radiusLimitedTurnCount"),
                  static_cast<int>(engine.radiusLimitedTurnCount()));
    result.insert(QStringLiteral("minimumEffectiveTurnRadiusMeters"),
                  engine.minimumEffectiveTurnRadiusMeters());
    result.insert(QStringLiteral("achievedAccelerationMps2"),
                  engine.achievedMaximumAccelerationMps2());
    result.insert(QStringLiteral("achievedLateralAccelerationMps2"),
                  engine.achievedMaximumLateralAccelerationMps2());
    return result;
}

} // namespace

RouteGeneratorModel::RouteGeneratorModel(QObject *parent)
    : QObject(parent)
{
}

QVariantMap RouteGeneratorModel::generateDemoRoute(double originLatitudeDeg,
                                                   double originLongitudeDeg,
                                                   double originAltitudeMeters,
                                                   double turnRadiusMeters)
{
    NavJest::TrajectoryEngine engine;
    engine.loadDemoRoute(originLatitudeDeg, originLongitudeDeg, originAltitudeMeters);
    engine.setTurnRadiusMeters(turnRadiusMeters);

    if (!engine.isValid())
        return buildResult(false, QStringLiteral("演示航路构建失败"), {}, engine);

    const std::vector<NavJest::TrackPoint> track =
        engine.sampleTrack(engine.routeDurationSeconds(), 10.0, 720);

    QVariantList points;
    points.reserve(static_cast<qsizetype>(track.size()));
    for (const NavJest::TrackPoint &p : track)
    {
        QVariantMap point;
        point.insert(QStringLiteral("timeSeconds"), p.simulationSeconds);
        point.insert(QStringLiteral("lat"), p.latitudeDeg);
        point.insert(QStringLiteral("lon"), p.longitudeDeg);
        point.insert(QStringLiteral("alt"), p.altitudeMeters);
        point.insert(QStringLiteral("headingDeg"), p.headingDeg);
        points.append(point);
    }

    m_lastGeneratedPoints = points;
    m_lastStats = buildResult(true, QStringLiteral("演示航路生成完成"), {}, engine);
    m_lastStats.insert(QStringLiteral("pointCount"), static_cast<int>(track.size()));
    emit routeChanged();

    QVariantMap result = m_lastStats;
    result.insert(QStringLiteral("points"), points);
    return result;
}

QVariantMap RouteGeneratorModel::generateRoute(const QVariantList &waypoints,
                                               double turnRadiusMeters,
                                               double maxAccelerationMps2,
                                               double maxDecelerationMps2,
                                               double maxLateralAccelerationMps2)
{
    if (waypoints.size() < 2)
        return buildResult(false, QStringLiteral("至少需要 2 个航路点"), {}, NavJest::TrajectoryEngine());

    std::vector<NavJest::GeoWaypoint> route;
    route.reserve(static_cast<std::size_t>(waypoints.size()));
    for (const QVariant &item : waypoints)
    {
        const QVariantMap w = item.toMap();
        NavJest::GeoWaypoint waypoint;
        waypoint.name = w.value(QStringLiteral("name")).toString().toStdString();
        waypoint.latitudeDeg = w.value(QStringLiteral("lat")).toDouble();
        waypoint.longitudeDeg = w.value(QStringLiteral("lon")).toDouble();
        waypoint.altitudeMeters = w.value(QStringLiteral("alt")).toDouble();
        waypoint.speedMps = w.value(QStringLiteral("speed")).toDouble();
        route.push_back(waypoint);
    }

    NavJest::TrajectoryEngine engine;
    std::string errorMessage;
    if (!engine.setWaypoints(route, &errorMessage))
        return buildResult(false, QString::fromStdString(errorMessage), {}, engine);

    engine.setTurnRadiusMeters(turnRadiusMeters);
    engine.setMotionConstraints(maxAccelerationMps2, maxDecelerationMps2,
                                maxLateralAccelerationMps2);

    if (!engine.isValid())
        return buildResult(false, QStringLiteral("航路约束下构建失败"), {}, engine);

    const std::vector<NavJest::TrackPoint> track =
        engine.sampleTrack(engine.routeDurationSeconds(), 10.0, 720);

    QVariantList points;
    points.reserve(static_cast<qsizetype>(track.size()));
    for (const NavJest::TrackPoint &p : track)
    {
        QVariantMap point;
        point.insert(QStringLiteral("timeSeconds"), p.simulationSeconds);
        point.insert(QStringLiteral("lat"), p.latitudeDeg);
        point.insert(QStringLiteral("lon"), p.longitudeDeg);
        point.insert(QStringLiteral("alt"), p.altitudeMeters);
        point.insert(QStringLiteral("headingDeg"), p.headingDeg);
        points.append(point);
    }

    m_lastGeneratedPoints = points;
    m_lastStats = buildResult(true, QStringLiteral("航路生成完成"), {}, engine);
    m_lastStats.insert(QStringLiteral("pointCount"), static_cast<int>(track.size()));
    emit routeChanged();

    QVariantMap result = m_lastStats;
    result.insert(QStringLiteral("points"), points);
    return result;
}

QVariantList RouteGeneratorModel::lastGeneratedPoints() const
{
    return m_lastGeneratedPoints;
}

QVariantMap RouteGeneratorModel::lastStats() const
{
    return m_lastStats;
}
