#include "SatelliteModel.h"
#include "PntDataModel.h"
#include "CoordinateSystem.h"
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
    beginResetModel();
    m_satellites.clear();
    endResetModel();

    m_groundPoints.clear();

    invalidateOrbitCache();
    emit orbitPathsChanged();
    emit satelliteCountChanged();
    emit groundPointsChanged();
}

void SatelliteModel::updateSimulation(double time)
{
    // 暂停时跳过计算
    if (m_paused) return;

    m_elapsedTime = time; // 使用外部传入的累计秒数

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
