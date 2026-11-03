#include "OrbitalMechanics.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

int failures = 0;

void expectNear(const std::string &name,
                double actual,
                double expected,
                double tolerance)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::cerr << "[FAIL] " << name
                  << ": actual=" << actual
                  << ", expected=" << expected
                  << ", tolerance=" << tolerance << '\n';
        ++failures;
        return;
    }

    std::cout << "[PASS] " << name << '\n';
}

void expectTrue(const std::string &name, bool condition)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << name << '\n';
        ++failures;
        return;
    }

    std::cout << "[PASS] " << name << '\n';
}

} // namespace

int main()
{
    using namespace OrbitMath;

    OrbitalElements circularOrbit;
    circularOrbit.semiMajorAxisKm = 7000.0;
    circularOrbit.eccentricity = 0.0;
    circularOrbit.inclinationDeg = 0.0;
    circularOrbit.raanDeg = 0.0;
    circularOrbit.argumentOfPerigeeDeg = 0.0;
    circularOrbit.trueAnomalyDeg = 0.0;

    const StateVector circularState = stateFromElements(
        circularOrbit, circularOrbit.trueAnomalyDeg);

    expectNear("circular orbit radius", norm(circularState.positionKm),
               7000.0, 1.0e-8);
    expectNear("circular orbit speed",
               norm(circularState.velocityKmPerSecond),
               std::sqrt(EarthMuKm3PerSecond2 / 7000.0), 1.0e-10);

    expectTrue("valid circular orbit accepted",
               isValidEllipticOrbit(circularOrbit));

    OrbitalElements invalidEccentricityOrbit = circularOrbit;
    invalidEccentricityOrbit.eccentricity = 1.0;

    expectTrue("parabolic orbit rejected by elliptic propagator",
               !isValidEllipticOrbit(invalidEccentricityOrbit));

    OrbitalElements lowPerigeeOrbit = circularOrbit;
    lowPerigeeOrbit.semiMajorAxisKm = EarthEquatorialRadiusKm + 40.0;

    expectTrue("orbit below minimum perigee rejected",
               !isValidEllipticOrbit(lowPerigeeOrbit, 80.0));

    const double circularPeriod = orbitalPeriodSeconds(circularOrbit);
    const StateVector afterOnePeriod = propagateKepler(
        circularOrbit, circularPeriod);

    expectNear("period propagation position",
               norm({
                   afterOnePeriod.positionKm.x - circularState.positionKm.x,
                   afterOnePeriod.positionKm.y - circularState.positionKm.y,
                   afterOnePeriod.positionKm.z - circularState.positionKm.z
               }),
               0.0, 1.0e-6);

    expectNear("period propagation velocity",
               norm({
                   afterOnePeriod.velocityKmPerSecond.x
                       - circularState.velocityKmPerSecond.x,
                   afterOnePeriod.velocityKmPerSecond.y
                       - circularState.velocityKmPerSecond.y,
                   afterOnePeriod.velocityKmPerSecond.z
                       - circularState.velocityKmPerSecond.z
               }),
               0.0, 1.0e-9);

    const double eccentricity = 0.31;
    const double meanAnomaly = 2.17;
    const double eccentricAnomaly = solveEccentricAnomaly(
        meanAnomaly, eccentricity);
    const double keplerResidual = eccentricAnomaly
        - eccentricity * std::sin(eccentricAnomaly)
        - normalizeRadians(meanAnomaly);

    expectNear("Kepler equation residual", keplerResidual, 0.0, 1.0e-12);

    OrbitalElements gnssOrbit;
    gnssOrbit.semiMajorAxisKm = 26560.0;
    gnssOrbit.eccentricity = 0.01;
    gnssOrbit.inclinationDeg = 55.0;
    gnssOrbit.raanDeg = 40.0;
    gnssOrbit.argumentOfPerigeeDeg = 30.0;
    gnssOrbit.trueAnomalyDeg = 72.0;

    const double gnssPeriodHours =
        orbitalPeriodSeconds(gnssOrbit) / 3600.0;

    expectNear("GNSS MEO orbital period (hours)",
               gnssPeriodHours, 11.97, 0.03);

    const StateVector gnssState = stateFromElements(
        gnssOrbit, gnssOrbit.trueAnomalyDeg);
    OrbitalElements recoveredElements;
    const bool recovered = elementsFromState(gnssState, recoveredElements);

    expectTrue("state-to-elements conversion", recovered);

    if (recovered)
    {
        expectNear("semi-major axis round trip",
                   recoveredElements.semiMajorAxisKm,
                   gnssOrbit.semiMajorAxisKm, 1.0e-7);
        expectNear("eccentricity round trip",
                   recoveredElements.eccentricity,
                   gnssOrbit.eccentricity, 1.0e-10);
        expectNear("inclination round trip",
                   recoveredElements.inclinationDeg,
                   gnssOrbit.inclinationDeg, 1.0e-8);
    }

    const StateVector afterGnssPeriod = propagateKepler(
        gnssOrbit, orbitalPeriodSeconds(gnssOrbit));

    expectNear("elliptical orbit period closure",
               norm({
                   afterGnssPeriod.positionKm.x - gnssState.positionKm.x,
                   afterGnssPeriod.positionKm.y - gnssState.positionKm.y,
                   afterGnssPeriod.positionKm.z - gnssState.positionKm.z
               }),
               0.0, 1.0e-5);

    const Vector3 ecef = eciToEcef(gnssState.positionKm, 123456.0);
    const Vector3 eciRoundTrip = ecefToEci(ecef, 123456.0);

    expectNear("ECI/ECEF round trip",
               norm({
                   eciRoundTrip.x - gnssState.positionKm.x,
                   eciRoundTrip.y - gnssState.positionKm.y,
                   eciRoundTrip.z - gnssState.positionKm.z
               }),
               0.0, 1.0e-8);

    const GeodeticPosition beijing {
        39.9042,
        116.4074,
        50.0
    };
    const Vector3 beijingEcef = geodeticToEcef(beijing);
    const GeodeticPosition beijingRoundTrip = ecefToGeodetic(beijingEcef);

    expectNear("WGS84 latitude round trip",
               beijingRoundTrip.latitudeDeg,
               beijing.latitudeDeg, 1.0e-8);
    expectNear("WGS84 longitude round trip",
               beijingRoundTrip.longitudeDeg,
               beijing.longitudeDeg, 1.0e-8);
    expectNear("WGS84 altitude round trip",
               beijingRoundTrip.altitudeMeters,
               beijing.altitudeMeters, 1.0e-3);

    OrbitalElements geostationaryOrbit;
    geostationaryOrbit.semiMajorAxisKm = 42164.0;
    geostationaryOrbit.eccentricity = 0.0001;
    geostationaryOrbit.inclinationDeg = 0.0;

    expectNear("GEO orbital period (hours)",
               orbitalPeriodSeconds(geostationaryOrbit) / 3600.0,
               23.93, 0.03);

    const GeodeticPosition equatorialStation {0.0, 0.0, 0.0};
    const Vector3 stationEcef = geodeticToEcef(equatorialStation);
    const Vector3 overheadSatellite {
        stationEcef.x + 20000.0,
        stationEcef.y,
        stationEcef.z
    };
    const TopocentricObservation overheadObservation =
        observeFromGroundStation(overheadSatellite, equatorialStation);

    expectNear("overhead satellite elevation",
               overheadObservation.elevationDeg, 90.0, 1.0e-8);

    StateVector maneuverState = circularState;
    maneuverState.velocityKmPerSecond.y += 0.01;

    OrbitalElements maneuverElements;
    const bool maneuverOrbitValid = elementsFromState(
        maneuverState, maneuverElements);

    expectTrue("RTN impulse creates a valid bound orbit",
               maneuverOrbitValid);
    expectTrue("along-track impulse changes eccentricity",
               maneuverOrbitValid && maneuverElements.eccentricity > 1.0e-6);

    // ===== 跨语言金标：与 scripts/orb_common.py::sub_ecef 逐位对照 =====
    // 锁定"生成器 ↔ C++ 渲染"坐标约定（惯性推进 + 绕 Z 转 -Ωe·tow）。
    // 金标值由 orb_common.sub_ecef 生成（三幕演示两颗欺骗源、三个快照 tow）。
    // 修改任一侧坐标约定前，先读 docs/轨道参数报文接口.md §10.1。
    struct GoldenCase
    {
        const char *name;
        double aKm;
        double ecc;
        double incDeg;
        double raanDeg;
        double argpDeg;
        double m0Deg;
        double tow;
        double xKm;
        double yKm;
        double zKm;
    };
    const GoldenCase goldenCases[] = {
        { "DSP1@tow0", 27906.051, 0.0008347, 54.6016, 140.0414, 38.6571, 353.2896, 0.0,
          -23624.583745960, 8648.284714576, 12022.616659326 },
        { "DSP1@tow4500", 27906.051, 0.0008347, 54.6016, 140.0414, 38.6571, 353.2896, 4500.0,
          -18376.037007141, 1643.907422458, 20909.843023327 },
        { "DSP1@tow9000", 27906.051, 0.0008347, 54.6016, 140.0414, 38.6571, 353.2896, 9000.0,
          -14206.227480536, -9008.928723469, 22252.714850527 },
        { "DSP2@tow0", 42171.804, 0.003982, 54.2839, 324.4706, -162.9201, 292.7637, 0.0,
          -10726.306503813, 30992.022690392, 26409.728860728 },
        { "DSP2@tow4500", 42171.804, 0.003982, 54.2839, 324.4706, -162.9201, 292.7637, 4500.0,
          -10383.658529589, 36595.375785004, 17946.224440871 },
        { "DSP2@tow9000", 42171.804, 0.003982, 54.2839, 324.4706, -162.9201, 292.7637, 9000.0,
          -6694.190846555, 40795.939176520, 7554.167944749 },
    };

    for (const GoldenCase &gc : goldenCases)
    {
        OrbitalElements el;
        el.semiMajorAxisKm = gc.aKm;
        el.eccentricity = gc.ecc;
        el.inclinationDeg = gc.incDeg;
        el.raanDeg = gc.raanDeg;
        el.argumentOfPerigeeDeg = gc.argpDeg;
        el.trueAnomalyDeg = meanToTrueAnomalyDeg(gc.m0Deg, gc.ecc);

        // 与 orb_common.sub_ecef 同构：M0 + n·tow 惯性推进 + 绕 Z 转 -Ωe·tow
        const StateVector sv = propagateKepler(el, gc.tow);
        const double spinRad = -EarthRotationRateRadPerSecond * gc.tow;
        const Vector3 ecef = rotateZ(sv.positionKm, spinRad);

        expectNear((std::string(gc.name) + " x").c_str(),
                   ecef.x, gc.xKm, 1.0e-6);
        expectNear((std::string(gc.name) + " y").c_str(),
                   ecef.y, gc.yKm, 1.0e-6);
        expectNear((std::string(gc.name) + " z").c_str(),
                   ecef.z, gc.zKm, 1.0e-6);
    }

    if (failures > 0)
    {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All orbital mechanics tests passed.\n";
    return EXIT_SUCCESS;
}
