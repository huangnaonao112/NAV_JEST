#include "DayRateEstimator.h"

#include <algorithm>

namespace navcore
{
namespace
{

constexpr double kMinWallDelta = 0.05;   // 合包突发防护阈值（秒）
constexpr double kMaxDayDelta = 7200.0;  // 超过两小时的帧间隔视为异常

} // namespace

void DayRateEstimator::update(double dayDelta, double wallDelta)
{
    if (wallDelta < kMinWallDelta || dayDelta <= 0.0 || dayDelta > kMaxDayDelta)
        return;

    double instant = dayDelta / wallDelta;
    if (m_rateEmaInit)
        instant = std::clamp(instant, m_rateEma * 0.25, m_rateEma * 4.0);

    if (!m_rateEmaInit)
    {
        m_rateEma = instant;
        m_rateEmaInit = true;
    }
    else
    {
        m_rateEma = 0.75 * m_rateEma + 0.25 * instant;
    }

    m_dayRate = std::clamp(m_rateEma, m_rateEma * 0.25, m_rateEma * 4.0);
    m_lastDayDelta = dayDelta;
}

void DayRateEstimator::reset()
{
    m_dayRate = 1.0;
    m_rateEma = 1.0;
    m_rateEmaInit = false;
}

} // namespace navcore
