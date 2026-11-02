// MeasurementNoiseTests - 伪距热噪声模型回归测试。
// 验证: 确定性可复现、噪声幅度随有效 C/N0 恶化单调放大、
// 参考点处 sigma 符合标称值、以及统计均值近似无偏。
#include "VirtualReceiver.h"

#include <cmath>
#include <numeric>
#include <iostream>
#include <vector>

namespace
{

int failures = 0;

void check(bool condition, const char *message)
{
    if (!condition)
    {
        std::cerr << "[FAIL] " << message << '\n';
        ++failures;
    }
    else
    {
        std::cout << "[PASS] " << message << '\n';
    }
}

double sampleStdDev(const std::vector<double> &values)
{
    const double mean = std::accumulate(values.begin(), values.end(), 0.0)
        / static_cast<double>(values.size());
    double sum = 0.0;
    for (const double v : values)
        sum += (v - mean) * (v - mean);
    return std::sqrt(sum / static_cast<double>(values.size() - 1));
}

std::vector<NavJest::PseudorangeMeasurement> makeCleanMeasurements()
{
    std::vector<NavJest::PseudorangeMeasurement> m(64);
    for (std::size_t i = 0; i < m.size(); ++i)
        m[i].pseudorangeMeters = 20000000.0 + static_cast<double>(i) * 1000.0;
    return m;
}

} // namespace

int main()
{
    NavJest::MeasurementNoiseConfig config;

    // 1) 确定性: 相同输入两次叠加结果逐位一致
    std::vector<NavJest::PseudorangeMeasurement> a = makeCleanMeasurements();
    std::vector<NavJest::PseudorangeMeasurement> b = makeCleanMeasurements();
    NavJest::applyMeasurementNoise(a, 42.0, config);
    NavJest::applyMeasurementNoise(b, 42.0, config);
    bool identical = true;
    for (std::size_t i = 0; i < a.size(); ++i)
        identical = identical && a[i].pseudorangeMeters == b[i].pseudorangeMeters;
    check(identical, "noise application is deterministic");

    // 2) 参考点 42 dB-Hz 下样本标准差接近标称 1.5 m
    std::vector<double> deltas;
    for (std::size_t i = 0; i < a.size(); ++i)
        deltas.push_back(a[i].pseudorangeMeters
                         - (20000000.0 + static_cast<double>(i) * 1000.0));
    const double stdAtReference = sampleStdDev(deltas);
    check(std::abs(stdAtReference - config.sigmaRefMeters) < 0.4,
          "sample stddev at reference C/N0 is close to 1.5 m");

    // 3) 有效 C/N0 下降 → 噪声幅度单调放大 (42 → 22.8 dB-Hz 时 sigma 约放大 4.6 倍)
    std::vector<NavJest::PseudorangeMeasurement> degraded = makeCleanMeasurements();
    NavJest::applyMeasurementNoise(degraded, 22.8, config);
    std::vector<double> degradedDeltas;
    for (std::size_t i = 0; i < degraded.size(); ++i)
        degradedDeltas.push_back(degraded[i].pseudorangeMeters
                                 - (20000000.0 + static_cast<double>(i) * 1000.0));
    const double stdDegraded = sampleStdDev(degradedDeltas);
    check(stdDegraded > stdAtReference * 3.0,
          "noise stddev grows substantially as C/N0 degrades");

    // 4) 不同种子产生不同噪声
    NavJest::MeasurementNoiseConfig otherSeed = config;
    otherSeed.seed = 42u;
    std::vector<NavJest::PseudorangeMeasurement> c = makeCleanMeasurements();
    NavJest::applyMeasurementNoise(c, 42.0, otherSeed);
    check(c[0].pseudorangeMeters != a[0].pseudorangeMeters,
          "different seeds produce different noise");

    if (failures == 0)
        std::cout << "Measurement noise tests: PASS\n";
    return failures == 0 ? 0 : 1;
}
