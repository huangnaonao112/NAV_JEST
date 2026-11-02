#include "GnssSignalSimulation.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace
{

int g_passed = 0;
int g_failed = 0;

void check(bool condition, const std::string &name)
{
    if (condition)
    {
        ++g_passed;
        std::cout << "[PASS] " << name << '\n';
    }
    else
    {
        ++g_failed;
        std::cout << "[FAIL] " << name << '\n';
    }
}

bool nearlyEqual(double first, double second, double tolerance = 1.0e-6)
{
    return std::abs(first - second) <= tolerance;
}

} // namespace

int main()
{
    using GnssSignal::SignalSimulationEngine;

    SignalSimulationEngine engine;
    const GnssSignal::Configuration &defaultConfig = engine.configuration();

    check(static_cast<int>(defaultConfig.channels.size()) == 24,
          "默认创建 24 个信号通道");

    int enabledCount = 0;
    for (const GnssSignal::Channel &channel : defaultConfig.channels)
    {
        if (channel.enabled && nearlyEqual(channel.powerDbm, -130.0))
        {
            ++enabledCount;
        }
    }

    check(enabledCount == 24, "默认 24 通道均开启且功率为 -130 dBm");

    const GnssSignal::Metrics defaultMetrics = engine.evaluate();
    check(defaultMetrics.enabledChannelCount == 24,
          "默认启用通道数量统计正确");
    check(nearlyEqual(defaultMetrics.averageChannelPowerDbm, -130.0),
          "默认平均单通道功率正确");
    check(nearlyEqual(defaultMetrics.totalSignalPowerDbm,
                      -130.0 + 10.0 * std::log10(24.0)),
          "多通道总功率采用线性功率叠加");
    check(nearlyEqual(defaultMetrics.nominalCn0DbHz, 42.0),
          "默认 C/N0 根据 -174 dBm/Hz 和 2 dB 噪声系数计算");

    check(engine.setProfile("GPS_L1_CA"), "切换 GPS L1 C/A 信号体制");
    check(engine.profileInfo().carrierFrequencyMHz == 1575.420,
          "GPS L1 C/A 载波频率参数正确");
    check(engine.setProfile("GALILEO_E1"), "切换 Galileo E1 信号体制");
    check(engine.profileInfo().codeLengthChips == 4092,
          "Galileo E1 演示码长参数正确");
    check(engine.setProfile("BDS_B1I"), "切换回 BDS B1I 信号体制");
    check(!engine.setProfile("UNKNOWN"), "拒绝未知信号体制");

    check(engine.updateChannel(0, false, -130.0, 1),
          "关闭指定信号通道");
    check(engine.evaluate().enabledChannelCount == 23,
          "关闭通道后启用数量更新");
    check(engine.updateChannel(0, true, -125.0, 1),
          "更新通道功率");
    const double expectedAveragePower = 10.0 * std::log10(
        (std::pow(10.0, -125.0 / 10.0)
         + 23.0 * std::pow(10.0, -130.0 / 10.0)) / 24.0);
    check(nearlyEqual(engine.evaluate().averageChannelPowerDbm,
                      expectedAveragePower),
          "单通道功率修改反映到平均功率");

    check(!engine.updateChannel(24, true, -130.0, 25),
          "拒绝越界通道索引");
    check(!engine.updateChannel(0, true, -20.0, 1),
          "拒绝超出范围的通道功率");
    check(!engine.updateChannel(0, true,
                               std::numeric_limits<double>::quiet_NaN(), 1),
          "拒绝 NaN 通道功率");
    check(!engine.updateChannel(0, true, -130.0, 64),
          "拒绝超出范围的 PRN");

    engine.reset();
    check(engine.configureInterference(
              true, "BROADBAND", 0.0, 2.0, -110.0, 100.0),
          "配置带内宽带干扰");
    const GnssSignal::Metrics jammedMetrics = engine.evaluate();
    check(jammedMetrics.interferenceAffectsReceiverBand,
          "识别干扰与接收带宽重叠");
    check(nearlyEqual(jammedMetrics.jammingToSignalDb, 20.0),
          "计算 J/S 比");
    check(jammedMetrics.effectiveCn0DbHz < jammedMetrics.nominalCn0DbHz,
          "带内干扰降低模型有效 C/N0");

    engine.reset();
    check(engine.configureInterference(
              true, "NARROWBAND", 10.0, 0.01, -80.0, 100.0),
          "配置带外窄带干扰");
    check(!engine.evaluate().interferenceAffectsReceiverBand,
          "带外干扰不计入接收机带内影响");

    engine.reset();
    check(engine.configureInterference(
              true, "PULSED", 0.0, 2.0, -110.0, 10.0),
          "配置脉冲干扰");
    const double pulsedPower =
        engine.evaluate().inBandInterferencePowerDbm;
    engine.configureInterference(
        true, "PULSED", 0.0, 2.0, -110.0, 100.0);
    const double continuousPower =
        engine.evaluate().inBandInterferencePowerDbm;
    check(nearlyEqual(continuousPower - pulsedPower, 10.0),
          "脉冲干扰平均功率按占空比修正");

    check(!engine.configureInterference(
              true, "INVALID", 0.0, 2.0, -110.0, 100.0),
          "拒绝未知干扰类型");
    check(!engine.configureInterference(
              true, "BROADBAND", 0.0, 0.0, -110.0, 100.0),
          "拒绝无效干扰带宽");
    check(!engine.configureInterference(
              true, "BROADBAND", 0.0, 2.0, -20.0, 100.0),
          "拒绝超出范围的干扰功率");

    engine.reset();
    check(engine.configureSpoofing(
              true, 1, -120.0, 100.0, 12.5, 1.2),
          "配置 PRN 1 欺骗场景");
    const GnssSignal::Metrics spoofMetrics = engine.evaluate();
    check(nearlyEqual(spoofMetrics.spoofingToAuthenticDb, 10.0),
          "计算欺骗信号与真实信号功率比");
    check(spoofMetrics.spoofingDominant,
          "识别欺骗信号功率占优场景");

    check(!engine.configureSpoofing(
              true, 64, -120.0, 100.0, 12.5, 1.2),
          "拒绝超出范围的欺骗 PRN");
    check(!engine.configureSpoofing(
              true, 1, -120.0, 100001.0, 12.5, 1.2),
          "拒绝超范围伪距偏差");

    engine.reset();
    for (int index = 0; index < GnssSignal::ChannelCount; ++index)
    {
        engine.updateChannel(index, false, -130.0, index + 1);
    }

    const GnssSignal::Metrics noChannelMetrics = engine.evaluate();
    check(noChannelMetrics.enabledChannelCount == 0,
          "支持全部通道关闭");
    check(noChannelMetrics.status == "无有效导航通道",
          "无有效通道状态提示正确");

    engine.reset();
    const GnssSignal::Metrics resetMetrics = engine.evaluate();
    check(resetMetrics.enabledChannelCount == 24,
          "重置后恢复 24 个默认通道");

    std::cout << "\nTests passed: " << g_passed
              << ", failed: " << g_failed << '\n';

    return g_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
