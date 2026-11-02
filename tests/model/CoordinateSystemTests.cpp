#include <QtTest>
#include <QtMath>
#include <cmath>

#include "models/CoordinateSystem.h"
#include "OrbitalMechanics.h"

/**
 * 尺度常量一致性测试。
 *
 * 这些断言把"场景里只能有一套事实来源"钉死在 CI 里：
 * 任何人不小心把 127.42 / 34.74 / 40 / 80 / 30 改错，或把某处换算改回
 * ×0.01、/100，都会在这里失败。
 */
class CoordinateSystemTests : public QObject
{
    Q_OBJECT

private slots:
    void spherePrimitiveRadiusIsQmlDefault();
    void nodeScalesMatchRealRadii();
    void moonDisplayPositionIsCompressed();
    void thicknessConstantsAreInPlausibleRange();
    void thicknessSanityGuardRejectsKnownBadValues();
    void displaySpaceRoundTrip();
    void cameraDistancesAreSane();
    void sceneAxisConventionMatchesEarthTexture();
};

void CoordinateSystemTests::spherePrimitiveRadiusIsQmlDefault()
{
    // Qt Quick 3D 内置 #Sphere 的基准半径是 50（直径 100）。
    // 这个值一旦假设错误，地球/月球尺寸会整体差 100 倍。
    QCOMPARE(CoordinateSystem::SPHERE_PRIMITIVE_RADIUS, 50.0);
}

void CoordinateSystemTests::nodeScalesMatchRealRadii()
{
    // 节点 scale × 基准半径 = 场景半径，且空间 1 中 1 单位 = 1 km
    QCOMPARE(CoordinateSystem::EARTH_NODE_SCALE * CoordinateSystem::SPHERE_PRIMITIVE_RADIUS,
             CoordinateSystem::EARTH_RADIUS);
    QCOMPARE(CoordinateSystem::MOON_NODE_SCALE * CoordinateSystem::SPHERE_PRIMITIVE_RADIUS,
             CoordinateSystem::MOON_RADIUS_DISPLAY);

    // 与 QML 中历史字面量一致（127.42 / 34.74），避免视觉回归
    QVERIFY(qAbs(CoordinateSystem::EARTH_NODE_SCALE - 127.42) < 1e-6);
    QVERIFY(qAbs(CoordinateSystem::MOON_NODE_SCALE - 34.74) < 1e-6);

    // 大气层：地球半径 + 50 km
    QCOMPARE(CoordinateSystem::ATMOSPHERE_NODE_SCALE * CoordinateSystem::SPHERE_PRIMITIVE_RADIUS,
             CoordinateSystem::ATMOSPHERE_RADIUS_KM);
}

void CoordinateSystemTests::moonDisplayPositionIsCompressed()
{
    // 空间 2：月球摆位压缩到 80000 单位，压缩比 ≈ 0.2081
    QCOMPARE(CoordinateSystem::MOON_SCENE_X, 80000.0);
    QVERIFY(qAbs(CoordinateSystem::DISTANCE_SCALE - 80000.0 / 384400.0) < 1e-12);
    // 压缩比必须显著小于 1（真实距离会被压缩放入同屏）
    QVERIFY(CoordinateSystem::DISTANCE_SCALE > 0.0 && CoordinateSystem::DISTANCE_SCALE < 1.0);
}

void CoordinateSystemTests::thicknessConstantsAreInPlausibleRange()
{
    // 所有线宽都必须落在"肉眼可见且不糊屏"的物理量级（km / 空间2单位）
    const double thicknesses[] = {
        CoordinateSystem::ORBIT_THICKNESS_KM,
        CoordinateSystem::DECEPTION_ORBIT_THICKNESS_KM,
        CoordinateSystem::REPLAY_THICKNESS_UNITS,
        CoordinateSystem::MOON_TRAIL_THICKNESS_KM,
    };
    for (double t : thicknesses) {
        QVERIFY2(t >= 1.0, qPrintable(QStringLiteral("线宽 %1 过小（历史 bug: 0.4）").arg(t)));
        QVERIFY2(t <= 500.0, qPrintable(QStringLiteral("线宽 %1 过大").arg(t)));
    }
}

void CoordinateSystemTests::thicknessSanityGuardRejectsKnownBadValues()
{
    using CS = CoordinateSystem;

    // 正常值（当前线上配置）
    QVERIFY(CS::thicknessIsPlausible(24000.0, CS::ORBIT_THICKNESS_KM));          // 轨道环
    QVERIFY(CS::thicknessIsPlausible(60000.0, CS::REPLAY_THICKNESS_UNITS));      // 回放轨迹
    QVERIFY(CS::thicknessIsPlausible(3480.0, CS::MOON_TRAIL_THICKNESS_KM));      // 月面轨迹

    // 历史错误 1：轨道环线宽 0.4（亚像素不可见）
    QVERIFY(!CS::thicknessIsPlausible(24000.0, 0.4));
    // 历史错误 2：月面点 ×0.01 后仍用 30（≈3000 km 巨带）
    QVERIFY(!CS::thicknessIsPlausible(35.0, 30.0));
}

void CoordinateSystemTests::displaySpaceRoundTrip()
{
    const QVector3D realKm(384400.0f, 0.0f, 0.0f);
    const QVector3D display = CoordinateSystem::toDisplaySpace(realKm);
    QVERIFY(qAbs(display.x() - float(CoordinateSystem::MOON_SCENE_X)) < 1.0f);

    const QVector3D back = CoordinateSystem::toRealKm(display);
    QVERIFY(qAbs(back.x() - realKm.x()) < 0.5f);
}

void CoordinateSystemTests::cameraDistancesAreSane()
{
    // 线宽自适应以 DEFAULT_CAMERA_DISTANCE 为基准：它必须与 QML 默认视距一致，
    // 否则默认视角下线宽会整体偏粗/偏细（会直接看出观感回归）。
    QCOMPARE(CoordinateSystem::DEFAULT_CAMERA_DISTANCE, 30000.0);
    // 月心视角视距应约为月球半径的 4~6 倍（太小会穿模，太大月球变成小点）
    const double ratio = CoordinateSystem::MOON_VIEW_CAMERA_DISTANCE
                         / CoordinateSystem::MOON_RADIUS_DISPLAY;
    QVERIFY2(ratio > 4.0 && ratio < 6.0,
             qPrintable(QStringLiteral("moon view ratio=%1").arg(ratio)));
}

void CoordinateSystemTests::sceneAxisConventionMatchesEarthTexture()
{
    // 场景轴约定(λ=0→+Z, Y=北极, llaToCartesian/地球贴图) 与
    // 标准 ECEF(λ=0→+X, Z=北极, navcore geodeticToEcef) 经
    // sceneFromEcefStd 映射后必须指向同一方向——不一致即"卫星与
    // 地球错位"回归（曾致 2D 标记散布美洲）
    using CS = CoordinateSystem;
    const double cases[][2] = {
        {0.0, 0.0}, {39.9, 116.4}, {-45.0, 120.0}, {80.0, -30.0}, {-60.0, 170.0},
    };
    for (const auto &c : cases) {
        const double lat = c[0], lon = c[1];
        // 场景约定公式（即被钉死的约定本体）：x=cosφ·sinλ, y=sinφ, z=cosφ·cosλ
        const double la = qDegreesToRadians(lat), lo = qDegreesToRadians(lon);
        const QVector3D sceneRef(float(6371.0 * std::cos(la) * std::sin(lo)),
                                 float(6371.0 * std::sin(la)),
                                 float(6371.0 * std::cos(la) * std::cos(lo)));
        OrbitMath::GeodeticPosition geo;
        geo.latitudeDeg = lat;
        geo.longitudeDeg = lon;
        geo.altitudeMeters = 0.0;
        const OrbitMath::Vector3 ecefStd = OrbitMath::geodeticToEcef(geo);
        const QVector3D sceneMapped = CS::sceneFromEcefStd(
            QVector3D(float(ecefStd.x), float(ecefStd.y), float(ecefStd.z)));
        const QVector3D a = sceneRef.normalized();
        const QVector3D b = sceneMapped.normalized();
        QVERIFY2(QVector3D::dotProduct(a, b) > 0.99999,
                 qPrintable(QStringLiteral("lat=%1 lon=%2 dot=%3")
                            .arg(lat).arg(lon)
                            .arg(QVector3D::dotProduct(a, b))));
    }
}

QTEST_APPLESS_MAIN(CoordinateSystemTests)
#include "CoordinateSystemTests.moc"
