#ifndef GNSSSIGNALSIMULATION_H
#define GNSSSIGNALSIMULATION_H

#include <array>
#include <string>

namespace GnssSignal
{

constexpr int ChannelCount = 24;

enum class SignalProfile
{
    BdsB1I,
    GpsL1Ca,
    GalileoE1
};

enum class InterferenceType
{
    BroadbandNoise,
    NarrowbandTone,
    PulsedNoise
};

struct ProfileInfo
{
    const char *id = "";
    const char *displayName = "";
    double carrierFrequencyMHz = 0.0;
    double chipRateMcps = 0.0;
    int codeLengthChips = 0;
    double dataRateBps = 0.0;
    const char *description = "";
};

struct Channel
{
    int index = 0;
    int prn = 1;
    bool enabled = true;
    double powerDbm = -130.0;
};

struct InterferenceConfig
{
    bool enabled = false;
    InterferenceType type = InterferenceType::BroadbandNoise;
    double centerOffsetMHz = 0.0;
    double bandwidthMHz = 2.0;
    double powerDbm = -110.0;
    double dutyCyclePercent = 100.0;
};

struct SpoofingConfig
{
    bool enabled = false;
    int prn = 1;
    double powerDbm = -125.0;
    double pseudorangeBiasMeters = 50.0;
    double dopplerOffsetHz = 5.0;
    double timeOffsetMicroseconds = 0.5;
};

struct Configuration
{
    SignalProfile profile = SignalProfile::BdsB1I;
    std::array<Channel, ChannelCount> channels {};
    double receiverNoiseFigureDb = 2.0;
    double receiverBandwidthMHz = 4.0;
    InterferenceConfig interference;
    SpoofingConfig spoofing;
};

struct Metrics
{
    int enabledChannelCount = 0;
    double averageChannelPowerDbm = -180.0;
    double totalSignalPowerDbm = -180.0;
    double noiseDensityDbmHz = -172.0;
    double receiverNoisePowerDbm = -100.0;
    double inBandInterferencePowerDbm = -180.0;
    double interferenceDensityDbmHz = -240.0;
    double nominalCn0DbHz = 0.0;
    double effectiveCn0DbHz = 0.0;
    double jammingToSignalDb = -180.0;
    double spoofingToAuthenticDb = -180.0;
    bool interferenceAffectsReceiverBand = false;
    bool spoofingDominant = false;
    std::string status;
};

class SignalSimulationEngine
{
public:
    SignalSimulationEngine();

    const Configuration &configuration() const;

    ProfileInfo profileInfo() const;
    static bool profileFromId(const std::string &id, SignalProfile &profile);
    static const char *profileId(SignalProfile profile);
    static const char *interferenceTypeId(InterferenceType type);
    static bool interferenceTypeFromId(const std::string &id,
                                        InterferenceType &type);

    bool setProfile(const std::string &profileId);
    bool updateChannel(int channelIndex,
                       bool enabled,
                       double powerDbm,
                       int prn);
    bool configureInterference(bool enabled,
                               const std::string &typeId,
                               double centerOffsetMHz,
                               double bandwidthMHz,
                               double powerDbm,
                               double dutyCyclePercent);
    bool configureSpoofing(bool enabled,
                           int prn,
                           double powerDbm,
                           double pseudorangeBiasMeters,
                           double dopplerOffsetHz,
                           double timeOffsetMicroseconds);
    void reset();

    Metrics evaluate() const;

private:
    static double dbmToMilliwatts(double dbm);
    static double milliwattsToDbm(double milliwatts);

    Configuration m_configuration;
};

} // namespace GnssSignal

#endif // GNSSSIGNALSIMULATION_H
