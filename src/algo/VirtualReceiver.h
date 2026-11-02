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

// 伪距测量热噪声配置。噪声幅度随有效载噪比恶化而放大：
//   sigma = sigmaRefMeters * sqrt(10^(referenceCn0DbHz/10) / 10^(effectiveCn0DbHz/10))
// 参考点 42 dB-Hz 时 sigma = 1.5 m。固定随机种子，结果逐次可复现。
struct MeasurementNoiseConfig
{
    double sigmaRefMeters = 1.5;
    double referenceCn0DbHz = 42.0;
    double sigmaMaxMeters = 30.0;
    unsigned int seed = 20261102u;
};

// 就地对各通道伪距叠加确定性热噪声（按每通道 C/N0 独立采样）。
// effectiveCn0DbHz 越低，注入的伪距噪声越大；不修改通道的其它字段。
void applyMeasurementNoise(std::vector<PseudorangeMeasurement> &measurements,
                           double effectiveCn0DbHz,
                           const MeasurementNoiseConfig &config = {});

} // namespace NavJest

#endif // VIRTUALRECEIVER_H
