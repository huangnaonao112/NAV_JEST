#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

/**
 * @brief $ORB 轨道参数报文（本项目只消费，不负责解算）
 *
 * 设计约定：广播星历的解析/精密定轨属于外部解算器（独立工具/服务），
 * 本项目只接收"已经解算好的开普勒根数"并负责 3D/2D 显示。
 *
 * 报文格式（NMEA 风格，与 $SAT/$PNT 一致，字段以逗号分隔）：
 *
 *   $ORB,HHMMSS.mmm,week,tow,count,
 *        PRN,sys,a,e,i,raan,argp,M0,n,health,
 *        ... (重复 count 组)
 *
 * 字段说明（单位必须严格遵守，否则 3D 与 2D 会错位）：
 *   week     GPS 周（BDS 周需 +1356 归一到 GPS 周；Galileo 直接用 GPS 周）
 *   tow      周内秒（GPS 时，0..604800）
 *   count    卫星数量（1..128）
 *   PRN      系统内编号（1..64）
 *   sys      'G' GPS / 'C' 北斗 / 'E' Galileo
 *   a        半长轴 km（如 26560）
 *   e        偏心率（0..0.9）
 *   i        轨道倾角 deg（0..180）
 *   raan     升交点赤经 deg
 *   argp     近地点幅角 deg
 *   M0       参考时刻平近点角 deg
 *   n        平均角速度 rev/day（填 0 表示由 a 推算）
 *   health   0 健康 / 1 不健康（不健康卫星仍接收，但渲染为灰色）
 *
 * 示例（2 颗星）：
 *   $ORB,120000.000,2309,172800.0,2,
 *       1,G,26560.0,0.0130,55.0,120.0,0.0,45.0,2.0057,0,
 *       7,C,42164.0,0.0004,3.0,60.0,180.0,120.0,1.0027,0
 *
 * 语义：
 *   - 根数参考历元 = (week, tow)，渲染侧从该历元起按二体开普勒推进；
 *   - 同一帧内的所有卫星共享同一历元，保证相对几何关系正确；
 *   - 后续帧可周期性重发（建议 1~10 s 一次），用于修正长期漂移。
 */
namespace OrbitMessage
{

struct Satellite
{
    int prn = 0;
    char system = 'G';
    double semiMajorAxisKm = 0.0;
    double eccentricity = 0.0;
    double inclinationDeg = 0.0;
    double raanDeg = 0.0;
    double argumentOfPerigeeDeg = 0.0;
    double meanAnomalyDeg = 0.0;
    double meanMotionRevPerDay = 0.0;   // 0 = 由 a 推算
    int health = 0;

    /** @brief 卫星显示名，如 "G01" / "C07" */
    QString displayName() const;
};

struct Frame
{
    int week = 0;
    double secondsOfWeek = 0.0;
    QVector<Satellite> satellites;
};

/** @brief 解析 $ORB 帧（payload 不含 '$' 与校验和） */
bool parseOrbitFrame(const QByteArray &payload, Frame &out, QString &error);

/** @brief GPS 周内秒 → 距 J2000 的秒数（用于地球自转/ECI→ECEF） */
double gpsToSecondsSinceJ2000(int week, double secondsOfWeek);

// ================= $DSP 欺骗干扰报文 =================
// $DSP,HHMMSS.mmm,count,idx,a,e,i,raan,argp,M0,active[,...]
// 欺骗干扰卫星携带完整开普勒根数——与 $ORB 同语义（M0+n·tow 即 ECEF），
//欺骗星在帧间沿轨道连续运动，干扰波束随卫星移动。active=1 时卫星显示
// 且干扰波束开启。全量声明式：帧中未出现的欺骗源自动移除。
struct DeceptionEntry
{
    int index = 0;          // 1-based 欺骗卫星编号
    double aKm = 0.0;       // 半长轴 km
    double e = 0.0;         // 偏心率
    double iDeg = 0.0;      // 倾角 deg
    double raanDeg = 0.0;   // 升交点赤经 deg
    double argpDeg = 0.0;   // 近地点幅角 deg
    double M0Deg = 0.0;     // 帧时刻平近点角 deg
    bool active = false;    // 干扰波束开关（=卫星可见性）
};

/** @brief 解析 $DSP 帧（payload 不含 '$' 与校验和） */
bool parseDeceptionFrame(const QByteArray &payload,
                         QVector<DeceptionEntry> &out, QString &error);

} // namespace OrbitMessage
