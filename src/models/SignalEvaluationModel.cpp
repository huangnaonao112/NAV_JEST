#include "SignalEvaluationModel.h"

#include "GnssSignalSimulation.h"
#include "OrbitalMechanics.h"
#include "VirtualReceiver.h"

#include <array>
#include <cmath>
#include <random>
#include <vector>

namespace
{

// 演示星座：8 颗 MEO 级卫星的 ECEF 位置（km），与 VirtualReceiverTests 一致，
// 保证桥接层行为与算法回归测试可互相印证。
constexpr std::array<OrbitMath::Vector3, 8> kDemoConstellation { {
    {15600.0, 7540.0, 20140.0},
    {18760.0, -2750.0, 18610.0},
    {17610.0, 14630.0, 13480.0},
    {19170.0, 610.0, 18390.0},
    {-13400.0, 21000.0, 11000.0},
    {-21000.0, -11000.0, 8000.0},
    {7000.0, -23000.0, 12000.0},
    {-10000.0, 10000.0, -22000.0}
} };

// 演示场景的接收机钟差（等效距离，米），与 VirtualReceiverTests 相同。
constexpr double kScenarioClockBiasMeters = 65.0;

// 伪距热噪声模型：σ = kNoiseSigmaRefM × √(C/N0ref / C/N0eff)（线性比），
// 参考点 42 dB-Hz 时 σ=1.5 m；确定性固定种子保证每次评估可回归。
constexpr double kNoiseSeed = 20261102.0;
constexpr double kNoiseSigmaRefMeters = 1.5;
constexpr double kNoiseReferenceCn0DbHz = 42.0;
constexpr double kNoiseSigmaMaxMeters = 30.0;

double rangeMeters(const OrbitMath::Vector3 &a, const OrbitMath::Vector3 &b)
{
    const double dx = (a.x - b.x) * 1000.0;
    const double dy = (a.y - b.y) * 1000.0;
    const double dz = (a.z - b.z) * 1000.0;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

SignalEvaluationModel::SignalEvaluationModel(QObject *parent)
    : QObject(parent)
{
}

void SignalEvaluationModel::runEvaluation()
{
    // 1) 信号场景评估：把 QML 配置搬到算法引擎
    GnssSignal::SignalSimulationEngine engine;

    QString statusPrefix;
    if (!engine.setProfile(m_profileId.toStdString()))
    {
        engine.setProfile(GnssSignal::SignalSimulationEngine::profileId(
            GnssSignal::SignalProfile::BdsB1I));
        statusPrefix = QStringLiteral("未知信号体制 %1，已回退 BDS_B1I；")
                           .arg(m_profileId);
    }

    std::string interferenceTypeId = m_interferenceTypeId.toStdString();
    GnssSignal::InterferenceType interferenceType;
    if (!GnssSignal::SignalSimulationEngine::interferenceTypeFromId(
            interferenceTypeId, interferenceType))
    {
        interferenceTypeId = GnssSignal::SignalSimulationEngine::interferenceTypeId(
            GnssSignal::InterferenceType::BroadbandNoise);
        statusPrefix += QStringLiteral("未知干扰类型，已回退宽带噪声；");
    }

    if (!engine.configureInterference(m_interferenceEnabled,
                                      interferenceTypeId,
                                      m_interferenceCenterOffsetMHz,
                                      m_interferenceBandwidthMHz,
                                      m_interferencePowerDbm,
                                      m_interferenceDutyCyclePercent))
    {
        statusPrefix += QStringLiteral("干扰参数被算法层拒绝；");
    }

    if (!engine.configureSpoofing(m_spoofingEnabled,
                                  m_spoofingPrn,
                                  m_spoofingPowerDbm,
                                  m_spoofingRangeBiasMeters,
                                  5.0,
                                  0.5))
    {
        statusPrefix += QStringLiteral("欺骗参数被算法层拒绝；");
    }

    const GnssSignal::Metrics metrics = engine.evaluate();

    // 2) 定位解算：以评估出的有效 C/N0 作为各通道载噪比，生成伪距观测
    const OrbitMath::GeodeticPosition truthGeo {
        m_truthLatitudeDeg, m_truthLongitudeDeg, m_truthAltitudeMeters };
    const OrbitMath::Vector3 truth = OrbitMath::geodeticToEcef(truthGeo);

    std::vector<NavJest::PseudorangeMeasurement> measurements;
    measurements.reserve(kDemoConstellation.size());

    // 伪距测量噪声：幅度随有效 C/N0 恶化而增大（干扰→C/N0↓→噪声↑→误差↑），
    // 固定种子 + PRN 顺序采样，结果逐次可复现
    std::mt19937 noiseEngine(static_cast<unsigned>(kNoiseSeed));
    double noiseSigmaMeters = 0.0;
    if (m_measurementNoiseEnabled)
    {
        const double effLinear = std::pow(10.0, metrics.effectiveCn0DbHz / 10.0);
        const double refLinear = std::pow(10.0, kNoiseReferenceCn0DbHz / 10.0);
        noiseSigmaMeters = std::clamp(
            kNoiseSigmaRefMeters * std::sqrt(refLinear / std::max(effLinear, 1e-9)),
            0.0, kNoiseSigmaMaxMeters);
    }
    std::normal_distribution<double> noise(0.0, noiseSigmaMeters);

    int prn = 1;
    for (const OrbitMath::Vector3 &satellite : kDemoConstellation)
    {
        NavJest::PseudorangeMeasurement measurement;
        measurement.prn = prn;
        measurement.satelliteEcefKm = satellite;
        double pseudorange = rangeMeters(satellite, truth) + kScenarioClockBiasMeters;
        // 欺骗场景：对指定 PRN 通道注入伪距偏差（对应欺骗功率比干扰效应）
        if (m_spoofingEnabled && prn == m_spoofingPrn)
        {
            pseudorange += m_spoofingRangeBiasMeters;
        }
        // 热噪声：每通道独立采样（顺序固定 → 确定性）
        if (m_measurementNoiseEnabled)
        {
            pseudorange += noise(noiseEngine);
        }
        measurement.pseudorangeMeters = pseudorange;
        measurement.dopplerHz = (m_spoofingEnabled && prn == m_spoofingPrn)
                                    ? 5.0
                                    : 0.0;
        measurement.cn0DbHz = metrics.effectiveCn0DbHz;
        measurements.push_back(measurement);
        ++prn;
    }

    // 与回归测试相同的微扰初值，保证解算收敛路径可复现
    const OrbitMath::Vector3 initial {
        truth.x + 0.020, truth.y - 0.015, truth.z + 0.010 };

    NavJest::VirtualGnssReceiver receiver;
    const NavJest::ReceiverSolution solution =
        receiver.solvePosition(measurements, initial);

    // 3) 结果缓存并通知 QML
    m_enabledChannelCount = metrics.enabledChannelCount;
    m_nominalCn0DbHz = metrics.nominalCn0DbHz;
    m_effectiveCn0DbHz = metrics.effectiveCn0DbHz;
    m_jammingToSignalDb = metrics.jammingToSignalDb;
    m_spoofingToAuthenticDb = metrics.spoofingToAuthenticDb;
    m_interferenceInBand = metrics.interferenceAffectsReceiverBand;

    m_receiverValid = solution.valid;
    if (solution.valid)
    {
        m_positionErrorMeters = rangeMeters(solution.positionEcefKm, truth);
        m_clockBiasMeters = solution.clockBiasMeters;
        m_residualRmsMeters = solution.rmsResidualMeters;
        m_usedSatellites = solution.usedSatellites;
        m_solutionIterations = solution.iterations;
        m_solutionMessage = QString::fromStdString(solution.message);
    }
    else
    {
        m_positionErrorMeters = 0.0;
        m_clockBiasMeters = 0.0;
        m_residualRmsMeters = 0.0;
        m_usedSatellites = solution.usedSatellites;
        m_solutionIterations = solution.iterations;
        m_solutionMessage = QString::fromStdString(solution.message);
    }

    m_evaluationStatus = statusPrefix
        + (solution.valid ? QStringLiteral("评估完成")
                          : QStringLiteral("解算失败：") + m_solutionMessage);

    emit evaluationChanged();
}

// ---- 场景配置 getter/setter ----
QString SignalEvaluationModel::profileId() const { return m_profileId; }
void SignalEvaluationModel::setProfileId(const QString &profileId)
{
    if (m_profileId == profileId) return;
    m_profileId = profileId;
    emit configChanged();
}

bool SignalEvaluationModel::interferenceEnabled() const { return m_interferenceEnabled; }
void SignalEvaluationModel::setInterferenceEnabled(bool enabled)
{
    if (m_interferenceEnabled == enabled) return;
    m_interferenceEnabled = enabled;
    emit configChanged();
}

QString SignalEvaluationModel::interferenceTypeId() const { return m_interferenceTypeId; }
void SignalEvaluationModel::setInterferenceTypeId(const QString &typeId)
{
    if (m_interferenceTypeId == typeId) return;
    m_interferenceTypeId = typeId;
    emit configChanged();
}

double SignalEvaluationModel::interferenceCenterOffsetMHz() const { return m_interferenceCenterOffsetMHz; }
void SignalEvaluationModel::setInterferenceCenterOffsetMHz(double value)
{
    if (qFuzzyCompare(m_interferenceCenterOffsetMHz, value)) return;
    m_interferenceCenterOffsetMHz = value;
    emit configChanged();
}

double SignalEvaluationModel::interferenceBandwidthMHz() const { return m_interferenceBandwidthMHz; }
void SignalEvaluationModel::setInterferenceBandwidthMHz(double value)
{
    if (qFuzzyCompare(m_interferenceBandwidthMHz, value)) return;
    m_interferenceBandwidthMHz = value;
    emit configChanged();
}

double SignalEvaluationModel::interferencePowerDbm() const { return m_interferencePowerDbm; }
void SignalEvaluationModel::setInterferencePowerDbm(double value)
{
    if (qFuzzyCompare(m_interferencePowerDbm, value)) return;
    m_interferencePowerDbm = value;
    emit configChanged();
}

double SignalEvaluationModel::interferenceDutyCyclePercent() const { return m_interferenceDutyCyclePercent; }
void SignalEvaluationModel::setInterferenceDutyCyclePercent(double value)
{
    if (qFuzzyCompare(m_interferenceDutyCyclePercent, value)) return;
    m_interferenceDutyCyclePercent = value;
    emit configChanged();
}

bool SignalEvaluationModel::spoofingEnabled() const { return m_spoofingEnabled; }
void SignalEvaluationModel::setSpoofingEnabled(bool enabled)
{
    if (m_spoofingEnabled == enabled) return;
    m_spoofingEnabled = enabled;
    emit configChanged();
}

int SignalEvaluationModel::spoofingPrn() const { return m_spoofingPrn; }
void SignalEvaluationModel::setSpoofingPrn(int prn)
{
    if (m_spoofingPrn == prn) return;
    m_spoofingPrn = prn;
    emit configChanged();
}

double SignalEvaluationModel::spoofingPowerDbm() const { return m_spoofingPowerDbm; }
void SignalEvaluationModel::setSpoofingPowerDbm(double value)
{
    if (qFuzzyCompare(m_spoofingPowerDbm, value)) return;
    m_spoofingPowerDbm = value;
    emit configChanged();
}

double SignalEvaluationModel::spoofingRangeBiasMeters() const { return m_spoofingRangeBiasMeters; }
void SignalEvaluationModel::setSpoofingRangeBiasMeters(double value)
{
    if (qFuzzyCompare(m_spoofingRangeBiasMeters, value)) return;
    m_spoofingRangeBiasMeters = value;
    emit configChanged();
}

bool SignalEvaluationModel::measurementNoiseEnabled() const { return m_measurementNoiseEnabled; }
void SignalEvaluationModel::setMeasurementNoiseEnabled(bool enabled)
{
    if (m_measurementNoiseEnabled == enabled) return;
    m_measurementNoiseEnabled = enabled;
    emit configChanged();
}

double SignalEvaluationModel::truthLatitudeDeg() const { return m_truthLatitudeDeg; }
void SignalEvaluationModel::setTruthLatitudeDeg(double value)
{
    if (qFuzzyCompare(m_truthLatitudeDeg, value)) return;
    m_truthLatitudeDeg = value;
    emit configChanged();
}

double SignalEvaluationModel::truthLongitudeDeg() const { return m_truthLongitudeDeg; }
void SignalEvaluationModel::setTruthLongitudeDeg(double value)
{
    if (qFuzzyCompare(m_truthLongitudeDeg, value)) return;
    m_truthLongitudeDeg = value;
    emit configChanged();
}

double SignalEvaluationModel::truthAltitudeMeters() const { return m_truthAltitudeMeters; }
void SignalEvaluationModel::setTruthAltitudeMeters(double value)
{
    if (qFuzzyCompare(m_truthAltitudeMeters, value)) return;
    m_truthAltitudeMeters = value;
    emit configChanged();
}

// ---- 结果 getter ----
QString SignalEvaluationModel::evaluationStatus() const { return m_evaluationStatus; }
int SignalEvaluationModel::enabledChannelCount() const { return m_enabledChannelCount; }
double SignalEvaluationModel::nominalCn0DbHz() const { return m_nominalCn0DbHz; }
double SignalEvaluationModel::effectiveCn0DbHz() const { return m_effectiveCn0DbHz; }
double SignalEvaluationModel::jammingToSignalDb() const { return m_jammingToSignalDb; }
double SignalEvaluationModel::spoofingToAuthenticDb() const { return m_spoofingToAuthenticDb; }
bool SignalEvaluationModel::interferenceInBand() const { return m_interferenceInBand; }
bool SignalEvaluationModel::receiverValid() const { return m_receiverValid; }
double SignalEvaluationModel::positionErrorMeters() const { return m_positionErrorMeters; }
double SignalEvaluationModel::clockBiasMeters() const { return m_clockBiasMeters; }
double SignalEvaluationModel::residualRmsMeters() const { return m_residualRmsMeters; }
int SignalEvaluationModel::usedSatellites() const { return m_usedSatellites; }
int SignalEvaluationModel::solutionIterations() const { return m_solutionIterations; }
QString SignalEvaluationModel::solutionMessage() const { return m_solutionMessage; }
