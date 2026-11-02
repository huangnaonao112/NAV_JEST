#include "TrajectoryEngine.h"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>

namespace
{

int failures = 0;

void check(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
    else
    {
        std::cout << "[PASS] " << message << '\n';
    }
}

bool near(double lhs, double rhs, double tolerance)
{
    return std::abs(lhs - rhs) <= tolerance;
}

bool finiteState(const NavJest::TrajectoryState &state)
{
    return std::isfinite(state.latitudeDeg)
        && std::isfinite(state.longitudeDeg)
        && std::isfinite(state.altitudeMeters)
        && std::isfinite(state.groundSpeedMps)
        && std::isfinite(state.verticalSpeedMps)
        && std::isfinite(state.headingDeg)
        && std::isfinite(state.pitchDeg)
        && std::isfinite(state.rollDeg)
        && std::isfinite(state.accelerationXEcefMps2)
        && std::isfinite(state.bodyRollRateDegPerSecond)
        && std::isfinite(state.bodyPitchRateDegPerSecond)
        && std::isfinite(state.bodyYawRateDegPerSecond);
}

} // namespace

int main()
{
    NavJest::TrajectoryEngine engine;
    engine.loadDemoRoute(39.9042, 116.4074, 50.0);

    check(engine.isValid(), "demo trajectory is valid");
    check(engine.waypoints().size() == 7,
          "demo trajectory contains seven waypoints");
    check(engine.routeLengthMeters() > 500000.0,
          "demo route length exceeds 500 km");
    check(engine.routeDurationSeconds() > 3600.0,
          "demo route duration exceeds one hour");

    const double geometryInterval = std::max(
        1.0, engine.routeDurationSeconds() / 5999.0);
    const std::vector<NavJest::TrackPoint> routeGeometry = engine.sampleTrack(
        engine.routeDurationSeconds(), geometryInterval, 6000);
    check(routeGeometry.size() > engine.waypoints().size(),
          "full route geometry contains interpolated samples");
    check(!routeGeometry.empty()
              && near(routeGeometry.front().latitudeDeg,
                      engine.waypoints().front().latitudeDeg, 1.0e-6)
              && near(routeGeometry.front().longitudeDeg,
                      engine.waypoints().front().longitudeDeg, 1.0e-6),
          "full route geometry starts at the first waypoint");
    check(!routeGeometry.empty()
              && near(routeGeometry.back().latitudeDeg,
                      engine.waypoints().back().latitudeDeg, 1.0e-6)
              && near(routeGeometry.back().longitudeDeg,
                      engine.waypoints().back().longitudeDeg, 1.0e-6),
          "full route geometry ends at the last waypoint");

    const NavJest::TrajectoryState initial = engine.stateAt(0.0);
    check(near(initial.latitudeDeg, 39.9042, 1.0e-8),
          "initial latitude matches first waypoint");
    check(near(initial.longitudeDeg, 116.4074, 1.0e-8),
          "initial longitude matches first waypoint");
    check(near(initial.altitudeMeters, 2550.0, 1.0e-6),
          "initial altitude matches first waypoint");
    check(near(initial.groundSpeedMps, 85.0, 1.0e-6),
          "initial horizontal speed is initialized to 85 m/s");
    check(initial.totalSpeedMps >= initial.groundSpeedMps,
          "total speed includes vertical velocity");
    check(initial.headingDeg >= 0.0 && initial.headingDeg < 360.0,
          "initial heading is normalized");

    const NavJest::TrajectoryState afterTenMinutes =
        engine.stateAt(600.0);
    check(!near(afterTenMinutes.latitudeDeg,
                initial.latitudeDeg, 1.0e-5)
              || !near(afterTenMinutes.longitudeDeg,
                      initial.longitudeDeg, 1.0e-5),
          "vehicle advances along the waypoint route");
    check(afterTenMinutes.distanceMeters > 40000.0,
          "distance travelled increases with simulation time");
    check(afterTenMinutes.routeProgress > initial.routeProgress,
          "route progress increases with simulation time");
    check(afterTenMinutes.remainingDistanceMeters
              < initial.remainingDistanceMeters,
          "remaining route distance decreases");
    check(afterTenMinutes.activeWaypointIndex >= 1,
          "active waypoint index is populated");
    check(finiteState(afterTenMinutes),
          "six-degree-of-freedom state contains finite values");

    const double ecefVelocityMagnitude = std::sqrt(
        afterTenMinutes.velocityXEcefMps
            * afterTenMinutes.velocityXEcefMps
        + afterTenMinutes.velocityYEcefMps
            * afterTenMinutes.velocityYEcefMps
        + afterTenMinutes.velocityZEcefMps
            * afterTenMinutes.velocityZEcefMps);
    check(near(ecefVelocityMagnitude,
               afterTenMinutes.totalSpeedMps, 1.0e-5),
          "ECEF velocity magnitude matches total 3D speed");

    const double ecefRadius = std::sqrt(
        afterTenMinutes.ecefXmeters * afterTenMinutes.ecefXmeters
        + afterTenMinutes.ecefYmeters * afterTenMinutes.ecefYmeters
        + afterTenMinutes.ecefZmeters * afterTenMinutes.ecefZmeters);
    check(ecefRadius > 6.3e6 && ecefRadius < 6.5e6,
          "ECEF position is near the WGS84 Earth surface");

    const std::vector<NavJest::TrackPoint> history =
        engine.sampleTrack(600.0, 10.0, 100);
    check(history.size() == 61,
          "track history samples the route at ten-second intervals");
    check(near(history.back().simulationSeconds, 600.0, 1.0e-6),
          "track history includes the current simulation time");

    const double roundedRouteLength = engine.routeLengthMeters();
    engine.setTurnRadiusMeters(0.0);
    const double sharpRouteLength = engine.routeLengthMeters();
    check(sharpRouteLength > roundedRouteLength,
          "turn-radius setting changes corner-cutting path length");

    engine.setTurnRadiusMeters(2500.0);
    check(engine.isValid(), "route rebuild succeeds after turn-radius update");
    check(engine.roundedTurnCount() > 0,
          "route geometry contains finite-radius circular turns");
    check(near(engine.minimumEffectiveTurnRadiusMeters(), 2500.0, 0.1),
          "long demo legs preserve the requested turn radius");
    check(engine.achievedMaximumLateralAccelerationMps2()
              <= engine.maximumLateralAccelerationMps2() + 0.05,
          "curved route speed respects lateral-acceleration limit");

    const double unconstrainedDuration = engine.routeDurationSeconds();
    engine.setMotionConstraints(3.0, 4.0, 1.5);
    check(engine.isValid(), "route rebuild succeeds after motion constraints change");
    check(engine.routeDurationSeconds() > unconstrainedDuration,
          "lower lateral acceleration limit reduces curve speed and increases duration");
    check(engine.achievedMaximumLateralAccelerationMps2() <= 1.55,
          "achieved lateral acceleration stays below configured limit");
    check(engine.achievedMaximumAccelerationMps2() <= 4.05,
          "longitudinal acceleration and braking stay within configured limits");
    engine.setMotionConstraints(3.0, 4.0, 3.0);

    const NavJest::TrajectoryState routeEnd =
        engine.stateAt(engine.routeDurationSeconds() + 10.0);
    check(near(routeEnd.groundSpeedMps, 0.0, 1.0e-9),
          "vehicle stops at the final waypoint after route completion");
    check(near(routeEnd.routeProgress, 1.0, 1.0e-9),
          "route progress reaches 100 percent at route completion");

    std::vector<NavJest::GeoWaypoint> invalidRoute(1);
    std::string errorMessage;
    check(!engine.setWaypoints(invalidRoute, &errorMessage),
          "route validation rejects fewer than two waypoints");
    check(!errorMessage.empty(),
          "route validation returns a useful error message");

    std::vector<NavJest::GeoWaypoint> customRoute;
    customRoute.push_back({"Start", 31.2304, 121.4737, 100.0, 60.0});
    customRoute.push_back({"Finish", 31.3304, 121.5737, 800.0, 75.0});
    check(engine.setWaypoints(customRoute, &errorMessage),
          "custom waypoint route is accepted");
    check(engine.waypoints().size() == 2,
          "custom route replaces the demo route");
    check(near(engine.stateAt(0.0).groundSpeedMps, 60.0, 1.0e-6),
          "custom route applies the first waypoint speed");

    std::vector<NavJest::GeoWaypoint> tightTurnRoute;
    tightTurnRoute.push_back({"A", 0.0, 0.0, 100.0, 100.0});
    tightTurnRoute.push_back({"B", 0.0, 0.009, 100.0, 100.0});
    tightTurnRoute.push_back({"C", 0.009, 0.009, 100.0, 100.0});
    check(engine.setWaypoints(tightTurnRoute, &errorMessage),
          "short right-angle route is accepted");
    engine.setTurnRadiusMeters(500.0);
    check(engine.roundedTurnCount() == 1,
          "right-angle waypoint is converted to a circular turn");
    check(engine.radiusLimitedTurnCount() == 1,
          "turn radius is reported as limited when adjacent legs are too short");
    check(engine.minimumEffectiveTurnRadiusMeters() < 500.0,
          "effective turn radius reports geometric fit limitation");
    check(engine.isValid(),
          "radius-limited route remains a valid trajectory");

    if (failures != 0)
    {
        std::cerr << failures << " trajectory test(s) failed.\n";
        return 1;
    }

    std::cout << "All trajectory engine tests passed.\n";
    return 0;
}
