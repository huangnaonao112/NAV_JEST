#ifndef ORBITALMECHANICS_H
#define ORBITALMECHANICS_H

#include <cstddef>

namespace OrbitMath
{

// 双精度三维向量。轨道计算统一使用 km、s、km/s，避免使用 Qt Quick 3D 的 float 向量做精密计算。
struct Vector3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct StateVector
{
    Vector3 positionKm;
    Vector3 velocityKmPerSecond;
};

// 经典开普勒轨道根数。角度字段统一使用 degree。
struct OrbitalElements
{
    double semiMajorAxisKm = 7000.0;
    double eccentricity = 0.0;
    double inclinationDeg = 0.0;
    double raanDeg = 0.0;
    double argumentOfPerigeeDeg = 0.0;
    double trueAnomalyDeg = 0.0;
};

struct GeodeticPosition
{
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    double altitudeMeters = 0.0;
};

struct TopocentricObservation
{
    double azimuthDeg = 0.0;
    double elevationDeg = -90.0;
    double rangeKm = 0.0;
};

constexpr double EarthMuKm3PerSecond2 = 398600.4418;
constexpr double EarthEquatorialRadiusKm = 6378.137;
constexpr double EarthFlattening = 1.0 / 298.257223563;
constexpr double EarthRotationRateRadPerSecond = 7.2921150e-5;
constexpr double Pi = 3.1415926535897932384626433832795;

double toRadians(double degrees);
double toDegrees(double radians);
double normalizeRadians(double radians);
double normalizeDegrees(double degrees);

double dot(const Vector3 &a, const Vector3 &b);
Vector3 cross(const Vector3 &a, const Vector3 &b);
double norm(const Vector3 &value);
Vector3 normalized(const Vector3 &value);

bool isValidEllipticOrbit(const OrbitalElements &elements,
                          double minimumPerigeeAltitudeKm = 80.0);

// 使用牛顿迭代法求解 E - e sin(E) = M，返回偏近点角（rad）。
double solveEccentricAnomaly(double meanAnomalyRad,
                             double eccentricity,
                             double tolerance = 1.0e-12,
                             int maximumIterations = 50);

// 根据轨道根数和真近点角计算地心惯性系位置、速度。
StateVector stateFromElements(const OrbitalElements &elements,
                              double trueAnomalyDeg,
                              double gravitationalParameter = EarthMuKm3PerSecond2);

// 从历元轨道根数传播指定秒数，采用二体开普勒模型。
StateVector propagateKepler(const OrbitalElements &elements,
                            double elapsedSeconds,
                            double gravitationalParameter = EarthMuKm3PerSecond2);

// 根据位置、速度反算经典轨道根数。只接受椭圆束缚轨道。
bool elementsFromState(const StateVector &state,
                       OrbitalElements &elements,
                       double gravitationalParameter = EarthMuKm3PerSecond2);

// J2000 起算秒数对应的格林尼治平恒星时角（rad）。
double greenwichMeanSiderealAngle(double secondsSinceJ2000);

// ECI 与 ECEF 互转。旋转只考虑地球自转，不包含极移、章动和岁差的完整高精度模型。
Vector3 eciToEcef(const Vector3 &eciKm, double secondsSinceJ2000);
Vector3 ecefToEci(const Vector3 &ecefKm, double secondsSinceJ2000);

// WGS84 大地坐标与 ECEF 互转。
Vector3 geodeticToEcef(const GeodeticPosition &position);
GeodeticPosition ecefToGeodetic(const Vector3 &ecefKm);

// 计算卫星相对于地面站的方位角、仰角和斜距。
TopocentricObservation observeFromGroundStation(
    const Vector3 &satelliteEcefKm,
    const GeodeticPosition &station);

double orbitalPeriodSeconds(const OrbitalElements &elements,
                             double gravitationalParameter = EarthMuKm3PerSecond2);

} // namespace OrbitMath

#endif // ORBITALMECHANICS_H
