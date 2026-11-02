// RouteGeneratorModelTests - 航路生成桥接层回归测试。
// 锚点：7 点演示航路（北京原点）长度 >500 km、时长 >1 h，
// 与 TrajectoryEngineTests 的引擎级断言互相印证。
#include "RouteGeneratorModel.h"

#include <cmath>
#include <QtTest/QtTest>

class RouteGeneratorModelTests : public QObject {
    Q_OBJECT

private slots:
    void demoRouteGeneration();
    void manualRouteGeneration();
    void insufficientWaypointsRejected();
};

void RouteGeneratorModelTests::demoRouteGeneration()
{
    RouteGeneratorModel model;
    const QVariantMap r = model.generateDemoRoute(39.9042, 116.4074, 50.0, 2500.0);

    QCOMPARE(r.value("valid").toBool(), true);
    const int pointCount = r.value("pointCount").toInt();
    QVERIFY2(pointCount > 100,
             qPrintable(QStringLiteral("点数=%1").arg(pointCount)));
    QVERIFY2(r.value("lengthMeters").toDouble() > 500000.0,
             qPrintable(QStringLiteral("长度=%1 m")
                            .arg(r.value("lengthMeters").toDouble())));
    QVERIFY2(r.value("durationSeconds").toDouble() > 3600.0,
             qPrintable(QStringLiteral("时长=%1 s")
                            .arg(r.value("durationSeconds").toDouble())));
    QVERIFY(r.value("points").toList().size() == pointCount);

    // 属性缓存一致
    QCOMPARE(model.lastGeneratedPoints().size(), pointCount);
    QCOMPARE(model.lastStats().value("lengthMeters").toDouble(),
             r.value("lengthMeters").toDouble());

    // 演示航路是原点周边的环形航线：采样点环绕原点（不经过中心，也不跑远）
    double minDegDistance = 1e9;
    double maxDegDistance = 0.0;
    const QVariantList points = r.value("points").toList();
    for (const QVariant &item : points)
    {
        const QVariantMap p = item.toMap();
        const double dLat = p.value("lat").toDouble() - 39.9042;
        const double dLon = p.value("lon").toDouble() - 116.4074;
        const double degDistance = std::sqrt(dLat * dLat + dLon * dLon);
        minDegDistance = qMin(minDegDistance, degDistance);
        maxDegDistance = qMax(maxDegDistance, degDistance);
    }
    QVERIFY2(minDegDistance > 0.05,
             qPrintable(QStringLiteral("距原点最小角距=%1°").arg(minDegDistance)));
    QVERIFY2(maxDegDistance < 8.0,
             qPrintable(QStringLiteral("距原点最大角距=%1°").arg(maxDegDistance)));
}

void RouteGeneratorModelTests::manualRouteGeneration()
{
    RouteGeneratorModel model;
    QVariantList waypoints;
    waypoints.append(QVariantMap{{"name", "起点"}, {"lat", 39.0}, {"lon", 116.0},
                                 {"alt", 100.0}, {"speed", 85.0}});
    waypoints.append(QVariantMap{{"name", "拐点"}, {"lat", 40.0}, {"lon", 117.5},
                                 {"alt", 1000.0}, {"speed", 95.0}});
    waypoints.append(QVariantMap{{"name", "终点"}, {"lat", 38.5}, {"lon", 118.0},
                                 {"alt", 500.0}, {"speed", 85.0}});
    const QVariantMap r = model.generateRoute(waypoints, 2500.0, 3.0, 4.0, 3.0);

    QCOMPARE(r.value("valid").toBool(), true);
    QVERIFY(r.value("points").toList().size() > 50);
    QVERIFY(r.value("lengthMeters").toDouble() > 100000.0);
    // 统计字段存在
    QVERIFY(r.contains("roundedTurnCount"));
    QVERIFY(r.contains("radiusLimitedTurnCount"));
    QVERIFY(r.contains("minimumEffectiveTurnRadiusMeters"));
}

void RouteGeneratorModelTests::insufficientWaypointsRejected()
{
    RouteGeneratorModel model;
    QVariantList one;
    one.append(QVariantMap{{"lat", 39.0}, {"lon", 116.0}});
    const QVariantMap r = model.generateRoute(one, 2500.0, 3.0, 4.0, 3.0);
    QCOMPARE(r.value("valid").toBool(), false);
    QVERIFY(r.value("message").toString().contains(QStringLiteral("2")));
}

QTEST_GUILESS_MAIN(RouteGeneratorModelTests)
#include "RouteGeneratorModelTests.moc"
