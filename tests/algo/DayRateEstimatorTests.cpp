#include "DayRateEstimator.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{

int failures = 0;

void expectNear(const std::string &name,
                double actual,
                double expected,
                double tolerance)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::cerr << "[FAIL] " << name
                  << ": actual=" << actual
                  << ", expected=" << expected
                  << ", tolerance=" << tolerance << '\n';
        ++failures;
        return;
    }
    std::cout << "[PASS] " << name << '\n';
}

} // namespace

int main()
{
    using navcore::DayRateEstimator;

    // ===== 1. 稳态收敛：匀速回放（900s 日时间 / 3.125s 墙钟 = 288x）=====
    {
        DayRateEstimator est;
        for (int i = 0; i < 20; ++i)
            est.update(900.0, 3.125);
        expectNear("steady state converges to 288x",
                   est.dayRate(), 288.0, 1.0e-9);
        expectNear("steady state lastDayDelta", est.lastDayDelta(), 900.0, 1.0e-9);
    }

    // ===== 2. 合包突发防护：<50ms 间隔的帧不参与学习 =====
    {
        DayRateEstimator est;
        for (int i = 0; i < 10; ++i)
            est.update(900.0, 3.125);          // 稳态 288x
        const double before = est.dayRate();
        for (int i = 0; i < 5; ++i)
            est.update(900.0, 0.007);          // 突发：瞬时 128571x
        expectNear("burst frames do not poison EMA",
                   est.dayRate(), before, 1.0e-9);
    }

    // ===== 3. 单样本限幅：一个野样本最多把 EMA 拉向 4 倍 =====
    {
        DayRateEstimator est;
        for (int i = 0; i < 10; ++i)
            est.update(900.0, 3.125);          // 288x
        est.update(900.0, 0.06);               // 合法间隔(>=50ms)但瞬时 15000x
        const double emaAfter = 0.75 * 288.0 + 0.25 * (288.0 * 4.0);
        expectNear("wild sample clamped to 4x before EMA",
                   est.dayRate(), emaAfter, 1.0e-9);
    }

    // ===== 4. 异常样本静默忽略：非正/超大日时间 =====
    {
        DayRateEstimator est;
        for (int i = 0; i < 10; ++i)
            est.update(900.0, 3.125);
        const double before = est.dayRate();
        est.update(-900.0, 3.125);             // 倒放：忽略
        est.update(99999.0, 3.125);            // 超大：忽略
        est.update(900.0, -1.0);               // 非法墙钟：忽略
        expectNear("invalid samples ignored", est.dayRate(), before, 1.0e-9);
    }

    // ===== 5. reset：断线重连后回到实时倍率，下一帧直接采用瞬时值 =====
    {
        DayRateEstimator est;
        for (int i = 0; i < 10; ++i)
            est.update(900.0, 3.125);
        est.reset();
        expectNear("reset restores 1.0x", est.dayRate(), 1.0, 1.0e-12);
        est.update(900.0, 3.125);
        expectNear("first sample after reset adopted directly",
                   est.dayRate(), 288.0, 1.0e-9);
    }

    // ===== 6. 倍率切换：回放加速时 4 倍限幅下逐步跟上 =====
    {
        DayRateEstimator est;
        for (int i = 0; i < 10; ++i)
            est.update(900.0, 3.125);          // 288x
        for (int i = 0; i < 30; ++i)
            est.update(900.0, 0.3125);         // 切换到 2880x
        expectNear("rate change tracked upward", est.dayRate(), 2880.0, 1.0);
    }

    if (failures > 0)
    {
        std::cerr << failures << " test(s) failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "All day rate estimator tests passed.\n";
    return EXIT_SUCCESS;
}
