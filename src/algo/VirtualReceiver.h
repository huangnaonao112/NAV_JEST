#ifndef VIRTUALRECEIVER_H
#define VIRTUALRECEIVER_H

#include "OrbitalMechanics.h"

#include <string>
#include <vector>

namespace NavJest
{

struct PseudorangeMeasurement
{
    int prn = 0;
    OrbitMath::Vector3 satelliteEcefKm;
    double pseudorangeMeters = 0.0;
    double dopplerHz = 0.0;
    double cn0DbHz = 0.0;
};

struct ReceiverSolution
{
    bool valid = false;
    OrbitMath::Vector3 positionEcefKm;
    double clockBiasMeters = 0.0;
    double rmsResidualMeters = 0.0;
    int usedSatellites = 0;
    int iterations = 0;
    std::string message;
};

// 观测量级虚拟接收机：根据伪距进行四未知数（X/Y/Z/接收机钟差）最小二乘解算。
// 本类不执行真实 RF 捕获、码跟踪或载波跟踪。
class VirtualGnssReceiver
{
public:
    ReceiverSolution solvePosition(
        const std::vector<PseudorangeMeasurement> &measurements,
        const OrbitMath::Vector3 &initialPositionEcefKm) const;
};

} // namespace NavJest

#endif // VIRTUALRECEIVER_H
