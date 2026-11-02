#include "GnssSignalSimulation.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace GnssSignal
{
namespace
{

constexpr double MinimumPowerDbm = -180.0;
constexpr double MaximumChannelPowerDbm = -60.0;
constexpr double MaximumScenarioPowerDbm = -40.0;
constexpr double MinimumBandwidthMHz = 0.001;
constexpr double MaximumBandwidthMHz = 50.0;
constexpr double MaximumFrequencyOffsetMHz = 100.0;
constexpr double MaximumPseudorangeBiasMeters = 100000.0;
constexpr double MaximumDopplerOffsetHz = 100000.0;
constexpr double MaximumTimeOffsetMicroseconds = 1000000.0;
constexpr double BoltzmannNoiseDensityDbmHz = -174.0;
constexpr double MinimumPositive = 1.0e-30;

bool isFinite(double value)
{
    return std::isfinite(value);
}

double log10Safe(double value)
{
    return 10.0 * std::log10(std::max(value, MinimumPositive));
}

ProfileInfo profileInfoFor(SignalProfile profile)
{
    switch (profile)
    {
    case SignalProfile::BdsB1I:
        return {
            "BDS_B1I",
            "北斗 B1I",
            1561.098,
            2.046,
            2046,
            50.0,
            "1561.098 MHz · 2.046 Mcps · 2046 chips · 50 bps"
        };
    case SignalProfile::GpsL1Ca:
        return {
            "GPS_L1_CA",
            "GPS L1 C/A",
            1575.420,
            1.023,
            1023,
            50.0,
            "1575.420 MHz · 1.023 Mcps · 1023 chips · 50 bps"
        };
    case SignalProfile::GalileoE1:
        return {
            "GALILEO_E1",
            "Galileo E1",
            1575.420,
            1.023,
            4092,
            250.0,
            "1575.420 MHz · 1.023 Mcps · E1B/E1C 演示参数"
        };
    }

    return {};
}

double sumPowerDbm(const std::array<Channel, ChannelCount> &channels,
                   int &enabledCount,
                   double &averagePowerDbm)
{
    double totalMilliwatts = 0.0;
    enabledCount = 0;

    for (const Channel &channel : channels)
    {
        if (!channel.enabled)
        {
            continue;
        }

        totalMilliwatts += std::pow(10.0, channel.powerDbm / 10.0);
        ++enabledCount;
    }

    if (enabledCount == 0)
    {
        averagePowerDbm = MinimumPowerDbm;
        return MinimumPowerDbm;
    }

    const double averageMilliwatts =
        totalMilliwatts / static_cast<double>(enabledCount);
    averagePowerDbm = 10.0 * std::log10(averageMilliwatts);
    return 10.0 * std::log10(totalMilliwatts);
}

} // namespace

SignalSimulationEngine::SignalSimulationEngine()
{
    reset();
}

const Configuration &SignalSimulationEngine::configuration() const
{
    return m_configuration;
}

ProfileInfo SignalSimulationEngine::profileInfo() const
{
    return profileInfoFor(m_configuration.profile);
}

bool SignalSimulationEngine::profileFromId(const std::string &id,
                                           SignalProfile &profile)
{
    if (id == "BDS_B1I")
    {
        profile = SignalProfile::BdsB1I;
        return true;
    }

    if (id == "GPS_L1_CA")
    {
        profile = SignalProfile::GpsL1Ca;
        return true;
    }

    if (id == "GALILEO_E1")
    {
        profile = SignalProfile::GalileoE1;
        return true;
    }

    return false;
}

const char *SignalSimulationEngine::profileId(SignalProfile profile)
{
    return profileInfoFor(profile).id;
}

const char *SignalSimulationEngine::interferenceTypeId(
    InterferenceType type)
{
    switch (type)
    {
    case InterferenceType::BroadbandNoise:
        return "BROADBAND";
    case InterferenceType::NarrowbandTone:
        return "NARROWBAND";
    case InterferenceType::PulsedNoise:
        return "PULSED";
    }

    return "BROADBAND";
}

bool SignalSimulationEngine::interferenceTypeFromId(
    const std::string &id,
    InterferenceType &type)
{
    if (id == "BROADBAND")
    {
        type = InterferenceType::BroadbandNoise;
        return true;
    }

    if (id == "NARROWBAND")
    {
        type = InterferenceType::NarrowbandTone;
        return true;
    }

    if (id == "PULSED")
    {
        type = InterferenceType::PulsedNoise;
        return true;
    }

    return false;
}

bool SignalSimulationEngine::setProfile(const std::string &profileIdValue)
{
    SignalProfile profile = SignalProfile::BdsB1I;

    if (!profileFromId(profileIdValue, profile))
    {
        return false;
    }

    m_configuration.profile = profile;
    return true;
}

bool SignalSimulationEngine::updateChannel(int channelIndex,
                                           bool enabled,
                                           double powerDbm,
                                           int prn)
{
    if (channelIndex < 0 || channelIndex >= ChannelCount)
    {
        return false;
    }

    if (!isFinite(powerDbm)
        || powerDbm < MinimumPowerDbm
        || powerDbm > MaximumChannelPowerDbm)
    {
        return false;
    }

    if (prn < 1 || prn > 63)
    {
        return false;
    }

    Channel &channel =
        m_configuration.channels[static_cast<std::size_t>(channelIndex)];
    channel.enabled = enabled;
    channel.powerDbm = powerDbm;
    channel.prn = prn;

    return true;
}

bool SignalSimulationEngine::configureInterference(
    bool enabled,
    const std::string &typeId,
    double centerOffsetMHz,
    double bandwidthMHz,
    double powerDbm,
    double dutyCyclePercent)
{
    InterferenceType type = InterferenceType::BroadbandNoise;

    if (!interferenceTypeFromId(typeId, type))
    {
        return false;
    }

    if (!isFinite(centerOffsetMHz)
        || !isFinite(bandwidthMHz)
        || !isFinite(powerDbm)
        || !isFinite(dutyCyclePercent))
    {
        return false;
    }

    if (std::abs(centerOffsetMHz) > MaximumFrequencyOffsetMHz
        || bandwidthMHz < MinimumBandwidthMHz
        || bandwidthMHz > MaximumBandwidthMHz
        || powerDbm < MinimumPowerDbm
        || powerDbm > MaximumScenarioPowerDbm
        || dutyCyclePercent < 0.1
        || dutyCyclePercent > 100.0)
    {
        return false;
    }

    m_configuration.interference.enabled = enabled;
    m_configuration.interference.type = type;
    m_configuration.interference.centerOffsetMHz = centerOffsetMHz;
    m_configuration.interference.bandwidthMHz = bandwidthMHz;
    m_configuration.interference.powerDbm = powerDbm;
    m_configuration.interference.dutyCyclePercent = dutyCyclePercent;

    return true;
}

bool SignalSimulationEngine::configureSpoofing(
    bool enabled,
    int prn,
    double powerDbm,
    double pseudorangeBiasMeters,
    double dopplerOffsetHz,
    double timeOffsetMicroseconds)
{
    if (!isFinite(powerDbm)
        || !isFinite(pseudorangeBiasMeters)
        || !isFinite(dopplerOffsetHz)
        || !isFinite(timeOffsetMicroseconds))
    {
        return false;
    }

    if (prn < 1 || prn > 63
        || powerDbm < MinimumPowerDbm
        || powerDbm > MaximumScenarioPowerDbm
        || std::abs(pseudorangeBiasMeters) > MaximumPseudorangeBiasMeters
        || std::abs(dopplerOffsetHz) > MaximumDopplerOffsetHz
        || std::abs(timeOffsetMicroseconds) > MaximumTimeOffsetMicroseconds)
    {
        return false;
    }

    m_configuration.spoofing.enabled = enabled;
    m_configuration.spoofing.prn = prn;
    m_configuration.spoofing.powerDbm = powerDbm;
    m_configuration.spoofing.pseudorangeBiasMeters =
        pseudorangeBiasMeters;
    m_configuration.spoofing.dopplerOffsetHz = dopplerOffsetHz;
    m_configuration.spoofing.timeOffsetMicroseconds =
        timeOffsetMicroseconds;

    return true;
}

void SignalSimulationEngine::reset()
{
    m_configuration = Configuration {};

    for (int index = 0; index < ChannelCount; ++index)
    {
        Channel channel;
        channel.index = index;
        channel.prn = index + 1;
        channel.enabled = true;
        channel.powerDbm = -130.0;

        m_configuration.channels[static_cast<std::size_t>(index)] = channel;
    }
}

Metrics SignalSimulationEngine::evaluate() const
{
    Metrics result;

    result.totalSignalPowerDbm = sumPowerDbm(
        m_configuration.channels,
        result.enabledChannelCount,
        result.averageChannelPowerDbm);

    result.noiseDensityDbmHz =
        BoltzmannNoiseDensityDbmHz
        + m_configuration.receiverNoiseFigureDb;

    result.receiverNoisePowerDbm =
        result.noiseDensityDbmHz
        + log10Safe(m_configuration.receiverBandwidthMHz * 1.0e6);

    result.nominalCn0DbHz =
        result.averageChannelPowerDbm - result.noiseDensityDbmHz;

    result.effectiveCn0DbHz = result.nominalCn0DbHz;
    result.inBandInterferencePowerDbm = MinimumPowerDbm;
    result.interferenceDensityDbmHz = MinimumPowerDbm;

    if (m_configuration.interference.enabled)
    {
        const InterferenceConfig &interference =
            m_configuration.interference;

        const double receiverBandwidthMHz =
            m_configuration.receiverBandwidthMHz;
        const double interferenceLow =
            interference.centerOffsetMHz - interference.bandwidthMHz / 2.0;
        const double interferenceHigh =
            interference.centerOffsetMHz + interference.bandwidthMHz / 2.0;
        const double receiverLow = -receiverBandwidthMHz / 2.0;
        const double receiverHigh = receiverBandwidthMHz / 2.0;
        const double overlapMHz = std::max(
            0.0,
            std::min(interferenceHigh, receiverHigh)
                - std::max(interferenceLow, receiverLow));

        if (overlapMHz > 0.0)
        {
            result.interferenceAffectsReceiverBand = true;

            double effectiveInterferencePowerDbm = interference.powerDbm;

            if (interference.type == InterferenceType::PulsedNoise)
            {
                effectiveInterferencePowerDbm +=
                    10.0 * std::log10(
                        interference.dutyCyclePercent / 100.0);
            }

            const double overlapRatio =
                overlapMHz / interference.bandwidthMHz;
            result.inBandInterferencePowerDbm =
                effectiveInterferencePowerDbm
                + 10.0 * std::log10(std::max(overlapRatio, MinimumPositive));

            result.interferenceDensityDbmHz =
                result.inBandInterferencePowerDbm
                - log10Safe(overlapMHz * 1.0e6);

            result.jammingToSignalDb =
                result.inBandInterferencePowerDbm
                - result.averageChannelPowerDbm;

            const double noiseMilliwatts =
                dbmToMilliwatts(result.noiseDensityDbmHz);
            const double interferenceMilliwatts =
                dbmToMilliwatts(result.interferenceDensityDbmHz);
            const double combinedDensityDbmHz =
                milliwattsToDbm(noiseMilliwatts + interferenceMilliwatts);

            result.effectiveCn0DbHz =
                result.averageChannelPowerDbm - combinedDensityDbmHz;
        }
        else
        {
            result.jammingToSignalDb = -180.0;
        }
    }

    if (m_configuration.spoofing.enabled)
    {
        double authenticPowerDbm = result.averageChannelPowerDbm;

        for (const Channel &channel : m_configuration.channels)
        {
            if (channel.enabled
                && channel.prn == m_configuration.spoofing.prn)
            {
                authenticPowerDbm = channel.powerDbm;
                break;
            }
        }

        result.spoofingToAuthenticDb =
            m_configuration.spoofing.powerDbm - authenticPowerDbm;
        result.spoofingDominant =
            result.spoofingToAuthenticDb >= 0.0;
    }

    if (result.enabledChannelCount == 0)
    {
        result.status = "无有效导航通道";
    }
    else if (result.spoofingDominant)
    {
        result.status = "欺骗信号功率不低于对应真实信号：高风险模型提示";
    }
    else if (result.interferenceAffectsReceiverBand
             && result.jammingToSignalDb >= 0.0)
    {
        result.status = "带内干扰功率达到或超过平均导航信号功率";
    }
    else if (result.interferenceAffectsReceiverBand
             && result.jammingToSignalDb > -10.0)
    {
        result.status = "存在明显带内干扰：需结合接收机测试";
    }
    else if (m_configuration.spoofing.enabled)
    {
        result.status = "欺骗场景已启用：需结合接收机验证";
    }
    else if (m_configuration.interference.enabled
             && !result.interferenceAffectsReceiverBand)
    {
        result.status = "干扰频偏位于接收带宽之外";
    }
    else
    {
        result.status = "信号参数正常：当前为软件模型评估";
    }

    return result;
}

double SignalSimulationEngine::dbmToMilliwatts(double dbm)
{
    return std::pow(10.0, dbm / 10.0);
}

double SignalSimulationEngine::milliwattsToDbm(double milliwatts)
{
    return 10.0 * std::log10(std::max(milliwatts, MinimumPositive));
}

} // namespace GnssSignal
