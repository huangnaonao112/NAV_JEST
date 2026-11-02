#include "OrbitMessage.h"

#include <QList>
#include <QtMath>

namespace OrbitMessage
{

namespace
{

// GPS 历元（1980-01-06 00:00:00）到 J2000（2000-01-01 12:00:00）的秒数
constexpr double kGpsToJ2000Seconds = 630763200.0;
constexpr double kSecondsPerWeek = 604800.0;

bool parseDouble(const QByteArray &text, double &value)
{
    bool ok = false;
    value = text.trimmed().toDouble(&ok);
    return ok;
}

bool parseInt(const QByteArray &text, int &value)
{
    bool ok = false;
    value = text.trimmed().toInt(&ok);
    return ok;
}

} // namespace

QString Satellite::displayName() const
{
    return QStringLiteral("%1%2").arg(QChar(system)).arg(prn, 2, 10, QLatin1Char('0'));
}

bool parseOrbitFrame(const QByteArray &payload, Frame &out, QString &error)
{
    out = Frame();

    const QList<QByteArray> parts = payload.split(',');
    // ORB,time,week,tow,count + 10*count
    if (parts.size() < 5) {
        error = QStringLiteral("ORB frame requires at least 5 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "ORB") {
        error = QStringLiteral("expected ORB, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    // parts[1] 为 HHMMSS.mmm 时间戳，仅用于日志，不参与解算（历元由 week/tow 给出）
    if (!parseInt(parts[2], out.week)) {
        error = QStringLiteral("bad week");
        return false;
    }
    if (!parseDouble(parts[3], out.secondsOfWeek)
        || out.secondsOfWeek < 0.0 || out.secondsOfWeek >= kSecondsPerWeek) {
        error = QStringLiteral("bad seconds-of-week");
        return false;
    }

    int count = 0;
    if (!parseInt(parts[4], count) || count <= 0 || count > 128) {
        error = QStringLiteral("bad satellite count");
        return false;
    }
    const int expected = 5 + count * 10;
    if (parts.size() < expected) {
        error = QStringLiteral("ORB frame requires %1 fields, got %2")
                    .arg(expected).arg(parts.size());
        return false;
    }

    out.satellites.reserve(count);
    for (int i = 0; i < count; ++i) {
        const int base = 5 + i * 10;

        Satellite sat;
        if (!parseInt(parts[base + 0], sat.prn) || sat.prn <= 0 || sat.prn > 64) {
            error = QStringLiteral("bad PRN at satellite %1").arg(i + 1);
            return false;
        }
        const QByteArray sys = parts[base + 1].trimmed().toUpper();
        if (sys.size() != 1 || (sys[0] != 'G' && sys[0] != 'C' && sys[0] != 'E')) {
            error = QStringLiteral("bad system at satellite %1").arg(i + 1);
            return false;
        }
        sat.system = sys[0];

        if (!parseDouble(parts[base + 2], sat.semiMajorAxisKm)
            || sat.semiMajorAxisKm < 6000.0 || sat.semiMajorAxisKm > 60000.0) {
            error = QStringLiteral("bad semi-major axis at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseDouble(parts[base + 3], sat.eccentricity)
            || sat.eccentricity < 0.0 || sat.eccentricity >= 0.9) {
            error = QStringLiteral("bad eccentricity at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseDouble(parts[base + 4], sat.inclinationDeg)
            || sat.inclinationDeg < 0.0 || sat.inclinationDeg > 180.0) {
            error = QStringLiteral("bad inclination at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseDouble(parts[base + 5], sat.raanDeg)) {
            error = QStringLiteral("bad RAAN at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseDouble(parts[base + 6], sat.argumentOfPerigeeDeg)) {
            error = QStringLiteral("bad argument of perigee at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseDouble(parts[base + 7], sat.meanAnomalyDeg)) {
            error = QStringLiteral("bad mean anomaly at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseDouble(parts[base + 8], sat.meanMotionRevPerDay)
            || sat.meanMotionRevPerDay < 0.0 || sat.meanMotionRevPerDay > 20.0) {
            error = QStringLiteral("bad mean motion at satellite %1").arg(i + 1);
            return false;
        }
        if (!parseInt(parts[base + 9], sat.health)) {
            error = QStringLiteral("bad health at satellite %1").arg(i + 1);
            return false;
        }

        out.satellites.append(sat);
    }

    return true;
}

double gpsToSecondsSinceJ2000(int week, double secondsOfWeek)
{
    return double(week) * kSecondsPerWeek + secondsOfWeek - kGpsToJ2000Seconds;
}

} // namespace OrbitMessage
