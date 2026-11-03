#include <QtTest>
#include <QByteArray>
#include <QString>

#include "models/OrbitMessage.h"

/**
 * $ORB 报文接口契约测试。
 *
 * 本项目不解析广播星历：外部解算器把开普勒根数通过 $ORB 报文送进来，
 * 这里校验"契约"本身——字段顺序、单位、非法输入必须被拒绝。
 */
class OrbitMessageTests : public QObject
{
    Q_OBJECT

private slots:
    void parsesValidFrame();
    void rejectsBadCount();
    void rejectsBadSystem();
    void rejectsBadSemiMajorAxis();
    void rejectsTruncatedFrame();
    void rejectsNonOrbHead();
    void gpsToJ2000Conversion();
    void parsesDeceptionFrame();
    void rejectsBadDeceptionFrame();
};

void OrbitMessageTests::parsesValidFrame()
{
    const QByteArray payload =
        "ORB,120000.000,2309,172800.0,2,"
        "1,G,26560.0,0.0130,55.0,120.0,0.0,45.0,2.0057,0,"
        "7,C,42164.0,0.0004,3.0,60.0,180.0,120.0,1.0027,0";

    OrbitMessage::Frame frame;
    QString error;
    QVERIFY2(OrbitMessage::parseOrbitFrame(payload, frame, error), qPrintable(error));
    QCOMPARE(frame.week, 2309);
    QCOMPARE(frame.secondsOfWeek, 172800.0);
    QCOMPARE(frame.satellites.size(), 2);

    const auto &g01 = frame.satellites[0];
    QCOMPARE(g01.prn, 1);
    QCOMPARE(g01.system, 'G');
    QCOMPARE(g01.semiMajorAxisKm, 26560.0);
    QCOMPARE(g01.eccentricity, 0.013);
    QCOMPARE(g01.inclinationDeg, 55.0);
    QCOMPARE(g01.raanDeg, 120.0);
    QCOMPARE(g01.argumentOfPerigeeDeg, 0.0);
    QCOMPARE(g01.meanAnomalyDeg, 45.0);
    QCOMPARE(g01.meanMotionRevPerDay, 2.0057);
    QCOMPARE(g01.health, 0);
    QCOMPARE(g01.displayName(), QStringLiteral("G01"));

    const auto &c07 = frame.satellites[1];
    QCOMPARE(c07.displayName(), QStringLiteral("C07"));
    QCOMPARE(c07.semiMajorAxisKm, 42164.0);
}

void OrbitMessageTests::rejectsBadCount()
{
    OrbitMessage::Frame frame;
    QString error;
    QVERIFY(!OrbitMessage::parseOrbitFrame("ORB,120000.000,2309,172800.0,0", frame, error));
    QVERIFY(!error.isEmpty());
}

void OrbitMessageTests::rejectsBadSystem()
{
    const QByteArray payload =
        "ORB,120000.000,2309,172800.0,1,"
        "1,X,26560.0,0.013,55.0,120.0,0.0,45.0,2.0,0";
    OrbitMessage::Frame frame;
    QString error;
    QVERIFY(!OrbitMessage::parseOrbitFrame(payload, frame, error));
}

void OrbitMessageTests::rejectsBadSemiMajorAxis()
{
    // 半长轴小于地球半径：一定是单位错（m 当 km）或数据错，必须拒绝
    const QByteArray payload =
        "ORB,120000.000,2309,172800.0,1,"
        "1,G,6.6,0.013,55.0,120.0,0.0,45.0,2.0,0";
    OrbitMessage::Frame frame;
    QString error;
    QVERIFY(!OrbitMessage::parseOrbitFrame(payload, frame, error));
}

void OrbitMessageTests::rejectsTruncatedFrame()
{
    const QByteArray payload =
        "ORB,120000.000,2309,172800.0,2,"
        "1,G,26560.0,0.013,55.0,120.0,0.0,45.0,2.0,0";
    OrbitMessage::Frame frame;
    QString error;
    QVERIFY(!OrbitMessage::parseOrbitFrame(payload, frame, error));
    QVERIFY(error.contains("requires"));
}

void OrbitMessageTests::rejectsNonOrbHead()
{
    OrbitMessage::Frame frame;
    QString error;
    QVERIFY(!OrbitMessage::parseOrbitFrame("SAT,120000.000,1,1,0,0", frame, error));
}

void OrbitMessageTests::gpsToJ2000Conversion()
{
    // GPS 周 0 周内 0 秒 = 1980-01-06 00:00:00，
    // 到 J2000（2000-01-01 12:00:00）相差 630763200 s
    QCOMPARE(OrbitMessage::gpsToSecondsSinceJ2000(0, 0.0), 0.0 - 630763200.0);
    // J2000 对应 GPS 周 1042 的第 561600 秒（1042.93 周）
    QCOMPARE(OrbitMessage::gpsToSecondsSinceJ2000(1042, 561600.0), 0.0);
    QCOMPARE(OrbitMessage::gpsToSecondsSinceJ2000(1042, 0.0), -561600.0);
}

void OrbitMessageTests::parsesDeceptionFrame()
{
    const QByteArray payload =
        "DSP,120000.000,2,"
        "1,26560.0,0.001,55.0,120.0,10.0,45.0,1,"
        "2,38900.0,0.002,3.0,60.0,180.0,120.0,0";
    QVector<OrbitMessage::DeceptionEntry> entries;
    QString error;
    QVERIFY2(OrbitMessage::parseDeceptionFrame(payload, entries, error), qPrintable(error));
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].index, 1);
    QCOMPARE(entries[0].aKm, 26560.0);
    QCOMPARE(entries[0].e, 0.001);
    QCOMPARE(entries[0].iDeg, 55.0);
    QCOMPARE(entries[0].raanDeg, 120.0);
    QCOMPARE(entries[0].argpDeg, 10.0);
    QCOMPARE(entries[0].M0Deg, 45.0);
    QVERIFY(entries[0].active);
    QVERIFY(!entries[1].active);
}

void OrbitMessageTests::rejectsBadDeceptionFrame()
{
    QVector<OrbitMessage::DeceptionEntry> entries;
    QString error;
    // 数量与字段数不匹配
    QVERIFY(!OrbitMessage::parseDeceptionFrame(
        "DSP,120000.000,2,1,26560.0,0.001,55.0,120.0,10.0,45.0,1", entries, error));
    // 超范围半长轴（单位错：m 当 km）
    QVERIFY(!OrbitMessage::parseDeceptionFrame(
        "DSP,120000.000,1,1,6.6,0.001,55.0,120.0,10.0,45.0,1", entries, error));
    // 非法 active
    QVERIFY(!OrbitMessage::parseDeceptionFrame(
        "DSP,120000.000,1,1,26560.0,0.001,55.0,120.0,10.0,45.0,2", entries, error));
    // 非法偏心率
    QVERIFY(!OrbitMessage::parseDeceptionFrame(
        "DSP,120000.000,1,1,26560.0,1.5,55.0,120.0,10.0,45.0,1", entries, error));
}

QTEST_APPLESS_MAIN(OrbitMessageTests)
#include "OrbitMessageTests.moc"
