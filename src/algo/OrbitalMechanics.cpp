#include "OrbitalMechanics.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace OrbitMath
{

namespace
{

constexpr double TwoPi = 2.0 * Pi;
constexpr double EccentricityEpsilon = 1.0e-9;
constexpr double VectorEpsilon = 1.0e-12;

double clampUnit(double value)
{
    return std::max(-1.0, std::min(1.0, value));
}

Vector3 rotatePerifocalToEci(const Vector3 &perifocal,
                             double raan,
                             double inclination,
                             double argumentOfPerigee)
{
    const double cosRaan = std::cos(raan);
    const double sinRaan = std::sin(raan);
    const double cosInclination = std::cos(inclination);
    const double sinInclination = std::sin(inclination);
    const double cosArgument = std::cos(argumentOfPerigee);
    const double sinArgument = std::sin(argumentOfPerigee);

    const double m11 = cosRaan * cosArgument
        - sinRaan * sinArgument * cosInclination;
    const double m12 = -cosRaan * sinArgument
        - sinRaan * cosArgument * cosInclination;
    const double m21 = sinRaan * cosArgument
        + cosRaan * sinArgument * cosInclination;
    const double m22 = -sinRaan * sinArgument
        + cosRaan * cosArgument * cosInclination;
    const double m31 = sinArgument * sinInclination;
    const double m32 = cosArgument * sinInclination;

    return {
        m11 * perifocal.x + m12 * perifocal.y,
        m21 * perifocal.x + m22 * perifocal.y,
        m31 * perifocal.x + m32 * perifocal.y
    };
}

} // namespace

double toRadians(double degrees)
{
    return degrees * Pi / 180.0;
}

double toDegrees(double radians)
{
    return radians * 180.0 / Pi;
}

double normalizeRadians(double radians)
{
    double result = std::fmod(radians, TwoPi);

    if (result < 0.0)
    {
        result += TwoPi;
    }

    return result;
}

double normalizeDegrees(double degrees)
{
    double result = std::fmod(degrees, 360.0);

    if (result < 0.0)
    {
        result += 360.0;
    }

    return result;
}

double dot(const Vector3 &a, const Vector3 &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 cross(const Vector3 &a, const Vector3 &b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

double norm(const Vector3 &value)
{
    return std::sqrt(dot(value, value));
}

Vector3 normalized(const Vector3 &value)
{
    const double length = norm(value);

    if (length <= VectorEpsilon)
    {
        return {};
    }

    return {
        value.x / length,
        value.y / length,
        value.z / length
    };
}

bool isValidEllipticOrbit(const OrbitalElements &elements,
                          double minimumPerigeeAltitudeKm)
{
    if (!std::isfinite(elements.semiMajorAxisKm)
        || !std::isfinite(elements.eccentricity)
        || !std::isfinite(elements.inclinationDeg)
        || !std::isfinite(elements.raanDeg)
        || !std::isfinite(elements.argumentOfPerigeeDeg)
        || !std::isfinite(elements.trueAnomalyDeg))
    {
        return false;
    }

    if (elements.semiMajorAxisKm <= 0.0
        || elements.eccentricity < 0.0
        || elements.eccentricity >= 1.0
        || elements.inclinationDeg < 0.0
        || elements.inclinationDeg > 180.0)
    {
        return false;
    }

    const double perigeeRadius = elements.semiMajorAxisKm
        * (1.0 - elements.eccentricity);

    return perigeeRadius >= EarthEquatorialRadiusKm + minimumPerigeeAltitudeKm;
}

double solveEccentricAnomaly(double meanAnomalyRad,
                             double eccentricity,
                             double tolerance,
                             int maximumIterations)
{
    if (eccentricity < 0.0 || eccentricity >= 1.0)
    {
        throw std::invalid_argument("Eccentricity must be in [0, 1).");
    }

    const double meanAnomaly = normalizeRadians(meanAnomalyRad);

    if (eccentricity < EccentricityEpsilon)
    {
        return meanAnomaly;
    }

    double eccentricAnomaly = eccentricity < 0.8
        ? meanAnomaly
        : Pi;

    for (int iteration = 0; iteration < maximumIterations; ++iteration)
    {
        const double functionValue = eccentricAnomaly
            - eccentricity * std::sin(eccentricAnomaly)
            - meanAnomaly;
        const double derivative = 1.0
            - eccentricity * std::cos(eccentricAnomaly);
        const double correction = functionValue / derivative;

        eccentricAnomaly -= correction;

        if (std::abs(correction) <= tolerance)
        {
            return normalizeRadians(eccentricAnomaly);
        }
    }

    return normalizeRadians(eccentricAnomaly);
}

double meanToTrueAnomalyDeg(double meanAnomalyDeg, double eccentricity)
{
    const double meanAnomaly = toRadians(meanAnomalyDeg);
    const double eccentricAnomaly = solveEccentricAnomaly(meanAnomaly,
                                                          eccentricity);
    const double trueAnomaly = std::atan2(
        std::sqrt(1.0 - eccentricity * eccentricity)
            * std::sin(eccentricAnomaly),
        std::cos(eccentricAnomaly) - eccentricity);
    return toDegrees(trueAnomaly);
}

Vector3 rotateZ(const Vector3 &value, double angleRad)
{
    const double cosine = std::cos(angleRad);
    const double sine = std::sin(angleRad);
    return { cosine * value.x - sine * value.y,
             sine * value.x + cosine * value.y,
             value.z };
}

StateVector stateFromElements(const OrbitalElements &elements,
                              double trueAnomalyDeg,
                              double gravitationalParameter)
{
    if (!isValidEllipticOrbit(elements, 0.0)
        || !std::isfinite(trueAnomalyDeg)
        || !std::isfinite(gravitationalParameter)
        || gravitationalParameter <= 0.0)
    {
        throw std::invalid_argument("Invalid orbital elements or gravitational parameter.");
    }

    const double eccentricity = elements.eccentricity;
    const double trueAnomaly = toRadians(trueAnomalyDeg);
    const double inclination = toRadians(elements.inclinationDeg);
    const double raan = toRadians(elements.raanDeg);
    const double argumentOfPerigee = toRadians(elements.argumentOfPerigeeDeg);
    const double semiLatusRectum = elements.semiMajorAxisKm
        * (1.0 - eccentricity * eccentricity);
    const double radius = semiLatusRectum
        / (1.0 + eccentricity * std::cos(trueAnomaly));

    const Vector3 positionPerifocal = {
        radius * std::cos(trueAnomaly),
        radius * std::sin(trueAnomaly),
        0.0
    };

    const double velocityScale = std::sqrt(
        gravitationalParameter / semiLatusRectum);
    const Vector3 velocityPerifocal = {
        -velocityScale * std::sin(trueAnomaly),
        velocityScale * (eccentricity + std::cos(trueAnomaly)),
        0.0
    };

    StateVector state;
    state.positionKm = rotatePerifocalToEci(
        positionPerifocal, raan, inclination, argumentOfPerigee);
    state.velocityKmPerSecond = rotatePerifocalToEci(
        velocityPerifocal, raan, inclination, argumentOfPerigee);

    return state;
}

StateVector propagateKepler(const OrbitalElements &elements,
                            double elapsedSeconds,
                            double gravitationalParameter)
{
    if (!isValidEllipticOrbit(elements, 0.0)
        || !std::isfinite(gravitationalParameter)
        || gravitationalParameter <= 0.0
        || !std::isfinite(elapsedSeconds))
    {
        throw std::invalid_argument("Invalid input for Kepler propagation.");
    }

    const double eccentricity = elements.eccentricity;
    const double initialTrueAnomaly = toRadians(elements.trueAnomalyDeg);
    const double initialEccentricAnomaly = 2.0 * std::atan2(
        std::sqrt(1.0 - eccentricity) * std::sin(initialTrueAnomaly / 2.0),
        std::sqrt(1.0 + eccentricity) * std::cos(initialTrueAnomaly / 2.0));
    const double initialMeanAnomaly = initialEccentricAnomaly
        - eccentricity * std::sin(initialEccentricAnomaly);
    const double meanMotion = std::sqrt(
        gravitationalParameter
        / std::pow(elements.semiMajorAxisKm, 3.0));
    const double meanAnomaly = initialMeanAnomaly + meanMotion * elapsedSeconds;
    const double eccentricAnomaly = solveEccentricAnomaly(
        meanAnomaly, eccentricity);
    const double trueAnomaly = 2.0 * std::atan2(
        std::sqrt(1.0 + eccentricity) * std::sin(eccentricAnomaly / 2.0),
        std::sqrt(1.0 - eccentricity) * std::cos(eccentricAnomaly / 2.0));

    return stateFromElements(elements, toDegrees(trueAnomaly), gravitationalParameter);
}

bool elementsFromState(const StateVector &state,
                       OrbitalElements &elements,
                       double gravitationalParameter)
{
    if (gravitationalParameter <= 0.0)
    {
        return false;
    }

    const Vector3 &position = state.positionKm;
    const Vector3 &velocity = state.velocityKmPerSecond;
    const double radius = norm(position);
    const double speedSquared = dot(velocity, velocity);

    if (radius <= VectorEpsilon || !std::isfinite(radius)
        || !std::isfinite(speedSquared))
    {
        return false;
    }

    const Vector3 angularMomentum = cross(position, velocity);
    const double angularMomentumMagnitude = norm(angularMomentum);

    if (angularMomentumMagnitude <= VectorEpsilon)
    {
        return false;
    }

    const Vector3 nodeVector = cross({0.0, 0.0, 1.0}, angularMomentum);
    const double nodeMagnitude = norm(nodeVector);
    const Vector3 eccentricityVector = {
        cross(velocity, angularMomentum).x / gravitationalParameter
            - position.x / radius,
        cross(velocity, angularMomentum).y / gravitationalParameter
            - position.y / radius,
        cross(velocity, angularMomentum).z / gravitationalParameter
            - position.z / radius
    };
    const double eccentricity = norm(eccentricityVector);
    const double specificEnergy = speedSquared / 2.0
        - gravitationalParameter / radius;

    if (specificEnergy >= 0.0 || eccentricity >= 1.0)
    {
        return false;
    }

    const double semiMajorAxis = -gravitationalParameter
        / (2.0 * specificEnergy);
    const double inclination = std::acos(clampUnit(
        angularMomentum.z / angularMomentumMagnitude));

    double raan = 0.0;

    if (nodeMagnitude > VectorEpsilon)
    {
        raan = std::atan2(nodeVector.y, nodeVector.x);
    }

    double argumentOfPerigee = 0.0;
    double trueAnomaly = 0.0;

    if (eccentricity > EccentricityEpsilon)
    {
        if (nodeMagnitude > VectorEpsilon)
        {
            const double cosineArgument = dot(nodeVector, eccentricityVector)
                / (nodeMagnitude * eccentricity);
            const double sineArgument = dot(
                cross(nodeVector, eccentricityVector), angularMomentum)
                / (nodeMagnitude * eccentricity * angularMomentumMagnitude);
            argumentOfPerigee = std::atan2(sineArgument, cosineArgument);
        }
        else
        {
            argumentOfPerigee = std::atan2(
                eccentricityVector.y, eccentricityVector.x);
        }

        const double cosineAnomaly = dot(eccentricityVector, position)
            / (eccentricity * radius);
        const double sineAnomaly = dot(
            cross(eccentricityVector, position), angularMomentum)
            / (eccentricity * radius * angularMomentumMagnitude);
        trueAnomaly = std::atan2(sineAnomaly, cosineAnomaly);
    }
    else if (nodeMagnitude > VectorEpsilon)
    {
        const double cosineAnomaly = dot(nodeVector, position)
            / (nodeMagnitude * radius);
        const double sineAnomaly = dot(
            cross(nodeVector, position), angularMomentum)
            / (nodeMagnitude * radius * angularMomentumMagnitude);
        trueAnomaly = std::atan2(sineAnomaly, cosineAnomaly);
    }
    else
    {
        trueAnomaly = std::atan2(position.y, position.x);
    }

    elements.semiMajorAxisKm = semiMajorAxis;
    elements.eccentricity = eccentricity;
    elements.inclinationDeg = normalizeDegrees(toDegrees(inclination));
    elements.raanDeg = normalizeDegrees(toDegrees(raan));
    elements.argumentOfPerigeeDeg = normalizeDegrees(
        toDegrees(argumentOfPerigee));
    elements.trueAnomalyDeg = normalizeDegrees(toDegrees(trueAnomaly));

    return isValidEllipticOrbit(elements, 0.0);
}

double greenwichMeanSiderealAngle(double secondsSinceJ2000)
{
    const double julianDate = 2451545.0 + secondsSinceJ2000 / 86400.0;
    const double centuries = (julianDate - 2451545.0) / 36525.0;
    const double degrees = 280.46061837
        + 360.98564736629 * (julianDate - 2451545.0)
        + 0.000387933 * centuries * centuries
        - centuries * centuries * centuries / 38710000.0;

    return toRadians(normalizeDegrees(degrees));
}

Vector3 eciToEcef(const Vector3 &eciKm, double secondsSinceJ2000)
{
    const double angle = greenwichMeanSiderealAngle(secondsSinceJ2000);
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);

    return {
        cosine * eciKm.x + sine * eciKm.y,
        -sine * eciKm.x + cosine * eciKm.y,
        eciKm.z
    };
}

Vector3 ecefToEci(const Vector3 &ecefKm, double secondsSinceJ2000)
{
    const double angle = greenwichMeanSiderealAngle(secondsSinceJ2000);
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);

    return {
        cosine * ecefKm.x - sine * ecefKm.y,
        sine * ecefKm.x + cosine * ecefKm.y,
        ecefKm.z
    };
}

Vector3 geodeticToEcef(const GeodeticPosition &position)
{
    const double latitude = toRadians(position.latitudeDeg);
    const double longitude = toRadians(position.longitudeDeg);
    const double altitudeKm = position.altitudeMeters / 1000.0;
    const double flattening = EarthFlattening;
    const double eccentricitySquared = flattening * (2.0 - flattening);
    const double sineLatitude = std::sin(latitude);
    const double cosineLatitude = std::cos(latitude);
    const double primeVerticalRadius = EarthEquatorialRadiusKm
        / std::sqrt(1.0 - eccentricitySquared
                    * sineLatitude * sineLatitude);

    return {
        (primeVerticalRadius + altitudeKm)
            * cosineLatitude * std::cos(longitude),
        (primeVerticalRadius + altitudeKm)
            * cosineLatitude * std::sin(longitude),
        (primeVerticalRadius * (1.0 - eccentricitySquared) + altitudeKm)
            * sineLatitude
    };
}

GeodeticPosition ecefToGeodetic(const Vector3 &ecefKm)
{
    const double flattening = EarthFlattening;
    const double eccentricitySquared = flattening * (2.0 - flattening);
    const double longitude = std::atan2(ecefKm.y, ecefKm.x);
    const double horizontalRadius = std::hypot(ecefKm.x, ecefKm.y);

    double latitude = std::atan2(
        ecefKm.z,
        horizontalRadius * (1.0 - eccentricitySquared));
    double altitudeKm = 0.0;

    for (int iteration = 0; iteration < 12; ++iteration)
    {
        const double sineLatitude = std::sin(latitude);
        const double primeVerticalRadius = EarthEquatorialRadiusKm
            / std::sqrt(1.0 - eccentricitySquared
                        * sineLatitude * sineLatitude);
        const double cosineLatitude = std::cos(latitude);

        if (std::abs(cosineLatitude) > 1.0e-10)
        {
            altitudeKm = horizontalRadius / cosineLatitude
                - primeVerticalRadius;
        }
        else
        {
            altitudeKm = std::abs(ecefKm.z)
                - primeVerticalRadius * (1.0 - eccentricitySquared);
        }

        const double denominator = horizontalRadius
            * (1.0 - eccentricitySquared
               * primeVerticalRadius / (primeVerticalRadius + altitudeKm));
        const double nextLatitude = std::atan2(ecefKm.z, denominator);

        if (std::abs(nextLatitude - latitude) < 1.0e-13)
        {
            latitude = nextLatitude;
            break;
        }

        latitude = nextLatitude;
    }

    GeodeticPosition result;
    result.latitudeDeg = toDegrees(latitude);
    result.longitudeDeg = toDegrees(longitude);
    result.altitudeMeters = altitudeKm * 1000.0;

    return result;
}

TopocentricObservation observeFromGroundStation(
    const Vector3 &satelliteEcefKm,
    const GeodeticPosition &station)
{
    const Vector3 stationEcef = geodeticToEcef(station);
    const Vector3 delta = {
        satelliteEcefKm.x - stationEcef.x,
        satelliteEcefKm.y - stationEcef.y,
        satelliteEcefKm.z - stationEcef.z
    };

    const double latitude = toRadians(station.latitudeDeg);
    const double longitude = toRadians(station.longitudeDeg);
    const double sinLatitude = std::sin(latitude);
    const double cosLatitude = std::cos(latitude);
    const double sinLongitude = std::sin(longitude);
    const double cosLongitude = std::cos(longitude);

    const double east = -sinLongitude * delta.x
        + cosLongitude * delta.y;
    const double north = -sinLatitude * cosLongitude * delta.x
        - sinLatitude * sinLongitude * delta.y
        + cosLatitude * delta.z;
    const double up = cosLatitude * cosLongitude * delta.x
        + cosLatitude * sinLongitude * delta.y
        + sinLatitude * delta.z;

    TopocentricObservation observation;
    observation.azimuthDeg = normalizeDegrees(
        toDegrees(std::atan2(east, north)));
    observation.elevationDeg = toDegrees(
        std::atan2(up, std::hypot(east, north)));
    observation.rangeKm = norm(delta);

    return observation;
}

double orbitalPeriodSeconds(const OrbitalElements &elements,
                            double gravitationalParameter)
{
    if (!std::isfinite(elements.semiMajorAxisKm)
        || !std::isfinite(gravitationalParameter)
        || elements.semiMajorAxisKm <= 0.0
        || gravitationalParameter <= 0.0)
    {
        return 0.0;
    }

    return 2.0 * Pi * std::sqrt(
        std::pow(elements.semiMajorAxisKm, 3.0) / gravitationalParameter);
}

} // namespace OrbitMath
