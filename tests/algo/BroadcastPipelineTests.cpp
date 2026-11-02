#include <QtTest>
#include <cmath>

#include "OrbitalMechanics.h"

/**
 * 广播星历 → 渲染坐标管线回归测试（纯 navcore，无 Qt Quick 依赖）。
 *
 * 锁定的约定（历史 bug 均出在这里，见 docs/轨道参数报文接口.md §11）：
 *   1. 帧根数 (Ω0,ω,M0) 的二体位置在帧时刻即 ECEF（Ω0 已吸收 GMST 偏置）；
 *   2. 帧间外推需绕地轴(Z)转 -Ωe×tow_render，GEO 净转≈0（定点不动）；
 *   3. M0→ν 转换。
 */
class BroadcastPipelineTests : public QObject
{
    Q_OBJECT

private slots:
    void meanToTrueAnomalyBasics();
    void geoStaysPinnedAtAssignedLongitude();
    void meoDriftsAtNMinusOmegaE();
    void spinCorrectionDirection();

private:
    // 合成一帧 GEO/MEO 根数，使 tow=0 时星下点经度 = targetLonDeg
    OrbitMath::OrbitalElements makeElements(double targetLonDeg, double aKm,
                                            double inclinationDeg);

    // app 管线复现：帧根数 + 二体推进 + 地球自转补偿 → 标准ECEF → 星下点经度
    double subLonAt(const OrbitMath::OrbitalElements &el, double tow0,
                    double towRender);
    double subLatAt(const OrbitMath::OrbitalElements &el, double tow0,
                    double towRender);
};

OrbitMath::OrbitalElements
BroadcastPipelineTests::makeElements(double targetLonDeg, double aKm,
                                     double inclinationDeg)
{
    // i≈0 时星下点经度 ≈ Ω0+ω+ν；取 Ω0=100, ω=10, M0=targetLon-110
    OrbitMath::OrbitalElements el;
    el.semiMajorAxisKm = aKm;
    el.eccentricity = 0.0001;
    el.inclinationDeg = inclinationDeg;
    el.raanDeg = 100.0;
    el.argumentOfPerigeeDeg = 10.0;
    el.trueAnomalyDeg = targetLonDeg - 110.0;
    return el;
}

double BroadcastPipelineTests::subLonAt(const OrbitMath::OrbitalElements &el,
                                        double tow0, double towRender)
{
    const double elapsed = towRender - tow0;
    const OrbitMath::StateVector sv = OrbitMath::propagateKepler(el, elapsed);
    const double spin = -OrbitMath::EarthRotationRateRadPerSecond * towRender;
    const OrbitMath::Vector3 ecef = OrbitMath::rotateZ(sv.positionKm, spin);
    return OrbitMath::normalizeDegrees(
        OrbitMath::toDegrees(std::atan2(ecef.y, ecef.x)));
}

double BroadcastPipelineTests::subLatAt(const OrbitMath::OrbitalElements &el,
                                        double tow0, double towRender)
{
    const double elapsed = towRender - tow0;
    const OrbitMath::StateVector sv = OrbitMath::propagateKepler(el, elapsed);
    const double spin = -OrbitMath::EarthRotationRateRadPerSecond * towRender;
    const OrbitMath::Vector3 ecef = OrbitMath::rotateZ(sv.positionKm, spin);
    const double r = std::sqrt(ecef.x * ecef.x + ecef.y * ecef.y + ecef.z * ecef.z);
    return OrbitMath::toDegrees(std::asin(ecef.z / r));
}

void BroadcastPipelineTests::meanToTrueAnomalyBasics()
{
    // 圆轨道：ν = M
    QVERIFY(std::abs(OrbitMath::meanToTrueAnomalyDeg(45.0, 0.0) - 45.0) < 1e-9);
    // M=0 → ν=0（任意 e）
    QVERIFY(std::abs(OrbitMath::meanToTrueAnomalyDeg(0.0, 0.5)) < 1e-9);
    // 近拱点 M=180 → ν=180
    QVERIFY(std::abs(OrbitMath::meanToTrueAnomalyDeg(180.0, 0.3) - 180.0) < 1e-6);
    // 椭圆一般情形：0<M<180 时 ν≥M，且 ν 反解回 M 自洽
    const double e = 0.3;
    const double nu = OrbitMath::meanToTrueAnomalyDeg(60.0, e);
    QVERIFY(nu > 60.0 && nu < 120.0);
    // ν 反解 M（与 propagateKepler 的 ν→E 变换一致）
    const double nuRad = OrbitMath::toRadians(nu);
    const double eBack = 2.0 * std::atan2(
        std::sqrt(1.0 - e) * std::sin(nuRad / 2.0),
        std::sqrt(1.0 + e) * std::cos(nuRad / 2.0));
    const double mBack = OrbitMath::toDegrees(
        OrbitMath::normalizeRadians(eBack - e * std::sin(eBack)));
    QVERIFY2(std::abs(mBack - 60.0) < 1e-6,
             qPrintable(QStringLiteral("mBack=%1").arg(mBack)));
}

void BroadcastPipelineTests::geoStaysPinnedAtAssignedLongitude()
{
    // GEO（i≈0）：全天任意时刻星下点经度恒为 140°E、纬度≈0——
    // "丢失 -Ωe×tow 自转补偿"这类回归会让经度随 tow 漂移（曾偏 39°）
    const OrbitMath::OrbitalElements el = makeElements(140.0, 42164.1, 0.05);
    const double tows[] = { 0.0, 600.0, 5400.0, 43200.0, 86164.0 };
    for (double tow : tows) {
        const double lon = subLonAt(el, 0.0, tow);
        QVERIFY2(std::abs(lon - 140.0) < 0.5,
                 qPrintable(QStringLiteral("tow=%1 lon=%2").arg(tow).arg(lon)));
        QVERIFY(std::abs(subLatAt(el, 0.0, tow)) < 0.2);
    }
}

void BroadcastPipelineTests::meoDriftsAtNMinusOmegaE()
{
    // MEO：ECEF 经度漂移速率 = n - Ωe（度/秒），容差 2%
    // 用 i≈0 的"赤道 MEO"：星下点经度漂移率恒等于节点漂移率 (n-Ωe)。
    // 倾斜轨道(如 i=55°)的星下点经度速率随纬度变化，不能作恒率断言。
    const double aKm = 26560.0;
    const OrbitMath::OrbitalElements el = makeElements(140.0, aKm, 0.05);
    const double n = std::sqrt(OrbitMath::EarthMuKm3PerSecond2 / (aKm * aKm * aKm));
    const double expected = (n - OrbitMath::EarthRotationRateRadPerSecond)
                            * 180.0 / M_PI;

    const double lon0 = subLonAt(el, 0.0, 0.0);
    const double lon1 = subLonAt(el, 0.0, 600.0);
    double dLon = lon1 - lon0;
    if (dLon > 180.0) dLon -= 360.0;
    if (dLon < -180.0) dLon += 360.0;
    const double rate = dLon / 600.0;
    QVERIFY2(std::abs(rate - expected) / expected < 0.02,
             qPrintable(QStringLiteral("rate=%1 expected=%2").arg(rate).arg(expected)));
}

void BroadcastPipelineTests::spinCorrectionDirection()
{
    // 方向校验：不带自转补偿时 GEO 经度随 tow 以 +Ωe 漂移；
    // 带补偿后应抵消（这是"重复扣/漏扣 Ωe"两类 bug 的判定器）
    const OrbitMath::OrbitalElements el = makeElements(140.0, 42164.1, 0.05);
    const double elapsed = 43200.0;
    const OrbitMath::StateVector sv = OrbitMath::propagateKepler(el, elapsed);
    // 不补偿：直出位置经度 ≈ 140 + Ωe×43200 ≈ 140+180.3
    OrbitMath::Vector3 raw = sv.positionKm;
    const double rawLon = OrbitMath::normalizeDegrees(
        OrbitMath::toDegrees(std::atan2(raw.y, raw.x)));
    const double drift = std::abs(rawLon - 140.0);
    QVERIFY2(drift > 170.0, qPrintable(QStringLiteral("raw drift=%1").arg(drift)));
    // 补偿后回到 140
    const double lon = subLonAt(el, 0.0, elapsed);
    QVERIFY2(std::abs(lon - 140.0) < 0.5,
             qPrintable(QStringLiteral("compensated lon=%1").arg(lon)));
}

QTEST_APPLESS_MAIN(BroadcastPipelineTests)
#include "BroadcastPipelineTests.moc"
