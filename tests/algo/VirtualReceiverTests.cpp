#include "VirtualReceiver.h"
#include "OrbitalMechanics.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace
{

double rangeMeters(const OrbitMath::Vector3 &a,
                   const OrbitMath::Vector3 &b)
{
    const double dx = (a.x - b.x) * 1000.0;
    const double dy = (a.y - b.y) * 1000.0;
    const double dz = (a.z - b.z) * 1000.0;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool testLeastSquaresPosition()
{
    const OrbitMath::Vector3 truth =
        OrbitMath::geodeticToEcef({39.9042, 116.4074, 50.0});

    const std::vector<OrbitMath::Vector3> satellites {
        {15600.0, 7540.0, 20140.0},
        {18760.0, -2750.0, 18610.0},
        {17610.0, 14630.0, 13480.0},
        {19170.0, 610.0, 18390.0},
        {-13400.0, 21000.0, 11000.0},
        {-21000.0, -11000.0, 8000.0},
        {7000.0, -23000.0, 12000.0},
        {-10000.0, 10000.0, -22000.0}
    };

    std::vector<NavJest::PseudorangeMeasurement> measurements;
    int prn = 1;

    for (const OrbitMath::Vector3 &satellite : satellites)
    {
        NavJest::PseudorangeMeasurement measurement;
        measurement.prn = prn++;
        measurement.satelliteEcefKm = satellite;
        measurement.pseudorangeMeters = rangeMeters(satellite, truth) + 65.0;
        measurement.cn0DbHz = 42.0;
        measurements.push_back(measurement);
    }

    const OrbitMath::Vector3 initial {
        truth.x + 0.020,
        truth.y - 0.015,
        truth.z + 0.010
    };

    NavJest::VirtualGnssReceiver receiver;
    const NavJest::ReceiverSolution solution =
        receiver.solvePosition(measurements, initial);

    if (!solution.valid)
    {
        std::cerr << "Position solve failed: " << solution.message << '\n';
        return false;
    }

    const double error = rangeMeters(solution.positionEcefKm, truth);
    std::cout << "Position error: " << error
              << " m, RMS: " << solution.rmsResidualMeters << " m\n";

    return error < 1.0 && solution.usedSatellites >= 4;
}

bool testInsufficientMeasurements()
{
    std::vector<NavJest::PseudorangeMeasurement> measurements(3);
    NavJest::VirtualGnssReceiver receiver;
    const NavJest::ReceiverSolution solution =
        receiver.solvePosition(measurements, {6378.0, 0.0, 0.0});

    return !solution.valid;
}

} // namespace

int main()
{
    const bool positionPassed = testLeastSquaresPosition();
    const bool minimumPassed = testInsufficientMeasurements();

    std::cout << "Virtual receiver tests: "
              << ((positionPassed && minimumPassed) ? "PASS" : "FAIL")
              << '\n';

    return positionPassed && minimumPassed ? 0 : 1;
}
