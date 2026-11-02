// SatelliteModelNavcoreTests - navcore 真实轨道接口回归测试。
// 用物理锚点做自洽验证：周期/速度/高度与解析解比对，
// 地面站放到星下点时仰角应接近 90°，星下点纬度受倾角约束。
#include "SatelliteModel.h"

#include <QtTest/QtTest>

class SatelliteModelNavcoreTests : public QObject {
    Q_OBJECT

private slots:
    void navcoreStatePhysics();
    void navcoreObservationSubPoint();
    void navcoreGroundTrack();
    void invalidIndexFails();
};

namespace
{
// 圆轨道 LEO: a=7000km, e=0, i=53°, RAAN/ω/ν=0（历元位于升交点）。
// 返回追加后的索引（构造函数预置 15 颗默认卫星，demo 在末尾）。
int addDemoLeo(SatelliteModel &model)
{
    const int index = model.satelliteCount();
    model.addSatellite(7000.0, 0.0, 53.0, 0.0, 0.0, 0.0, 15.0, QColor("red"));
    return index;
}
} // namespace

void SatelliteModelNavcoreTests::navcoreStatePhysics()
{
    SatelliteModel model;
    const int index = addDemoLeo(model);
    const QVariantMap s = model.navcoreState(index, 0.0);
    QVERIFY(s.value("valid").toBool());

    // 周期 2π√(a³/μ) = 2π√(7000³/398600.4418) ≈ 97.14 min
    QVERIFY2(qAbs(s.value("periodMinutes").toDouble() - 97.14) < 0.3,
             qPrintable(QStringLiteral("周期=%1 min")
                            .arg(s.value("periodMinutes").toDouble())));

    // 圆轨道赤道高度 ≈ a - R_eq = 7000 - 6378.137 ≈ 621.9 km
    const double altitudeKm = s.value("altitudeKm").toDouble();
    QVERIFY2(altitudeKm > 610.0 && altitudeKm < 635.0,
             qPrintable(QStringLiteral("高度=%1 km").arg(altitudeKm)));

    // 历元在升交点 (RAAN=ω=M=0) → 星下点纬度 ≈ 0
    QVERIFY2(qAbs(s.value("subLatitudeDeg").toDouble()) < 2.0,
             qPrintable(QStringLiteral("星下点纬度=%1")
                            .arg(s.value("subLatitudeDeg").toDouble())));

    // 圆轨道速度 √(μ/a) ≈ 7.546 km/s
    QVERIFY2(qAbs(s.value("speedKmPerSecond").toDouble() - 7.546) < 0.05,
             qPrintable(QStringLiteral("速度=%1 km/s")
                            .arg(s.value("speedKmPerSecond").toDouble())));

    // GMST 在 [0,360) 值域内
    const double gmstDeg = s.value("gmstDeg").toDouble();
    QVERIFY2(gmstDeg >= 0.0 && gmstDeg < 360.0,
             qPrintable(QStringLiteral("GMST=%1").arg(gmstDeg)));
}

void SatelliteModelNavcoreTests::navcoreObservationSubPoint()
{
    SatelliteModel model;
    const int index = addDemoLeo(model);

    const double t = 1234.5;
    const QVariantMap s = model.navcoreState(index, t);
    QVERIFY(s.value("valid").toBool());

    // 地面站放到星下点：仰角应接近 90°，斜距 ≈ 高度
    const QVariantMap o = model.navcoreObservation(
        index, s.value("subLatitudeDeg").toDouble(),
        s.value("subLongitudeDeg").toDouble(), 0.0, t);
    QVERIFY(o.value("valid").toBool());
    QCOMPARE(o.value("visible").toBool(), true);
    QVERIFY2(o.value("elevationDeg").toDouble() > 80.0,
             qPrintable(QStringLiteral("仰角=%1")
                            .arg(o.value("elevationDeg").toDouble())));
    QVERIFY2(qAbs(o.value("rangeKm").toDouble()
                  - s.value("altitudeKm").toDouble()) < 20.0,
             qPrintable(QStringLiteral("斜距=%1 vs 高度=%2")
                            .arg(o.value("rangeKm").toDouble())
                            .arg(s.value("altitudeKm").toDouble())));
}

void SatelliteModelNavcoreTests::navcoreGroundTrack()
{
    SatelliteModel model;
    const int index = addDemoLeo(model);
    const QVariantList track = model.navcoreGroundTrack(index, 0.0, 60, 5);
    QCOMPARE(track.size(), 13);

    double maxAbsLatitude = 0.0;
    for (const QVariant &item : track)
    {
        const QVariantMap point = item.toMap();
        maxAbsLatitude = qMax(maxAbsLatitude,
                              qAbs(point.value("latitudeDeg").toDouble()));
        QVERIFY(qAbs(point.value("longitudeDeg").toDouble()) <= 180.0 + 1e-6);
    }
    // 倾角 53° → 星下点纬度上界 53°
    QVERIFY2(maxAbsLatitude <= 53.5,
             qPrintable(QStringLiteral("最大星下点纬度=%1").arg(maxAbsLatitude)));
}

void SatelliteModelNavcoreTests::invalidIndexFails()
{
    SatelliteModel model;
    addDemoLeo(model);
    const int outOfRange = model.satelliteCount(); // 末尾 +1
    QVERIFY(!model.navcoreState(-1, 0.0).value("valid").toBool());
    QVERIFY(!model.navcoreState(outOfRange, 0.0).value("valid").toBool());
    QVERIFY(model.navcoreGroundTrack(outOfRange, 0.0, 60, 5).isEmpty());
    QVERIFY(!model.navcoreObservation(outOfRange, 39.9, 116.4, 50.0, 0.0)
                 .value("valid").toBool());
    // 预置卫星上接口可用（构造函数默认 15 颗）
    QVERIFY(model.navcoreState(0, 0.0).value("valid").toBool());
}

QTEST_GUILESS_MAIN(SatelliteModelNavcoreTests)
#include "SatelliteModelNavcoreTests.moc"
