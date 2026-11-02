#ifndef NAVJEST_TRAJECTORY_ENGINE_H
#define NAVJEST_TRAJECTORY_ENGINE_H

#include <cstddef>
#include <string>
#include <vector>

namespace NavJest
{

struct GeoWaypoint
{
    std::string name;
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeMeters = 0.0;
    double speedMps = 85.0;
};

struct TrajectoryState
{
    double simulationSeconds = 0.0;
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeMeters = 0.0;

    double distanceMeters = 0.0;
    double remainingDistanceMeters = 0.0;
    double routeProgress = 0.0;
    double groundSpeedMps = 0.0;
    double totalSpeedMps = 0.0;
    double verticalSpeedMps = 0.0;
    double headingDeg = 0.0;
    double pitchDeg = 0.0;
    double rollDeg = 0.0;
    double yawDeg = 0.0;

    double ecefXmeters = 0.0;
    double ecefYmeters = 0.0;
    double ecefZmeters = 0.0;
    double velocityXEcefMps = 0.0;
    double velocityYEcefMps = 0.0;
    double velocityZEcefMps = 0.0;
    double accelerationXEcefMps2 = 0.0;
    double accelerationYEcefMps2 = 0.0;
    double accelerationZEcefMps2 = 0.0;

    double bodyRollRateDegPerSecond = 0.0;
    double bodyPitchRateDegPerSecond = 0.0;
    double bodyYawRateDegPerSecond = 0.0;

    double tangentialAccelerationMps2 = 0.0;
    double yawRateDegPerSecond = 0.0;
    std::size_t activeWaypointIndex = 0;
};

struct TrackPoint
{
    double simulationSeconds = 0.0;
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeMeters = 0.0;
    double headingDeg = 0.0;
};

class TrajectoryEngine
{
public:
    TrajectoryEngine();

    bool setWaypoints(const std::vector<GeoWaypoint> &waypoints,
                      std::string *errorMessage = nullptr);
    void loadDemoRoute(double originLatitudeDeg,
                       double originLongitudeDeg,
                       double originAltitudeMeters);

    void setTurnRadiusMeters(double radiusMeters);

    TrajectoryState stateAt(double simulationSeconds) const;
    std::vector<TrackPoint> sampleTrack(double simulationSeconds,
                                         double intervalSeconds = 10.0,
                                         std::size_t maximumPoints = 720) const;

    const std::vector<GeoWaypoint> &waypoints() const;
    double turnRadiusMeters() const;
    double routeLengthMeters() const;
    double routeDurationSeconds() const;
    double maximumAccelerationMps2() const;
    double maximumDecelerationMps2() const;
    double maximumLateralAccelerationMps2() const;
    double achievedMaximumAccelerationMps2() const;
    double achievedMaximumLateralAccelerationMps2() const;
    double minimumEffectiveTurnRadiusMeters() const;
    std::size_t roundedTurnCount() const;
    std::size_t radiusLimitedTurnCount() const;
    void setMotionConstraints(double maximumAccelerationMps2,
                              double maximumDecelerationMps2,
                              double maximumLateralAccelerationMps2);
    bool isValid() const;

private:
    struct Vector2
    {
        double east = 0.0;
        double north = 0.0;
    };

    struct PathNode
    {
        Vector2 position;
        double altitudeMeters = 0.0;
        double speedMps = 0.0;
        double targetSpeedMps = 0.0;
        double turnRadiusMeters = 0.0;
        double cumulativeDistanceMeters = 0.0;
        double cumulativeTimeSeconds = 0.0;
        std::size_t waypointIndex = 0;
    };

    struct RawState
    {
        Vector2 position;
        double altitudeMeters = 0.0;
        double groundSpeedMps = 0.0;
        double verticalSpeedMps = 0.0;
        double tangentialAccelerationMps2 = 0.0;
        double verticalAccelerationMps2 = 0.0;
        double distanceMeters = 0.0;
        std::size_t activeWaypointIndex = 0;
    };

    struct AttitudeState
    {
        double headingDeg = 0.0;
        double pitchDeg = 0.0;
        double rollDeg = 0.0;
        double yawRateRadPerSecond = 0.0;
    };

    void rebuildPath();
    RawState rawStateAt(double simulationSeconds) const;
    Vector2 positionAtDistance(double distanceMeters) const;
    double headingAtDistance(double distanceMeters) const;
    double headingAtTime(double simulationSeconds) const;
    AttitudeState attitudeAt(double simulationSeconds) const;

    static double normalizeDegrees(double degrees);
    static double wrapDegrees(double degrees);
    static double distance(const Vector2 &a, const Vector2 &b);
    static Vector2 interpolate(const Vector2 &a,
                               const Vector2 &b,
                               double fraction);
    static Vector2 leftNormal(const Vector2 &direction);
    static double dot(const Vector2 &a, const Vector2 &b);
    static double cross(const Vector2 &a, const Vector2 &b);
    static Vector2 circularArcPoint(const Vector2 &center,
                                    double radiusMeters,
                                    double startAngleRadians,
                                    double sweepRadians,
                                    double fraction);
    static Vector2 toLocalEnu(double originLatitudeDeg,
                              double originLongitudeDeg,
                              double latitudeDeg,
                              double longitudeDeg);
    static void fromLocalEnu(double originLatitudeDeg,
                             double originLongitudeDeg,
                             const Vector2 &position,
                             double *latitudeDeg,
                             double *longitudeDeg);
    static void destinationPoint(double originLatitudeDeg,
                                 double originLongitudeDeg,
                                 double bearingDeg,
                                 double distanceMeters,
                                 double *latitudeDeg,
                                 double *longitudeDeg);
    static void geodeticToEcef(double latitudeDeg,
                               double longitudeDeg,
                               double altitudeMeters,
                               double *xMeters,
                               double *yMeters,
                               double *zMeters);

    std::vector<GeoWaypoint> m_waypoints;
    std::vector<PathNode> m_pathNodes;
    double m_turnRadiusMeters = 2500.0;
    double m_maximumAccelerationMps2 = 3.0;
    double m_maximumDecelerationMps2 = 4.0;
    double m_maximumLateralAccelerationMps2 = 3.0;
    double m_achievedMaximumAccelerationMps2 = 0.0;
    double m_achievedMaximumLateralAccelerationMps2 = 0.0;
    double m_minimumEffectiveTurnRadiusMeters = 0.0;
    std::size_t m_roundedTurnCount = 0;
    std::size_t m_radiusLimitedTurnCount = 0;
    double m_routeLengthMeters = 0.0;
    double m_routeDurationSeconds = 0.0;
    double m_originLatitudeDeg = 0.0;
    double m_originLongitudeDeg = 0.0;
    bool m_valid = false;
};

} // namespace NavJest

#endif // NAVJEST_TRAJECTORY_ENGINE_H
