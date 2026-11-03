#include "SatelliteModel.h"
#include "PntDataModel.h"
#include "CoordinateSystem.h"
#include "OrbitMessage.h"
#include "algo/OrbitalMechanics.h"
#include "utils/SceneLogger.h"
#include <QtMath>
#include <QRandomGenerator>
#include <QVector2D>
#include <QMatrix4x4>
#include <QQuaternion>

SatelliteModel::SatelliteModel(QObject *parent) : QAbstractListModel(parent)
{
    // 初始加载 15 个预设轨道，分三类：低轨 / 中高轨 / 高轨
    // 每类 5 条轨道，三种颜色，默认不显示（由前端开关控制）

    const QColor leoColor("#00C853");   // 低轨：绿色
    const QColor meoColor("#3B82F6");   // 中高轨：蓝色
    const QColor geoColor("#F59E0B");   // 高轨：橙色

    const double leoA = 12000.0;   // LEO 视觉调整半径
    const double meoA = 26560.0;   // MEO 真实半径
    const double geoA = 42164.0;   // GEO 同步轨道半径

    const double e = 0.0;
    const double i = 55.0;
    const double w = 0.0;
    const double m0 = 0.0;
    const double n = 500.0;

    for (int k = 0; k < 5; ++k) {
        const double raan = k * (360.0 / 5.0);
        addSatellite(leoA, e, i, raan, w, m0, n, leoColor, false);
    }
    for (int k = 0; k < 5; ++k) {
        const double raan = k * (360.0 / 5.0);
        addSatellite(meoA, e, i, raan, w, m0, n, meoColor, false);
    }
    for (int k = 0; k < 5; ++k) {
        const double raan = k * (360.0 / 5.0);
        addSatellite(geoA, e, i, raan, w, m0, n, geoColor, false);
    }

    // 干扰点（欺骗卫星信号落点）- 红色，位置固定在中国境内（北京附近）
    // 经度116.4°E，纬度39.9°N
    addGroundPoint(39.9, 116.4, "", JammerPoint, 500.0, QColor("#FF3333"));

    // 欺骗卫星：卫星-地心-北京三点共线
    // 卫星在北京的正上方，沿着北京到地心的方向延伸
    // 第一颗：北京上空 (116.4°E, 39.9°N)，高度26560km（中轨）
    addDeceptionSatellite(26560.0, 39.9, 116.4, QColor("#FF3333"));
    // 第二颗：备用位置
    addDeceptionSatellite(26560.0, 30.0, 100.0, QColor("#FF3333"));
}

void SatelliteModel::setVisualSymmetry(bool enabled)
{
    if (m_visualSymmetry == enabled) return;
    m_visualSymmetry = enabled;
    
    // 强制重新计算所有位置和轨道线
    updateSimulation(m_elapsedTime);
    invalidateOrbitCache();
    emit orbitPathsChanged();
    emit visualSymmetryChanged();
}

void SatelliteModel::setPaused(bool paused)
{
    if (m_paused == paused) return;
    m_paused = paused;
    emit pausedChanged();
}

void SatelliteModel::addSatellite(double a, double e, double i, double raan, double w, double m0, double n, QColor color, bool visible)
{
    beginInsertRows(QModelIndex(), m_satellites.size(), m_satellites.size());
    
    SatelliteData sat;
    sat.elements.semiMajorAxis = a;
    sat.elements.eccentricity = e;
    sat.elements.inclination = i;
    sat.elements.raan = raan;
    sat.elements.argPerigee = w;
    sat.elements.meanAnomaly = m0;
    sat.elements.meanMotion = n;
    sat.elements.color = color;
    sat.color = color;
    sat.visible = visible;
    sat.name = QString("SAT-%1").arg(m_satellites.size() + 1);
    sat.position = OrbitalPhysics::calculatePosition(sat.elements, m_elapsedTime, m_visualSymmetry);
    
    m_satellites.append(sat);
    endInsertRows();

    invalidateOrbitCache();
    emit orbitPathsChanged();
    emit satelliteCountChanged();
}

void SatelliteModel::addDeceptionSatellite(double altitude, double lat, double lon, QColor color)
{
    beginInsertRows(QModelIndex(), m_satellites.size(), m_satellites.size());

    SatelliteData sat;
    sat.isDeception = true;
    sat.color = color;
    sat.visible = false;  // 默认不显示，由面板开关控制
    sat.name = QString("DECEPTION-%1").arg(m_satellites.size() - 14);  // 从1开始编号

    // 设置轨道参数，让轨道面经过北京上空
    // 轨道倾角 = 目标纬度，这样轨道面会经过目标点上空
    sat.elements.semiMajorAxis = altitude;
    sat.elements.eccentricity = 0.0;           // 圆轨道
    sat.elements.inclination = lat;            // 倾角 = 目标纬度
    sat.elements.raan = lon;                   // RAAN = 目标经度
    sat.elements.argPerigee = 0.0;
    sat.elements.meanAnomaly = 90.0;           // 从目标点开始
    sat.elements.meanMotion = 0.0;             // 不运动
    sat.elements.color = color;

    // 计算卫星位置：卫星-地心-目标点三点共线
    sat.position = OrbitalPhysics::llaToCartesian(lat, lon, altitude);

    // 设置经纬度
    sat.latitude = lat;
    sat.longitude = lon;

    m_satellites.append(sat);
    endInsertRows();

    invalidateOrbitCache();
    emit orbitPathsChanged();
    emit satelliteCountChanged();
}

void SatelliteModel::updateSatellite(int index, double a, double e, double i, double raan, double w, double m0, double n)
{
    if (index < 0 || index >= m_satellites.size()) return;

    auto &sat = m_satellites[index];
    sat.elements.semiMajorAxis = a;
    sat.elements.eccentricity = e;
    sat.elements.inclination = i;
    sat.elements.raan = raan;
    sat.elements.argPerigee = w;
    sat.elements.meanAnomaly = m0;
    sat.elements.meanMotion = n;
    
    // 重新计算位置和旋转（必要时）
    sat.position = OrbitalPhysics::calculatePosition(sat.elements, m_elapsedTime, m_visualSymmetry);
    
    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {PositionRole, RotationRole});
    
    invalidateOrbitCache();
    emit orbitPathsChanged();
}

void SatelliteModel::setColor(int index, QColor color)
{
    if (index < 0 || index >= m_satellites.size()) return;
    
    m_satellites[index].color = color;
    m_satellites[index].elements.color = color;
    
    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {ColorRole});
    
    invalidateOrbitCache();
    emit orbitPathsChanged();
}

QVariantMap SatelliteModel::getElements(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_satellites.size()) return map;

    const auto &sat = m_satellites[index];
    const auto &el = sat.elements;
    map["a"] = el.semiMajorAxis;
    map["e"] = el.eccentricity;
    map["i"] = el.inclination;
    map["raan"] = el.raan;
    map["w"] = el.argPerigee;
    map["m0"] = el.meanAnomaly;
    map["n"] = el.meanMotion;
    map["color"] = sat.color;
    return map;
}

void SatelliteModel::setVisible(int index, bool visible)
{
    if (index < 0 || index >= m_satellites.size()) return;
    if (m_satellites[index].visible == visible) return;

    m_satellites[index].visible = visible;
    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {VisibleRole});

    invalidateOrbitCache();
    emit orbitPathsChanged();
}

void SatelliteModel::setSignalActive(int index, bool active)
{
    if (index < 0 || index >= m_satellites.size()) return;
    if (m_satellites[index].signalActive == active) return;

    m_satellites[index].signalActive = active;
    QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {SignalActiveRole});
}

// 批量设置可见性，只发射一次信号（优化性能）
void SatelliteModel::setVisibilityBatch(QVariantList indices, QVariantList visibles)
{
    if (indices.size() != visibles.size()) return;
    if (indices.isEmpty()) return;
    
    bool anyChanged = false;
    int minIdx = m_satellites.size();
    int maxIdx = -1;
    
    for (int i = 0; i < indices.size(); ++i) {
        int idx = indices[i].toInt();
        bool vis = visibles[i].toBool();
        
        if (idx < 0 || idx >= m_satellites.size()) continue;
        if (m_satellites[idx].visible == vis) continue;
        
        m_satellites[idx].visible = vis;
        anyChanged = true;
        minIdx = qMin(minIdx, idx);
        maxIdx = qMax(maxIdx, idx);
    }
    
    if (anyChanged) {
        // 发射一次 dataChanged 覆盖所有改变的行
        if (maxIdx >= 0) {
            emit dataChanged(createIndex(minIdx, 0), createIndex(maxIdx, 0), {VisibleRole});
        }
        invalidateOrbitCache();
        emit orbitPathsChanged();
    }
}

bool SatelliteModel::isVisible(int index) const
{
    if (index < 0 || index >= m_satellites.size()) return false;
    return m_satellites[index].visible;
}

void SatelliteModel::clear()
{
    const bool wasReal = m_realOrbitMode;
    m_realOrbitMode = false;
    m_frameWeek = 0;
    m_frameSecondsOfWeek = 0.0;
    m_frameSimT0 = 0.0;
    m_lastSubpointSync = -1.0;
    m_visibilityEpoch++;
    if (wasReal)
        emit satellitesVisibilityChanged();

    beginResetModel();
    m_satellites.clear();
    endResetModel();

    m_groundPoints.clear();

    invalidateOrbitCache();
    emit orbitPathsChanged();
    emit satelliteCountChanged();
    emit groundPointsChanged();
    if (wasReal)
        emit realOrbitModeChanged();
}

bool SatelliteModel::isSatelliteVisible(int index) const
{
    if (index < 0 || index >= m_satellites.size())
        return false;
    return m_satellites[index].visible;
}

void SatelliteModel::updateDayRate(const OrbitMessage::Frame &frame)
{
    // 由相邻两帧推算"报文日时间/真实时间"倍率：
    // 回放一天(86400 s)用 120 s 时 dayRate≈720，渲染按该倍率快进，
    // 卫星在帧间沿开普勒弧连续移动，而不是每帧跳一步。
    //
    // TCP 会把匀速发送的帧合包（到达间隔剧烈抖动），直接用瞬时到达间隔
    // 会让倍率乱跳、外推位置过冲（GEO 曾偏 39°）。因此：
    //   1) 瞬时倍率经 EMA 平滑；
    //   2) EMA 结果限幅在上一次的 [1/4, 4] 倍，抑制突发；
    //   3) 渲染外推量按"不超过约两帧的日时间"钳制（updateSimulation）。
    const double prevTow = m_frameSecondsOfWeek;
    const double prevArrival = m_frameSimT0;
    if (!m_realOrbitMode) {
        m_dayRate = 1.0;    // 首帧无历史，先按实时，第二帧起校准
        m_rateEmaInit = false;
        return;
    }
    double dayDelta = frame.secondsOfWeek - prevTow;
    if (dayDelta < -302400.0)
        dayDelta += 604800.0;
    else if (dayDelta > 302400.0)
        dayDelta -= 604800.0;
    const double wallDelta = m_elapsedTime - prevArrival;
    if (wallDelta > 0.02 && dayDelta > 0.0 && dayDelta <= 7200.0) {
        const double instant = dayDelta / wallDelta;
        if (!m_rateEmaInit) {
            m_rateEma = instant;
            m_rateEmaInit = true;
        } else {
            m_rateEma = 0.75 * m_rateEma + 0.25 * instant;
        }
        m_dayRate = qBound(m_rateEma * 0.25, m_rateEma, m_rateEma * 4.0);
        m_lastDayDelta = dayDelta;
    }
    // 其余情况（暂停后恢复/乱序/倒放）保持上一次的倍率
}

void SatelliteModel::applyOrbitFrame(const OrbitMessage::Frame &frame)
{
    if (frame.satellites.isEmpty())
        return;

    // ===== 快路径：卫星集合与轨道形状都没变（白天文件回放的常态，
    // 每帧只是历元/M0 推进）→ 原地更新，QML 不重建 delegate、
    // 轨道环不重新采样。轨道环是完整周期采样，形状与 M0 无关。=====
    bool sameSet = m_realOrbitMode
                   && m_orbCount == frame.satellites.size();
    bool shapeChanged = false;
    if (sameSet) {
        for (int i = 0; i < frame.satellites.size(); ++i) {
            const OrbitMessage::Satellite &src = frame.satellites.at(i);
            const SatelliteData &dst = m_satellites.at(i);
            if (dst.name != src.displayName()) {
                sameSet = false;
                break;
            }
            if (dst.elements.semiMajorAxis != src.semiMajorAxisKm
                || dst.elements.eccentricity != src.eccentricity
                || dst.elements.inclination != src.inclinationDeg
                || dst.elements.raan != src.raanDeg
                || dst.elements.argPerigee != src.argumentOfPerigeeDeg) {
                shapeChanged = true;
            }
        }
    }
    if (sameSet) {
        for (int i = 0; i < frame.satellites.size(); ++i) {
            const OrbitMessage::Satellite &src = frame.satellites.at(i);
            SatelliteData &sat = m_satellites[i];
            sat.elements.meanAnomaly =
                OrbitMath::meanToTrueAnomalyDeg(src.meanAnomalyDeg, src.eccentricity);
            sat.health = src.health;
        }
        updateDayRate(frame);
        m_frameWeek = frame.week;
        m_frameSecondsOfWeek = frame.secondsOfWeek;
        m_frameSimT0 = m_elapsedTime;
        if (shapeChanged) {
            invalidateOrbitCache();
            emit orbitPathsChanged();
        }
        emit dataChanged(createIndex(0, 0),
                         createIndex(m_satellites.size() - 1, 0),
                         {PositionRole, RotationRole, LatitudeRole, LongitudeRole});
        updateSimulation(m_elapsedTime);
        return;
    }

    // $DSP 欺骗卫星与 $ORB 星座独立，重建星座时必须保留
    QVector<SatelliteData> deceptions;
    for (const SatelliteData &d : m_satellites)
        if (d.isDeception) deceptions.append(d);

    beginResetModel();
    m_satellites.clear();
    m_satellites.reserve(frame.satellites.size() + deceptions.size());

    for (const OrbitMessage::Satellite &src : frame.satellites) {
        SatelliteData sat;
        sat.elements.semiMajorAxis = src.semiMajorAxisKm;
        sat.elements.eccentricity = src.eccentricity;
        sat.elements.inclination = src.inclinationDeg;
        sat.elements.raan = src.raanDeg;
        sat.elements.argPerigee = src.argumentOfPerigeeDeg;
        sat.elements.meanAnomaly = src.meanAnomalyDeg;
        sat.elements.meanMotion = src.meanMotionRevPerDay;
        sat.meanMotionRevPerDay = src.meanMotionRevPerDay;
        sat.health = src.health;
        sat.name = src.displayName();
        sat.visible = (src.health == 0);   // 只显示该分钟参与定位的卫星
        sat.visualScale = 0.7;             // 符号化标记：1.0 偏大，0.7 兼顾可读与比例

        // 分系统配色（不健康卫星用灰色，便于现场一眼识别）
        if (src.health != 0) {
            sat.color = QColor("#8A8A8A");
        } else if (src.system == 'G') {
            sat.color = QColor("#3B82F6");   // GPS 蓝
        } else if (src.system == 'C') {
            sat.color = QColor("#F59E0B");   // 北斗 橙
        } else {
            sat.color = QColor("#A78BFA");   // Galileo 紫
        }
        sat.elements.color = sat.color;

        sat.elements.meanAnomaly =
            OrbitMath::meanToTrueAnomalyDeg(src.meanAnomalyDeg, src.eccentricity);

        m_satellites.append(sat);
    }
    for (const SatelliteData &d : deceptions)
        m_satellites.append(d);
    endResetModel();

    m_orbCount = frame.satellites.size();
    m_realOrbitMode = true;
    updateDayRate(frame);
    m_frameWeek = frame.week;
    m_frameSecondsOfWeek = frame.secondsOfWeek;
    m_frameSimT0 = m_elapsedTime;
    m_lastSubpointSync = -1.0;

    invalidateOrbitCache();
    emit orbitPathsChanged();
    emit satelliteCountChanged();
    emit realOrbitModeChanged();

    // 立即按新历元算一帧，避免界面出现"跳一下"
    updateSimulation(m_elapsedTime);
}

int SatelliteModel::deceptionCount() const
{
    int n = 0;
    for (const SatelliteData &s : m_satellites)
        if (s.isDeception) ++n;
    return n;
}

void SatelliteModel::applyDeceptionFrame(
    const QVector<OrbitMessage::DeceptionEntry> &entries)
{
    // $DSP 语义：全量声明当前激活的欺骗干扰源。
    // 现有 $DSP 卫星中不在本帧里的 → 移除（干扰演示结束）。
    bool changed = false;

    // 更新/新增
    for (const OrbitMessage::DeceptionEntry &e : entries) {
        const QString name = QStringLiteral("DSP-%1").arg(e.index);
        SatelliteData *existing = nullptr;
        for (SatelliteData &s : m_satellites) {
            if (s.isDeception && s.name == name) { existing = &s; break; }
        }
        const QColor color = QColor("#FF3333");
        if (!existing) {
            SatelliteData sat;
            sat.isDeception = true;
            sat.name = name;
            sat.color = color;
            sat.elements.color = color;
            sat.elements.semiMajorAxis = e.altKm;
            sat.elements.eccentricity = 0.0;
            sat.elements.inclination = 0.0;
            sat.elements.raan = e.lonDeg;       // 复用字段供轨道环生成

            beginInsertRows(QModelIndex(), m_satellites.size(), m_satellites.size());
            m_satellites.append(sat);
            endInsertRows();
            existing = &m_satellites.last();
            changed = true;
        }
        // 位置与状态（场景轴约定: llaToCartesian 直接给场景坐标）
        existing->position = OrbitalPhysics::llaToCartesian(
            e.latDeg, e.lonDeg, e.altKm);
        existing->latitude = e.latDeg;
        existing->longitude = e.lonDeg;
        existing->signalActive = e.active;
        existing->visible = e.active;       // 波束开 = 卫星显示
        existing->visualScale = 1.2;
        changed = true;
    }

    // 移除不在本帧中的
    for (int i = m_satellites.size() - 1; i >= 0; --i) {
        const SatelliteData &s = m_satellites[i];
        if (!s.isDeception) continue;
        bool inFrame = false;
        for (const OrbitMessage::DeceptionEntry &e : entries)
            if (e.index == s.name.mid(4).toInt()) { inFrame = true; break; }
        if (!inFrame) {
            beginRemoveRows(QModelIndex(), i, i);
            m_satellites.removeAt(i);
            endRemoveRows();
            changed = true;
        }
    }

    if (changed) {
        invalidateOrbitCache();
        emit orbitPathsChanged();
        emit dataChanged(createIndex(0, 0),
                         createIndex(qMax(0, m_satellites.size() - 1), 0),
                         {PositionRole, VisibleRole, ColorRole});
        ++m_visibilityEpoch;
        emit satellitesVisibilityChanged();
    }
}

void SatelliteModel::syncSubpointsToPnt(double simTime)
{
    if (!m_realOrbitMode || !m_pntModel)
        return;
    // 1 Hz 同步即可：2D 地图的星下点与 3D 同一套解算，保证一一对应
    if (m_lastSubpointSync >= 0.0 && simTime - m_lastSubpointSync < 1.0)
        return;
    m_lastSubpointSync = simTime;

    QStringList prns;
    QVector<double> lats;
    QVector<double> lons;
    prns.reserve(m_satellites.size());
    lats.reserve(m_satellites.size());
    lons.reserve(m_satellites.size());
    for (const SatelliteData &sat : m_satellites) {
        if (!sat.visible)
            continue;   // 只同步参与定位的卫星（与 3D 显示一致）
        prns.append(sat.name);
        lats.append(sat.latitude);
        lons.append(sat.longitude);
    }
    if (!prns.isEmpty()) {
        m_pntModel->updateFromSat(prns.size(), prns, lats, lons);

        // 2D/3D 一致性自检：首次或卫星数变化时记录全部 (PRN, lat, lon)
        static int lastCount = -1;
        if (prns.size() != lastCount) {
            lastCount = prns.size();
            QString detail;
            for (int i = 0; i < prns.size(); ++i)
                detail += QStringLiteral(" %1:%2,%3").arg(prns[i])
                              .arg(lats[i], 0, 'f', 1).arg(lons[i], 0, 'f', 1);
            SCENE_LOG_CAT("ORB", QString("subpoints synced n=%1 tow=%2:%3")
                .arg(prns.size())
                .arg(m_frameSecondsOfWeek, 0, 'f', 0).arg(detail));
        }
    }
}

void SatelliteModel::updateSimulation(double time)
{
    // 暂停时跳过计算
    if (m_paused) return;

    m_elapsedTime = time; // 使用外部传入的累计秒数

    // ===== 真实轨道模式：$ORB 报文给出的开普勒根数 + 二体推进 =====
    if (m_realOrbitMode) {
        // 按报文日时间推进（回放时 dayRate>1，一天被压缩到几分钟）。
        // 外推量钳制在约两帧的日时间内：即使倍率抖动，位置也不会甩飞
        double elapsed = (m_elapsedTime - m_frameSimT0) * m_dayRate;
        const double maxElapsed = m_lastDayDelta * 2.0;
        if (elapsed > maxElapsed)
            elapsed = maxElapsed;
        else if (elapsed < -maxElapsed)
            elapsed = -maxElapsed;
        // 广播星历根数(Ω0 已吸收 GMST 偏置)经二体推进的输出即 ECEF——
        // 不能再做 GMST 旋转(重复扣地球自转, 曾致星下点错位 196°)。
        // 卫星始终出现在接收站半球, 与 2D 星下点一一对应。
        for (SatelliteData &sat : m_satellites) {
            if (sat.isDeception) continue;   // 欺骗星由 $DSP 管理位置/可见性
            sat.visible = (sat.health == 0);   // 只显示参与定位的卫星
            OrbitMath::OrbitalElements el;
            el.semiMajorAxisKm = sat.elements.semiMajorAxis;
            el.eccentricity = sat.elements.eccentricity;
            el.inclinationDeg = sat.elements.inclination;
            el.raanDeg = sat.elements.raan;
            el.argumentOfPerigeeDeg = sat.elements.argPerigee;
            el.trueAnomalyDeg = sat.elements.meanAnomaly;   // 见 applyOrbitFrame 的 M0→ν 转换
            try {
                if (sat.isDeception) continue;
                const OrbitMath::StateVector sv = OrbitMath::propagateKepler(el, elapsed);
                // GPS 广播约定：帧根数的 Ω0 已对齐"周起始"，广播算法烧进
                // -Ωe×tow(周内秒) —— 实测 tow=0 帧位置即真值、 tow>0 帧多转
                // +Ωe×tow。因此渲染 ECEF = 惯性推进位置绕地轴(Z)转
                // -Ωe×tow_render(tow_render = 帧 tow + 日时间外推)。
                // 验证：GEO 帧时刻定点不动、外推期净转 ≈0；MEO 净转 (n-Ωe)Δ ✓
                const double towRender = m_frameSecondsOfWeek + elapsed;
                const double spinRad =
                    -OrbitMath::EarthRotationRateRadPerSecond * towRender;
                const OrbitMath::Vector3 ecef =
                    OrbitMath::rotateZ(sv.positionKm, spinRad);
                const OrbitMath::GeodeticPosition geo = OrbitMath::ecefToGeodetic(ecef);
                sat.latitude = geo.latitudeDeg;
                sat.longitude = geo.longitudeDeg;
                // 渲染：标准 ECEF → 场景轴约定(λ=0→+Z, Y=北极)：scene=(y,z,x)
                sat.position = CoordinateSystem::sceneFromEcefStd(
                    QVector3D(float(ecef.x), float(ecef.y), float(ecef.z)));
            } catch (const std::exception &) {
                continue;   // 参数非法：保持上一帧位置，不崩溃
            }

            const QVector3D direction = -sat.position.normalized();
            sat.rotation = QQuaternion::fromDirection(direction, QVector3D(0, 1, 0)).toEulerAngles();
            sat.visualScale = 0.7;         // 符号化标记，见 applyOrbitFrame 注释
        }

        if (!m_satellites.isEmpty()) {
            emit dataChanged(createIndex(0, 0), createIndex(m_satellites.size() - 1, 0),
                             {PositionRole, RotationRole, LongitudeRole, LatitudeRole,
                              PhaseRole, ScaleRole});
        }
        syncSubpointsToPnt(time);
        return;
    }

    // 计算地球当前的旋转角度 (60秒一圈)
    double earthRotation = fmod((m_elapsedTime / 60.0) * 360.0, 360.0);

    for (int i = 0; i < m_satellites.size(); ++i) {
        SatelliteData &sat = m_satellites[i];

        // 欺骗卫星位置已经在addDeceptionSatellite中设置好，不需要重新计算
        if (!sat.isDeception) {
            // 普通卫星：使用轨道参数计算位置
            sat.position = OrbitalPhysics::calculatePosition(sat.elements, m_elapsedTime, m_visualSymmetry);

            // 计算经纬度
            double lat_eci, lon_eci;
            OrbitalPhysics::cartesianToLla(sat.position, lat_eci, lon_eci);

            sat.latitude = lat_eci;
            sat.longitude = lon_eci - earthRotation;

            // 归一化经度到 [-180, 180]
            while (sat.longitude > 180) sat.longitude -= 360;
            while (sat.longitude < -180) sat.longitude += 360;
        }
        // 欺骗卫星的经纬度已经在addDeceptionSatellite中设置，保持不变

        // 计算旋转（所有卫星都需要朝向地心）
        QVector3D direction;
        direction = -sat.position.normalized();

        QQuaternion rotation = QQuaternion::fromDirection(direction, QVector3D(0, 1, 0));
        sat.rotation = rotation.toEulerAngles();

        // 动态计算视觉缩放
        sat.visualScale = 1.5;
    }

    // 干扰机数据现在由JAM报文提供，不再由模拟更新

    if (!m_satellites.isEmpty()) {
        QModelIndex topLeft = createIndex(0, 0);
        QModelIndex bottomRight = createIndex(m_satellites.size() - 1, 0);
        emit dataChanged(topLeft, bottomRight, {PositionRole, RotationRole, LongitudeRole, LatitudeRole, PhaseRole, ScaleRole});
    }
}

void SatelliteModel::addGroundPoint(double lat, double lon, QString name, int type, double radius, QColor color, double errorRadius)
{
    QVariantMap point;
    // 地球半径 6371 km，标记抬升到 GROUND_MARKER_ALTITUDE(6391 km) 避免被球面遮挡
    QVector3D pos = OrbitalPhysics::llaToCartesian(
        lat, lon, CoordinateSystem::GROUND_MARKER_ALTITUDE); 
    point["position"] = pos;
    point["lat"] = lat;
    point["lon"] = lon;
    point["name"] = name;
    point["type"] = type;
    point["radius"] = radius;
    point["color"] = color;
    point["errorRadius"] = errorRadius;
    
    m_groundPoints.append(point);
    emit groundPointsChanged();
}

void SatelliteModel::updateGroundPoint(int index, double lat, double lon, QString name, int type, double radius, QColor color, double errorRadius)
{
    if (index < 0 || index >= m_groundPoints.size()) return;

    QVariantMap point = m_groundPoints[index].toMap();
    QVector3D pos = OrbitalPhysics::llaToCartesian(
        lat, lon, CoordinateSystem::GROUND_MARKER_ALTITUDE);
    point["position"] = pos;
    point["lat"] = lat;
    point["lon"] = lon;
    point["name"] = name;
    point["type"] = type;
    point["radius"] = radius;
    point["color"] = color;
    point["errorRadius"] = errorRadius;

    m_groundPoints[index] = point;
    emit groundPointsChanged();
}

QVariantMap SatelliteModel::getGroundPoint(int index) const
{
    if (index < 0 || index >= m_groundPoints.size()) return QVariantMap();
    return m_groundPoints[index].toMap();
}

void SatelliteModel::removeGroundPoint(int index)
{
    if (index < 0 || index >= m_groundPoints.size()) return;
    m_groundPoints.removeAt(index);
    emit groundPointsChanged();
}

int SatelliteModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_satellites.size();
}

QVariant SatelliteModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_satellites.size())
        return QVariant();

    const auto &sat = m_satellites[index.row()];
    switch (role) {
    case PositionRole: return sat.position;
    case RotationRole: return sat.rotation;
    case ColorRole: return sat.color;
    case VisibleRole: return sat.visible;
    case LongitudeRole: return sat.longitude;
    case LatitudeRole: return sat.latitude;
    case NameRole: return sat.name;
    case PhaseRole: return (int)sat.phase;
    case ScaleRole: return sat.visualScale;
    case IsDeceptionRole: return sat.isDeception;
    case SignalActiveRole: return sat.signalActive;
    default: return QVariant();
    }
}

QHash<int, QByteArray> SatelliteModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PositionRole] = "position";
    roles[RotationRole] = "eulerRotation";
    roles[ColorRole] = "color";
    roles[VisibleRole] = "visible";
    roles[LongitudeRole] = "longitude";
    roles[LatitudeRole] = "latitude";
    roles[NameRole] = "name";
    roles[PhaseRole] = "phase";
    roles[ScaleRole] = "visualScale";
    roles[IsDeceptionRole] = "isDeception";
    roles[SignalActiveRole] = "signalActive";
    return roles;
}

QVariantList SatelliteModel::orbitPaths() const
{
    // 使用缓存，避免每帧重新计算
    if (m_orbitPathsCacheValid) {
        return m_cachedOrbitPaths;
    }

    if (m_realOrbitMode) {
        // 真实轨道：在惯性系采样一个完整周期（闭合椭圆）。
        // 不在地固系采样，否则轨道环会变成不闭合的弧段。
        QVariantList realList;
        for (const SatelliteData &sat : m_satellites) {
            // 真实模式下集合固定（全天并集），所有卫星都画环：
            // 不参与定位的卫星本体灰显，环保持同色，便于看出它的完整轨道
            {
            QVariantMap pathData;
            QVariantList points;

            OrbitMath::OrbitalElements el;
            el.semiMajorAxisKm = sat.elements.semiMajorAxis;
            el.eccentricity = sat.elements.eccentricity;
            el.inclinationDeg = sat.elements.inclination;
            el.raanDeg = sat.elements.raan;
            el.argumentOfPerigeeDeg = sat.elements.argPerigee;
            el.trueAnomalyDeg = sat.elements.meanAnomaly;

            try {
                const double a = el.semiMajorAxisKm;
                const double period = 2.0 * M_PI
                    * std::sqrt((a * a * a) / OrbitMath::EarthMuKm3PerSecond2);
                // 环保持 ECI 采样（缓存一次），地固朝向由 QML 侧整体绕
                // 地轴旋转 -gmstDeg 实现（GMST 推进零重建），与卫星 ECEF
                // 渲染位置始终重合
                const int segments = 96;
                for (int k = 0; k <= segments; ++k) {
                    const double t = period * double(k) / segments;
                    const OrbitMath::StateVector sv = OrbitMath::propagateKepler(el, t);
                    // 环 = "帧时刻的轨道在地固系"（与卫星同一语义），场景轴映射
                    points.append(CoordinateSystem::sceneFromEcefStd(
                        QVector3D(float(sv.positionKm.x), float(sv.positionKm.y),
                                  float(sv.positionKm.z))));
                }
            } catch (const std::exception &) {
                // 参数非法：给空轨道，避免渲染垃圾
            }

            pathData["points"] = points;
            pathData["color"] = sat.color;
            pathData["thickness"] = CoordinateSystem::ORBIT_THICKNESS_KM;
            realList.append(pathData);
            }
        }
        m_cachedOrbitPaths = realList;
        m_orbitPathsCacheValid = true;
        return realList;
    }

    QVariantList list;
    for (const auto &sat : m_satellites) {
        if (!sat.visible) {
            list.append(QVariant());
            continue;
        }

        QVariantMap pathData;
        QVariantList points;

        if (sat.isDeception) {
            // 欺骗卫星：生成经过卫星位置的圆形轨道
            // 卫星位置 = 北京方向，轨道必须经过这个点
            double lat = sat.latitude;
            double lon = sat.longitude;
            double altitude = sat.elements.semiMajorAxis;

            // 北京方向单位向量（从地心指向北京/卫星）
            double latRad = qDegreesToRadians(lat);
            double lonRad = qDegreesToRadians(lon);
            QVector3D beijingDir(qCos(latRad) * qSin(lonRad), qSin(latRad), qCos(latRad) * qCos(lonRad));

            // 卫星位置
            QVector3D satPos = altitude * beijingDir;

            // 轨道面法向量：选择一个垂直于北京方向的向量作为法向量
            // 这样轨道面就会经过北京方向（卫星位置）
            // 使用 Y 轴和北京方向的叉积作为法向量（如果不平行的话）
            QVector3D up(0, 1, 0);
            QVector3D orbitNormal = QVector3D::crossProduct(up, beijingDir).normalized();

            // 如果北京方向接近 Y 轴，使用 X 轴
            if (orbitNormal.length() < 0.1) {
                QVector3D right(1, 0, 0);
                orbitNormal = QVector3D::crossProduct(right, beijingDir).normalized();
            }

            // 在轨道面上创建两个正交向量
            QVector3D tangent1 = beijingDir;  // 轨道经过北京方向
            QVector3D tangent2 = QVector3D::crossProduct(orbitNormal, tangent1).normalized();

            // 生成圆形轨道点（48段），从卫星位置开始
            // 卫星位置在 angle=0 处
            for (int i = 0; i <= 48; ++i) {
                double angle = 2.0 * M_PI * i / 48.0;
                QVector3D pt = altitude * (qCos(angle) * tangent1 + qSin(angle) * tangent2);
                points.append(pt);
            }

            pathData["color"] = QColor("#FF6666");
            pathData["thickness"] = CoordinateSystem::DECEPTION_ORBIT_THICKNESS_KM;
        } else {
            // 普通卫星：使用轨道参数计算
            auto pts = OrbitalPhysics::generateOrbitPoints(sat.elements, 48, m_visualSymmetry);
            for (const auto &p : pts) {
                points.append(p);
            }
            pathData["color"] = sat.color;
            pathData["thickness"] = CoordinateSystem::ORBIT_THICKNESS_KM;
        }

        pathData["points"] = points;
        list.append(pathData);
    }

    // 更新缓存
    m_cachedOrbitPaths = list;
    m_orbitPathsCacheValid = true;

    return list;
}
