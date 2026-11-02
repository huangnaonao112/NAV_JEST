#include "TrajectoryEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace NavJest
{
namespace
{

constexpr double Pi = 3.14159265358979323846;
constexpr double DegreesToRadians = Pi / 180.0;
constexpr double RadiansToDegrees = 180.0 / Pi;
constexpr double EarthMeanRadiusMeters = 6371008.8;
constexpr double Wgs84SemiMajorMeters = 6378137.0;
constexpr double Wgs84EccentricitySquared = 6.69437999014e-3;
constexpr double GravityMetersPerSecondSquared = 9.80665;
constexpr int CurveSampleCount = 32;
constexpr double MinimumWaypointSeparationMeters = 1.0;

} // namespace

TrajectoryEngine::TrajectoryEngine()
{
    loadDemoRoute(39.9042, 116.4074, 50.0);
}

bool TrajectoryEngine::setWaypoints(
    const std::vector<GeoWaypoint> &waypoints,
    std::string *errorMessage)
{
    const auto fail = [errorMessage](const std::string &message)
    {
        if (errorMessage)
        {
            *errorMessage = message;
        }

        return false;
    };

    if (waypoints.size() < 2)
    {
        return fail("A trajectory requires at least two waypoints.");
    }

    if (waypoints.size() > 64)
    {
        return fail("A trajectory can contain at most 64 waypoints.");
    }

    for (std::size_t index = 0; index < waypoints.size(); ++index)
    {
        const GeoWaypoint &waypoint = waypoints[index];

        if (!std::isfinite(waypoint.latitudeDeg)
            || !std::isfinite(waypoint.longitudeDeg)
            || !std::isfinite(waypoint.altitudeMeters)
            || !std::isfinite(waypoint.speedMps))
        {
            return fail("Waypoint values must be finite numbers.");
        }

        if (waypoint.latitudeDeg < -90.0
            || waypoint.latitudeDeg > 90.0
            || waypoint.longitudeDeg < -180.0
            || waypoint.longitudeDeg > 180.0)
        {
            return fail("Waypoint latitude or longitude is out of range.");
        }

        if (waypoint.altitudeMeters < -1000.0
            || waypoint.altitudeMeters > 100000.0)
        {
            return fail("Waypoint altitude must be between -1000 and 100000 m.");
        }

        if (waypoint.speedMps < 1.0 || waypoint.speedMps > 1500.0)
        {
            return fail("Waypoint speed must be between 1 and 1500 m/s.");
        }

        if (index > 0)
        {
            const Vector2 previous = toLocalEnu(
                waypoints.front().latitudeDeg,
                waypoints.front().longitudeDeg,
                waypoints[index - 1].latitudeDeg,
                waypoints[index - 1].longitudeDeg);
            const Vector2 current = toLocalEnu(
                waypoints.front().latitudeDeg,
                waypoints.front().longitudeDeg,
                waypoint.latitudeDeg,
                waypoint.longitudeDeg);

            if (distance(previous, current) < MinimumWaypointSeparationMeters)
            {
                return fail("Adjacent waypoints must be at least 1 m apart.");
            }
        }
    }

    m_waypoints = waypoints;

    for (std::size_t index = 0; index < m_waypoints.size(); ++index)
    {
        if (m_waypoints[index].name.empty())
        {
            std::ostringstream stream;
            stream << "WP" << (index + 1);
            m_waypoints[index].name = stream.str();
        }
    }

    m_originLatitudeDeg = m_waypoints.front().latitudeDeg;
    m_originLongitudeDeg = m_waypoints.front().longitudeDeg;
    rebuildPath();

    if (errorMessage)
    {
        errorMessage->clear();
    }

    return m_valid;
}

void TrajectoryEngine::loadDemoRoute(double originLatitudeDeg,
                                     double originLongitudeDeg,
                                     double originAltitudeMeters)
{
    std::vector<GeoWaypoint> route;
    route.reserve(7);

    const auto appendWaypoint =
        [&route, originLatitudeDeg, originLongitudeDeg, originAltitudeMeters](
            const char *name,
            double distanceMeters,
            double bearingDeg,
            double altitudeOffsetMeters,
            double speedMps)
    {
        GeoWaypoint waypoint;
        waypoint.name = name;
        waypoint.altitudeMeters = originAltitudeMeters + altitudeOffsetMeters;
        waypoint.speedMps = speedMps;

        if (distanceMeters <= 0.0)
        {
            waypoint.latitudeDeg = originLatitudeDeg;
            waypoint.longitudeDeg = originLongitudeDeg;
        }
        else
        {
            destinationPoint(originLatitudeDeg,
                             originLongitudeDeg,
                             bearingDeg,
                             distanceMeters,
                             &waypoint.latitudeDeg,
                             &waypoint.longitudeDeg);
        }

        route.push_back(waypoint);
    };

    appendWaypoint("WP01", 0.0, 65.0, 2500.0, 85.0);
    appendWaypoint("WP02", 65000.0, 65.0, 3200.0, 95.0);
    appendWaypoint("WP03", 150000.0, 105.0, 4200.0, 110.0);
    appendWaypoint("WP04", 245000.0, 155.0, 3600.0, 100.0);
    appendWaypoint("WP05", 345000.0, 220.0, 2800.0, 90.0);
    appendWaypoint("WP06", 455000.0, 280.0, 3400.0, 105.0);
    appendWaypoint("WP07", 560000.0, 330.0, 3000.0, 85.0);

    setWaypoints(route, nullptr);
}

void TrajectoryEngine::setTurnRadiusMeters(double radiusMeters)
{
    if (!std::isfinite(radiusMeters))
    {
        return;
    }

    const double boundedRadius = std::clamp(radiusMeters, 0.0, 50000.0);

    if (std::abs(boundedRadius - m_turnRadiusMeters) < 1.0e-9)
    {
        return;
    }

    m_turnRadiusMeters = boundedRadius;

    if (m_waypoints.size() >= 2)
    {
        rebuildPath();
    }
}

void TrajectoryEngine::setMotionConstraints(
    double maximumAccelerationMps2,
    double maximumDecelerationMps2,
    double maximumLateralAccelerationMps2)
{
    if (!std::isfinite(maximumAccelerationMps2)
        || !std::isfinite(maximumDecelerationMps2)
        || !std::isfinite(maximumLateralAccelerationMps2))
    {
        return;
    }

    m_maximumAccelerationMps2 = std::clamp(
        maximumAccelerationMps2, 0.1, 20.0);
    m_maximumDecelerationMps2 = std::clamp(
        maximumDecelerationMps2, 0.1, 30.0);
    m_maximumLateralAccelerationMps2 = std::clamp(
        maximumLateralAccelerationMps2, 0.1, 20.0);

    if (m_waypoints.size() >= 2)
    {
        rebuildPath();
    }
}

TrajectoryState TrajectoryEngine::stateAt(double simulationSeconds) const
{
    TrajectoryState result;

    if (!m_valid)
    {
        return result;
    }

    const double time = std::max(0.0, simulationSeconds);
    const RawState raw = rawStateAt(time);
    const AttitudeState attitude = attitudeAt(time);
    result.simulationSeconds = time;
    fromLocalEnu(m_originLatitudeDeg,
                 m_originLongitudeDeg,
                 raw.position,
                 &result.latitudeDeg,
                 &result.longitudeDeg);
    result.altitudeMeters = raw.altitudeMeters;
    result.distanceMeters = raw.distanceMeters;
    result.remainingDistanceMeters = std::max(
        0.0, m_routeLengthMeters - raw.distanceMeters);
    result.routeProgress = m_routeLengthMeters > 0.0
        ? std::clamp(raw.distanceMeters / m_routeLengthMeters, 0.0, 1.0)
        : 0.0;
    result.groundSpeedMps = raw.groundSpeedMps;
    result.totalSpeedMps = std::sqrt(
        raw.groundSpeedMps * raw.groundSpeedMps
        + raw.verticalSpeedMps * raw.verticalSpeedMps);
    result.verticalSpeedMps = raw.verticalSpeedMps;
    result.headingDeg = attitude.headingDeg;
    result.pitchDeg = attitude.pitchDeg;
    result.rollDeg = attitude.rollDeg;
    result.yawDeg = attitude.headingDeg;
    result.tangentialAccelerationMps2 =
        raw.tangentialAccelerationMps2;
    result.yawRateDegPerSecond =
        attitude.yawRateRadPerSecond * RadiansToDegrees;
    result.activeWaypointIndex = raw.activeWaypointIndex;

    geodeticToEcef(result.latitudeDeg,
                   result.longitudeDeg,
                   result.altitudeMeters,
                   &result.ecefXmeters,
                   &result.ecefYmeters,
                   &result.ecefZmeters);

    const double latitude = result.latitudeDeg * DegreesToRadians;
    const double longitude = result.longitudeDeg * DegreesToRadians;
    const double heading = result.headingDeg * DegreesToRadians;
    const double velocityNorth = raw.groundSpeedMps * std::cos(heading);
    const double velocityEast = raw.groundSpeedMps * std::sin(heading);
    const double velocityUp = raw.verticalSpeedMps;

    result.velocityXEcefMps =
        -std::sin(longitude) * velocityEast
        - std::sin(latitude) * std::cos(longitude) * velocityNorth
        + std::cos(latitude) * std::cos(longitude) * velocityUp;
    result.velocityYEcefMps =
        std::cos(longitude) * velocityEast
        - std::sin(latitude) * std::sin(longitude) * velocityNorth
        + std::cos(latitude) * std::sin(longitude) * velocityUp;
    result.velocityZEcefMps =
        std::cos(latitude) * velocityNorth
        + std::sin(latitude) * velocityUp;

    const double yawRate = attitude.yawRateRadPerSecond;
    const double accelerationNorth =
        raw.tangentialAccelerationMps2 * std::cos(heading)
        - raw.groundSpeedMps * yawRate * std::sin(heading);
    const double accelerationEast =
        raw.tangentialAccelerationMps2 * std::sin(heading)
        + raw.groundSpeedMps * yawRate * std::cos(heading);
    const double accelerationUp = raw.verticalAccelerationMps2;

    result.accelerationXEcefMps2 =
        -std::sin(longitude) * accelerationEast
        - std::sin(latitude) * std::cos(longitude) * accelerationNorth
        + std::cos(latitude) * std::cos(longitude) * accelerationUp;
    result.accelerationYEcefMps2 =
        std::cos(longitude) * accelerationEast
        - std::sin(latitude) * std::sin(longitude) * accelerationNorth
        + std::cos(latitude) * std::sin(longitude) * accelerationUp;
    result.accelerationZEcefMps2 =
        std::cos(latitude) * accelerationNorth
        + std::sin(latitude) * accelerationUp;

    const double attitudeSampleInterval = 0.25;
    const double beforeTime = std::max(0.0, time - attitudeSampleInterval);
    const double afterTime = std::min(
        m_routeDurationSeconds, time + attitudeSampleInterval);
    const double attitudeDeltaTime = afterTime - beforeTime;

    if (attitudeDeltaTime > 1.0e-6)
    {
        const AttitudeState before = attitudeAt(beforeTime);
        const AttitudeState after = attitudeAt(afterTime);
        const double rollRate =
            wrapDegrees(after.rollDeg - before.rollDeg)
            * DegreesToRadians / attitudeDeltaTime;
        const double pitchRate =
            (after.pitchDeg - before.pitchDeg)
            * DegreesToRadians / attitudeDeltaTime;
        const double yawRateBody =
            wrapDegrees(after.headingDeg - before.headingDeg)
            * DegreesToRadians / attitudeDeltaTime;
        const double rollRadians = attitude.rollDeg * DegreesToRadians;
        const double pitchRadians = attitude.pitchDeg * DegreesToRadians;

        const double bodyP = rollRate
            - yawRateBody * std::sin(pitchRadians);
        const double bodyQ = pitchRate * std::cos(rollRadians)
            + yawRateBody * std::sin(rollRadians)
                * std::cos(pitchRadians);
        const double bodyR = -pitchRate * std::sin(rollRadians)
            + yawRateBody * std::cos(rollRadians)
                * std::cos(pitchRadians);

        result.bodyRollRateDegPerSecond = bodyP * RadiansToDegrees;
        result.bodyPitchRateDegPerSecond = bodyQ * RadiansToDegrees;
        result.bodyYawRateDegPerSecond = bodyR * RadiansToDegrees;
    }

    return result;
}

std::vector<TrackPoint> TrajectoryEngine::sampleTrack(
    double simulationSeconds,
    double intervalSeconds,
    std::size_t maximumPoints) const
{
    std::vector<TrackPoint> result;

    if (!m_valid || maximumPoints == 0)
    {
        return result;
    }

    const double endTime = std::clamp(
        simulationSeconds, 0.0, m_routeDurationSeconds);
    const double interval = std::max(0.1, intervalSeconds);
    const std::size_t totalRegularSamples =
        static_cast<std::size_t>(std::floor(endTime / interval)) + 1;
    const std::size_t firstSample = totalRegularSamples > maximumPoints
        ? totalRegularSamples - maximumPoints
        : 0;

    result.reserve(std::min(maximumPoints, totalRegularSamples + 1));

    for (std::size_t sampleIndex = firstSample;
         sampleIndex < totalRegularSamples;
         ++sampleIndex)
    {
        const double sampleTime =
            static_cast<double>(sampleIndex) * interval;
        const TrajectoryState state = stateAt(sampleTime);

        TrackPoint point;
        point.simulationSeconds = sampleTime;
        point.latitudeDeg = state.latitudeDeg;
        point.longitudeDeg = state.longitudeDeg;
        point.altitudeMeters = state.altitudeMeters;
        point.headingDeg = state.headingDeg;
        result.push_back(point);
    }

    if (result.empty()
        || std::abs(result.back().simulationSeconds - endTime) > 1.0e-6)
    {
        const TrajectoryState state = stateAt(endTime);

        TrackPoint point;
        point.simulationSeconds = endTime;
        point.latitudeDeg = state.latitudeDeg;
        point.longitudeDeg = state.longitudeDeg;
        point.altitudeMeters = state.altitudeMeters;
        point.headingDeg = state.headingDeg;
        result.push_back(point);
    }

    if (result.size() > maximumPoints)
    {
        result.erase(result.begin(),
                     result.begin()
                         + static_cast<std::ptrdiff_t>(
                             result.size() - maximumPoints));
    }

    return result;
}

const std::vector<GeoWaypoint> &TrajectoryEngine::waypoints() const
{
    return m_waypoints;
}

double TrajectoryEngine::turnRadiusMeters() const
{
    return m_turnRadiusMeters;
}

double TrajectoryEngine::routeLengthMeters() const
{
    return m_routeLengthMeters;
}

double TrajectoryEngine::routeDurationSeconds() const
{
    return m_routeDurationSeconds;
}

double TrajectoryEngine::maximumAccelerationMps2() const
{
    return m_maximumAccelerationMps2;
}

double TrajectoryEngine::maximumDecelerationMps2() const
{
    return m_maximumDecelerationMps2;
}

double TrajectoryEngine::maximumLateralAccelerationMps2() const
{
    return m_maximumLateralAccelerationMps2;
}

double TrajectoryEngine::achievedMaximumAccelerationMps2() const
{
    return m_achievedMaximumAccelerationMps2;
}

double TrajectoryEngine::achievedMaximumLateralAccelerationMps2() const
{
    return m_achievedMaximumLateralAccelerationMps2;
}

double TrajectoryEngine::minimumEffectiveTurnRadiusMeters() const
{
    return m_minimumEffectiveTurnRadiusMeters;
}

std::size_t TrajectoryEngine::roundedTurnCount() const
{
    return m_roundedTurnCount;
}

std::size_t TrajectoryEngine::radiusLimitedTurnCount() const
{
    return m_radiusLimitedTurnCount;
}

bool TrajectoryEngine::isValid() const
{
    return m_valid;
}

void TrajectoryEngine::rebuildPath()
{
    m_pathNodes.clear();
    m_routeLengthMeters = 0.0;
    m_routeDurationSeconds = 0.0;
    m_achievedMaximumAccelerationMps2 = 0.0;
    m_achievedMaximumLateralAccelerationMps2 = 0.0;
    m_minimumEffectiveTurnRadiusMeters = 0.0;
    m_roundedTurnCount = 0;
    m_radiusLimitedTurnCount = 0;
    m_valid = false;

    if (m_waypoints.size() < 2)
    {
        return;
    }

    std::vector<Vector2> localWaypoints;
    localWaypoints.reserve(m_waypoints.size());

    for (const GeoWaypoint &waypoint : m_waypoints)
    {
        localWaypoints.push_back(toLocalEnu(
            m_originLatitudeDeg,
            m_originLongitudeDeg,
            waypoint.latitudeDeg,
            waypoint.longitudeDeg));
    }

    struct TurnCorner
    {
        Vector2 incoming;
        Vector2 outgoing;
        Vector2 center;
        double effectiveRadiusMeters = 0.0;
        double startAngleRadians = 0.0;
        double sweepRadians = 0.0;
        double incomingFraction = 1.0;
        double outgoingFraction = 0.0;
        bool enabled = false;
        bool radiusLimited = false;
    };

    std::vector<TurnCorner> corners(m_waypoints.size());

    for (std::size_t index = 1;
         index + 1 < localWaypoints.size();
         ++index)
    {
        const Vector2 &previous = localWaypoints[index - 1];
        const Vector2 &current = localWaypoints[index];
        const Vector2 &next = localWaypoints[index + 1];
        const double incomingLength = distance(previous, current);
        const double outgoingLength = distance(current, next);

        if (incomingLength < MinimumWaypointSeparationMeters
            || outgoingLength < MinimumWaypointSeparationMeters
            || m_turnRadiusMeters <= 0.0)
        {
            continue;
        }

        const Vector2 incomingDirection {
            (current.east - previous.east) / incomingLength,
            (current.north - previous.north) / incomingLength
        };
        const Vector2 outgoingDirection {
            (next.east - current.east) / outgoingLength,
            (next.north - current.north) / outgoingLength
        };

        const double directionDot = std::clamp(
            dot(incomingDirection, outgoingDirection), -1.0, 1.0);
        const double turnAngle = std::acos(directionDot);

        // Straight legs need no fillet; near-180-degree reversals are not
        // safely representable by a single finite-radius fly-by turn.
        if (turnAngle < 1.0e-3 || turnAngle > Pi - 1.0e-2)
        {
            continue;
        }

        const double tangentFactor = std::tan(turnAngle * 0.5);

        if (!std::isfinite(tangentFactor) || tangentFactor <= 1.0e-9)
        {
            continue;
        }

        const double requestedTangentDistance =
            m_turnRadiusMeters * tangentFactor;
        const double maximumTangentDistance =
            0.45 * std::min(incomingLength, outgoingLength);
        const double tangentDistance = std::min(
            requestedTangentDistance, maximumTangentDistance);
        const double effectiveRadius = tangentDistance / tangentFactor;

        if (tangentDistance < 1.0 || effectiveRadius < 1.0)
        {
            continue;
        }

        const double turnDirection = cross(
            incomingDirection, outgoingDirection) >= 0.0 ? 1.0 : -1.0;
        TurnCorner &corner = corners[index];
        corner.incoming = {
            current.east - incomingDirection.east * tangentDistance,
            current.north - incomingDirection.north * tangentDistance
        };
        corner.outgoing = {
            current.east + outgoingDirection.east * tangentDistance,
            current.north + outgoingDirection.north * tangentDistance
        };
        const Vector2 normal = leftNormal(incomingDirection);
        corner.center = {
            corner.incoming.east
                + turnDirection * normal.east * effectiveRadius,
            corner.incoming.north
                + turnDirection * normal.north * effectiveRadius
        };
        const Vector2 startRadius {
            corner.incoming.east - corner.center.east,
            corner.incoming.north - corner.center.north
        };
        const Vector2 endRadius {
            corner.outgoing.east - corner.center.east,
            corner.outgoing.north - corner.center.north
        };
        corner.startAngleRadians = std::atan2(
            startRadius.north, startRadius.east);
        corner.sweepRadians = std::atan2(
            cross(startRadius, endRadius), dot(startRadius, endRadius));

        if (turnDirection > 0.0 && corner.sweepRadians < 0.0)
        {
            corner.sweepRadians += 2.0 * Pi;
        }
        else if (turnDirection < 0.0 && corner.sweepRadians > 0.0)
        {
            corner.sweepRadians -= 2.0 * Pi;
        }

        corner.effectiveRadiusMeters = effectiveRadius;
        corner.incomingFraction = std::clamp(
            tangentDistance / incomingLength, 0.0, 0.45);
        corner.outgoingFraction = std::clamp(
            tangentDistance / outgoingLength, 0.0, 0.45);
        corner.radiusLimited = effectiveRadius
            + 1.0e-6 < m_turnRadiusMeters;
        corner.enabled = std::abs(corner.sweepRadians) > 1.0e-4;

        if (corner.enabled)
        {
            ++m_roundedTurnCount;
            if (corner.radiusLimited)
            {
                ++m_radiusLimitedTurnCount;
            }
            if (m_minimumEffectiveTurnRadiusMeters <= 0.0
                || effectiveRadius < m_minimumEffectiveTurnRadiusMeters)
            {
                m_minimumEffectiveTurnRadiusMeters = effectiveRadius;
            }
        }
    }

    const auto appendNode = [this](const Vector2 &position,
                                   double altitudeMeters,
                                   double targetSpeedMps,
                                   double turnRadiusMeters,
                                   std::size_t waypointIndex)
    {
        if (m_pathNodes.empty())
        {
            PathNode first;
            first.position = position;
            first.altitudeMeters = altitudeMeters;
            first.speedMps = targetSpeedMps;
            first.targetSpeedMps = targetSpeedMps;
            first.turnRadiusMeters = turnRadiusMeters;
            first.waypointIndex = waypointIndex;
            m_pathNodes.push_back(first);
            return;
        }

        PathNode &previous = m_pathNodes.back();
        const double segmentDistance = distance(previous.position, position);

        if (segmentDistance < 1.0e-6)
        {
            previous.altitudeMeters = altitudeMeters;
            previous.targetSpeedMps = targetSpeedMps;
            previous.turnRadiusMeters = std::max(
                previous.turnRadiusMeters, turnRadiusMeters);
            previous.waypointIndex = waypointIndex;
            return;
        }

        PathNode node;
        node.position = position;
        node.altitudeMeters = altitudeMeters;
        node.speedMps = targetSpeedMps;
        node.targetSpeedMps = targetSpeedMps;
        node.turnRadiusMeters = turnRadiusMeters;
        node.cumulativeDistanceMeters =
            previous.cumulativeDistanceMeters + segmentDistance;
        node.waypointIndex = waypointIndex;
        m_pathNodes.push_back(node);
    };

    appendNode(localWaypoints.front(),
               m_waypoints.front().altitudeMeters,
               m_waypoints.front().speedMps,
               0.0,
               0);

    for (std::size_t legIndex = 0;
         legIndex + 1 < localWaypoints.size();
         ++legIndex)
    {
        const std::size_t destinationIndex = legIndex + 1;
        const bool hasTurn = destinationIndex + 1 < localWaypoints.size()
            && corners[destinationIndex].enabled;
        const Vector2 lineEnd = hasTurn
            ? corners[destinationIndex].incoming
            : localWaypoints[destinationIndex];
        const double endFraction = hasTurn
            ? 1.0 - corners[destinationIndex].incomingFraction
            : 1.0;
        const double endAltitude = m_waypoints[legIndex].altitudeMeters
            + (m_waypoints[destinationIndex].altitudeMeters
               - m_waypoints[legIndex].altitudeMeters) * endFraction;
        const double endSpeed = m_waypoints[legIndex].speedMps
            + (m_waypoints[destinationIndex].speedMps
               - m_waypoints[legIndex].speedMps) * endFraction;

        appendNode(lineEnd,
                   endAltitude,
                   endSpeed,
                   hasTurn ? corners[destinationIndex].effectiveRadiusMeters : 0.0,
                   destinationIndex);

        if (!hasTurn)
        {
            continue;
        }

        const TurnCorner &corner = corners[destinationIndex];
        const double radius = corner.effectiveRadiusMeters;
        const double absoluteSweep = std::abs(corner.sweepRadians);
        const double maximumAngularStep = radius > 0.0
            ? 2.0 * std::acos(std::clamp(1.0 - 2.0 / radius, -1.0, 1.0))
            : absoluteSweep;
        const int sampleCount = std::clamp(
            static_cast<int>(std::ceil(absoluteSweep
                / std::max(0.01, maximumAngularStep))),
            8,
            128);

        for (int sample = 1; sample <= sampleCount; ++sample)
        {
            const double fraction = static_cast<double>(sample)
                / static_cast<double>(sampleCount);
            const Vector2 arcPoint = circularArcPoint(
                corner.center,
                radius,
                corner.startAngleRadians,
                corner.sweepRadians,
                fraction);

            appendNode(arcPoint,
                       m_waypoints[destinationIndex].altitudeMeters,
                       m_waypoints[destinationIndex].speedMps,
                       radius,
                       destinationIndex);
        }
    }

    if (m_pathNodes.size() < 2)
    {
        return;
    }

    // Convert waypoint speeds into a physically bounded speed profile.
    // Curved samples are additionally limited by v <= sqrt(a_lat * R).
    for (PathNode &node : m_pathNodes)
    {
        node.speedMps = std::clamp(node.targetSpeedMps, 1.0, 1500.0);
        if (node.turnRadiusMeters > 0.0)
        {
            const double curveSpeedLimit = std::sqrt(
                m_maximumLateralAccelerationMps2
                * node.turnRadiusMeters);
            node.speedMps = std::min(node.speedMps, curveSpeedLimit);
        }
    }

    // The vehicle is brought to rest at the final waypoint. Forward and
    // backward passes enforce acceleration and braking reachability.
    m_pathNodes.back().speedMps = 0.0;

    for (std::size_t index = 1; index < m_pathNodes.size(); ++index)
    {
        const PathNode &previous = m_pathNodes[index - 1];
        PathNode &current = m_pathNodes[index];
        const double segmentDistance = current.cumulativeDistanceMeters
            - previous.cumulativeDistanceMeters;
        const double reachableSpeed = std::sqrt(
            previous.speedMps * previous.speedMps
            + 2.0 * m_maximumAccelerationMps2 * segmentDistance);
        current.speedMps = std::min(current.speedMps, reachableSpeed);
    }

    for (std::size_t index = m_pathNodes.size() - 1; index > 0; --index)
    {
        const PathNode &next = m_pathNodes[index];
        PathNode &previous = m_pathNodes[index - 1];
        const double segmentDistance = next.cumulativeDistanceMeters
            - previous.cumulativeDistanceMeters;
        const double reachableSpeed = std::sqrt(
            next.speedMps * next.speedMps
            + 2.0 * m_maximumDecelerationMps2 * segmentDistance);
        previous.speedMps = std::min(previous.speedMps, reachableSpeed);
    }

    m_pathNodes.front().cumulativeTimeSeconds = 0.0;
    m_achievedMaximumAccelerationMps2 = 0.0;
    m_achievedMaximumLateralAccelerationMps2 = 0.0;

    for (std::size_t index = 1; index < m_pathNodes.size(); ++index)
    {
        PathNode &previous = m_pathNodes[index - 1];
        PathNode &current = m_pathNodes[index];
        const double segmentDistance = current.cumulativeDistanceMeters
            - previous.cumulativeDistanceMeters;
        const double speedSum = previous.speedMps + current.speedMps;

        if (speedSum <= 1.0e-9)
        {
            return;
        }

        const double duration = 2.0 * segmentDistance / speedSum;
        const double tangentialAcceleration =
            (current.speedMps * current.speedMps
             - previous.speedMps * previous.speedMps)
            / (2.0 * segmentDistance);

        current.cumulativeTimeSeconds =
            previous.cumulativeTimeSeconds + duration;
        m_achievedMaximumAccelerationMps2 = std::max(
            m_achievedMaximumAccelerationMps2,
            std::abs(tangentialAcceleration));

        if (current.turnRadiusMeters > 0.0)
        {
            const double lateralAcceleration = current.speedMps
                * current.speedMps / current.turnRadiusMeters;
            m_achievedMaximumLateralAccelerationMps2 = std::max(
                m_achievedMaximumLateralAccelerationMps2,
                lateralAcceleration);
        }
    }

    m_routeLengthMeters = m_pathNodes.back().cumulativeDistanceMeters;
    m_routeDurationSeconds = m_pathNodes.back().cumulativeTimeSeconds;
    m_valid = m_routeLengthMeters > 0.0
        && m_routeDurationSeconds > 0.0
        && std::isfinite(m_routeDurationSeconds);
}

TrajectoryEngine::RawState TrajectoryEngine::rawStateAt(
    double simulationSeconds) const
{
    RawState result;

    if (m_pathNodes.empty())
    {
        return result;
    }

    const double time = std::max(0.0, simulationSeconds);

    if (time >= m_routeDurationSeconds)
    {
        const PathNode &last = m_pathNodes.back();
        result.position = last.position;
        result.altitudeMeters = last.altitudeMeters;
        result.distanceMeters = last.cumulativeDistanceMeters;
        result.activeWaypointIndex = m_waypoints.size() - 1;
        return result;
    }

    const auto upper = std::upper_bound(
        m_pathNodes.begin(),
        m_pathNodes.end(),
        time,
        [](double value, const PathNode &node)
        {
            return value < node.cumulativeTimeSeconds;
        });

    const std::size_t upperIndex = static_cast<std::size_t>(
        std::distance(m_pathNodes.begin(), upper));
    const std::size_t lowerIndex = upperIndex == 0 ? 0 : upperIndex - 1;
    const PathNode &start = m_pathNodes[lowerIndex];
    const PathNode &end = m_pathNodes[upperIndex];

    const double duration = end.cumulativeTimeSeconds
        - start.cumulativeTimeSeconds;

    if (duration <= 1.0e-9)
    {
        result.position = end.position;
        result.altitudeMeters = end.altitudeMeters;
        result.distanceMeters = end.cumulativeDistanceMeters;
        result.activeWaypointIndex = end.waypointIndex;
        return result;
    }

    const double elapsed = std::clamp(
        time - start.cumulativeTimeSeconds, 0.0, duration);
    const double startSpeed = std::max(0.0, start.speedMps);
    const double endSpeed = std::max(0.0, end.speedMps);
    const double segmentDistance = end.cumulativeDistanceMeters
        - start.cumulativeDistanceMeters;
    const double acceleration = segmentDistance > 0.0
        ? (endSpeed * endSpeed - startSpeed * startSpeed)
            / (2.0 * segmentDistance)
        : 0.0;
    const double travelled = std::clamp(
        startSpeed * elapsed + 0.5 * acceleration * elapsed * elapsed,
        0.0,
        segmentDistance);
    const double fraction = segmentDistance > 0.0
        ? travelled / segmentDistance
        : 0.0;

    result.position = interpolate(
        start.position, end.position, fraction);
    result.altitudeMeters = start.altitudeMeters
        + (end.altitudeMeters - start.altitudeMeters) * fraction;
    result.groundSpeedMps = std::clamp(
        startSpeed + acceleration * elapsed,
        0.0,
        std::max(startSpeed, endSpeed));
    result.verticalSpeedMps = segmentDistance > 0.0
        ? (end.altitudeMeters - start.altitudeMeters)
            / segmentDistance * result.groundSpeedMps
        : 0.0;
    result.tangentialAccelerationMps2 = acceleration;
    result.verticalAccelerationMps2 = segmentDistance > 0.0
        ? (end.altitudeMeters - start.altitudeMeters)
            / segmentDistance * acceleration
        : 0.0;
    result.distanceMeters =
        start.cumulativeDistanceMeters + travelled;
    result.activeWaypointIndex = end.waypointIndex;

    return result;
}

TrajectoryEngine::Vector2 TrajectoryEngine::positionAtDistance(
    double distanceMeters) const
{
    if (m_pathNodes.empty())
    {
        return {};
    }

    const double distanceValue = std::clamp(
        distanceMeters, 0.0, m_routeLengthMeters);

    const auto upper = std::upper_bound(
        m_pathNodes.begin(),
        m_pathNodes.end(),
        distanceValue,
        [](double value, const PathNode &node)
        {
            return value < node.cumulativeDistanceMeters;
        });

    if (upper == m_pathNodes.begin())
    {
        return m_pathNodes.front().position;
    }

    if (upper == m_pathNodes.end())
    {
        return m_pathNodes.back().position;
    }

    const PathNode &end = *upper;
    const PathNode &start = *(upper - 1);
    const double segmentDistance = end.cumulativeDistanceMeters
        - start.cumulativeDistanceMeters;
    const double fraction = segmentDistance > 0.0
        ? (distanceValue - start.cumulativeDistanceMeters)
            / segmentDistance
        : 0.0;

    return interpolate(start.position, end.position, fraction);
}

double TrajectoryEngine::headingAtDistance(double distanceMeters) const
{
    const double lookAheadMeters = 10.0;
    const Vector2 before = positionAtDistance(
        std::max(0.0, distanceMeters - lookAheadMeters));
    const Vector2 after = positionAtDistance(
        std::min(m_routeLengthMeters,
                 distanceMeters + lookAheadMeters));
    const double eastDelta = after.east - before.east;
    const double northDelta = after.north - before.north;

    if (std::hypot(eastDelta, northDelta) < 1.0e-9)
    {
        return 0.0;
    }

    return normalizeDegrees(
        std::atan2(eastDelta, northDelta) * RadiansToDegrees);
}

double TrajectoryEngine::headingAtTime(double simulationSeconds) const
{
    const RawState raw = rawStateAt(simulationSeconds);
    return headingAtDistance(raw.distanceMeters);
}

TrajectoryEngine::AttitudeState TrajectoryEngine::attitudeAt(
    double simulationSeconds) const
{
    AttitudeState result;

    if (!m_valid)
    {
        return result;
    }

    const double time = std::clamp(
        simulationSeconds, 0.0, m_routeDurationSeconds);
    const RawState raw = rawStateAt(time);
    result.headingDeg = headingAtDistance(raw.distanceMeters);
    result.pitchDeg = std::atan2(
        raw.verticalSpeedMps, std::max(0.01, raw.groundSpeedMps))
        * RadiansToDegrees;

    const double sampleInterval = 0.25;
    const double beforeTime = std::max(0.0, time - sampleInterval);
    const double afterTime = std::min(
        m_routeDurationSeconds, time + sampleInterval);
    const double elapsed = afterTime - beforeTime;

    if (elapsed > 1.0e-6)
    {
        const double headingBefore = headingAtTime(beforeTime);
        const double headingAfter = headingAtTime(afterTime);
        const double headingDifference =
            wrapDegrees(headingAfter - headingBefore);
        result.yawRateRadPerSecond =
            headingDifference * DegreesToRadians / elapsed;
    }

    const double bankRadians = std::atan2(
        raw.groundSpeedMps * result.yawRateRadPerSecond,
        GravityMetersPerSecondSquared);
    result.rollDeg = std::clamp(
        bankRadians * RadiansToDegrees, -60.0, 60.0);

    return result;
}

double TrajectoryEngine::normalizeDegrees(double degrees)
{
    double result = std::fmod(degrees, 360.0);

    if (result < 0.0)
    {
        result += 360.0;
    }

    return result;
}

double TrajectoryEngine::wrapDegrees(double degrees)
{
    double result = std::remainder(degrees, 360.0);

    if (result > 180.0)
    {
        result -= 360.0;
    }

    if (result < -180.0)
    {
        result += 360.0;
    }

    return result;
}

double TrajectoryEngine::distance(const Vector2 &a, const Vector2 &b)
{
    return std::hypot(b.east - a.east, b.north - a.north);
}

TrajectoryEngine::Vector2 TrajectoryEngine::interpolate(
    const Vector2 &a,
    const Vector2 &b,
    double fraction)
{
    return {
        a.east + (b.east - a.east) * fraction,
        a.north + (b.north - a.north) * fraction
    };
}

TrajectoryEngine::Vector2 TrajectoryEngine::leftNormal(
    const Vector2 &direction)
{
    return {-direction.north, direction.east};
}

double TrajectoryEngine::dot(const Vector2 &a, const Vector2 &b)
{
    return a.east * b.east + a.north * b.north;
}

double TrajectoryEngine::cross(const Vector2 &a, const Vector2 &b)
{
    return a.east * b.north - a.north * b.east;
}

TrajectoryEngine::Vector2 TrajectoryEngine::circularArcPoint(
    const Vector2 &center,
    double radiusMeters,
    double startAngleRadians,
    double sweepRadians,
    double fraction)
{
    const double angle = startAngleRadians + sweepRadians * fraction;
    return {
        center.east + radiusMeters * std::cos(angle),
        center.north + radiusMeters * std::sin(angle)
    };
}

TrajectoryEngine::Vector2 TrajectoryEngine::toLocalEnu(
    double originLatitudeDeg,
    double originLongitudeDeg,
    double latitudeDeg,
    double longitudeDeg)
{
    const double latitude1 = originLatitudeDeg * DegreesToRadians;
    const double latitude2 = latitudeDeg * DegreesToRadians;
    const double longitudeDifference =
        (longitudeDeg - originLongitudeDeg) * DegreesToRadians;
    const double latitudeDifference = latitude2 - latitude1;
    const double haversine = std::sin(latitudeDifference * 0.5)
            * std::sin(latitudeDifference * 0.5)
        + std::cos(latitude1) * std::cos(latitude2)
            * std::sin(longitudeDifference * 0.5)
            * std::sin(longitudeDifference * 0.5);
    const double angularDistance = 2.0 * std::atan2(
        std::sqrt(std::max(0.0, haversine)),
        std::sqrt(std::max(0.0, 1.0 - haversine)));
    const double bearing = std::atan2(
        std::sin(longitudeDifference) * std::cos(latitude2),
        std::cos(latitude1) * std::sin(latitude2)
            - std::sin(latitude1) * std::cos(latitude2)
                * std::cos(longitudeDifference));
    const double distanceMeters = EarthMeanRadiusMeters * angularDistance;

    return {
        distanceMeters * std::sin(bearing),
        distanceMeters * std::cos(bearing)
    };
}

void TrajectoryEngine::fromLocalEnu(double originLatitudeDeg,
                                    double originLongitudeDeg,
                                    const Vector2 &position,
                                    double *latitudeDeg,
                                    double *longitudeDeg)
{
    const double distanceMeters =
        std::hypot(position.east, position.north);
    const double bearingDeg = std::atan2(
        position.east, position.north) * RadiansToDegrees;

    destinationPoint(originLatitudeDeg,
                     originLongitudeDeg,
                     bearingDeg,
                     distanceMeters,
                     latitudeDeg,
                     longitudeDeg);
}

void TrajectoryEngine::destinationPoint(double originLatitudeDeg,
                                        double originLongitudeDeg,
                                        double bearingDeg,
                                        double distanceMeters,
                                        double *latitudeDeg,
                                        double *longitudeDeg)
{
    const double latitude1 = originLatitudeDeg * DegreesToRadians;
    const double longitude1 = originLongitudeDeg * DegreesToRadians;
    const double bearing = bearingDeg * DegreesToRadians;
    const double angularDistance = distanceMeters / EarthMeanRadiusMeters;

    const double latitude2 = std::asin(
        std::sin(latitude1) * std::cos(angularDistance)
        + std::cos(latitude1) * std::sin(angularDistance)
            * std::cos(bearing));
    const double longitude2 = longitude1 + std::atan2(
        std::sin(bearing) * std::sin(angularDistance)
            * std::cos(latitude1),
        std::cos(angularDistance)
            - std::sin(latitude1) * std::sin(latitude2));

    if (latitudeDeg)
    {
        *latitudeDeg = latitude2 * RadiansToDegrees;
    }

    if (longitudeDeg)
    {
        double longitude = std::fmod(
            longitude2 * RadiansToDegrees + 180.0, 360.0);

        if (longitude < 0.0)
        {
            longitude += 360.0;
        }

        *longitudeDeg = longitude - 180.0;
    }
}

void TrajectoryEngine::geodeticToEcef(double latitudeDeg,
                                      double longitudeDeg,
                                      double altitudeMeters,
                                      double *xMeters,
                                      double *yMeters,
                                      double *zMeters)
{
    const double latitude = latitudeDeg * DegreesToRadians;
    const double longitude = longitudeDeg * DegreesToRadians;
    const double sinLatitude = std::sin(latitude);
    const double cosLatitude = std::cos(latitude);
    const double primeVerticalRadius = Wgs84SemiMajorMeters
        / std::sqrt(1.0 - Wgs84EccentricitySquared
                    * sinLatitude * sinLatitude);

    if (xMeters)
    {
        *xMeters = (primeVerticalRadius + altitudeMeters)
            * cosLatitude * std::cos(longitude);
    }

    if (yMeters)
    {
        *yMeters = (primeVerticalRadius + altitudeMeters)
            * cosLatitude * std::sin(longitude);
    }

    if (zMeters)
    {
        *zMeters = (primeVerticalRadius
                    * (1.0 - Wgs84EccentricitySquared)
                    + altitudeMeters) * sinLatitude;
    }
}

} // namespace NavJest
