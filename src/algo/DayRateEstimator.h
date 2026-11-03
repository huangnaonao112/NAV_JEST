#pragma once

namespace navcore
{

/// 回放倍率估计器：由相邻 $ORB 帧的"日时间增量 / 墙钟到达间隔"推算
/// 日时间快进倍率，供渲染端帧间开普勒外推使用。
///
/// 设计要点（历史教训见 docs/轨道参数报文接口.md §10.5）：
///  1) TCP 会把匀速发送的帧合包，到达间隔可缩到毫秒级，直接用瞬时值
///     会让倍率虚高几十倍 -> 外推过冲 -> 下一帧回跳（左右闪动）；
///     因此到达间隔 <50ms 的帧不参与学习；
///  2) 单样本瞬时倍率限幅在当前倍率的 [1/4, 4] 倍，防单帧带偏 EMA；
///  3) EMA(0.75/0.25) 平滑，输出再限幅在 EMA 的 [1/4, 4] 倍；
///  4) lastDayDelta() 供外推钳制（不超过约两帧的日时间）。
class DayRateEstimator
{
public:
    /// 记录一帧：dayDelta = 该帧与前帧的日时间差（周绕回已由调用方归一），
    /// wallDelta = 墙钟到达间隔（秒）。无效样本被静默忽略。
    void update(double dayDelta, double wallDelta);

    /// 复位到"未初始化"状态（换数据源/断线重连时调用）：
    /// 倍率回到 1.0，下一帧直接采用瞬时值。
    void reset();

    /// 当前日时间/墙钟倍率（首帧前为 1.0）
    double dayRate() const { return m_dayRate; }

    /// 最近一次有效帧间隔的日时间（秒），渲染外推钳制的基准
    double lastDayDelta() const { return m_lastDayDelta; }

    /// 是否已有有效样本
    bool initialized() const { return m_rateEmaInit; }

private:
    double m_dayRate = 1.0;
    double m_rateEma = 1.0;
    bool m_rateEmaInit = false;
    double m_lastDayDelta = 300.0;
};

} // namespace navcore
