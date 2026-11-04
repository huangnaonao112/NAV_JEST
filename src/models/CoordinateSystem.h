#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QVector>
#include <QtMath>
#include <cmath>
#include <algorithm>

/**
 * @brief 场景坐标与尺度的唯一事实来源（Single Source of Truth）
 *
 * ============================================================================
 * 一、本场景有三套坐标空间，混用即出错
 * ============================================================================
 *
 * 1) 天体局部空间（Body-Local Space）—— 比例 1 场景单位 = 1 km
 *    - 地球、月球、卫星轨道环、地面点、月面轨迹都活在这里
 *    - 依据：Qt Quick 3D 内置图元 #Sphere 的基准半径是 50 单位
 *      （直径 100），所以节点 scale = 目标半径(km) / 50
 *      地球：6371 / 50 = 127.42  → 场景半径 6371 单位 = 6371 km
 *      月球：1737 / 50 =  34.74  → 场景半径 1737 单位 = 1737 km
 *    - 卫星轨道环的点是 km（半长轴 a），直接喂给几何体，不再缩放
 *
 * 2) 地月压缩空间（Earth-Moon Display Space）—— 比例 1 场景单位 = 4.805 km
 *    - 只用于"地月转移轨迹"（$TRJ 报文的 ECEF km）与月球整体摆位
 *    - 为什么压缩：真实地月距离 384400 km，若按 1:1 摆放，月球在 384400
 *      单位外，地心视角下不可见。压缩到 80000 单位后能与地球同屏显示
 *    - 压缩比 = MOON_SCENE_X / EARTH_MOON_DISTANCE_KM ≈ 0.2081
 *    - 注意：月球"本体大小"仍是 1:1（半径 1737 单位），只有"位置"被压缩，
 *      这是本项目有意的非物理展示取舍，不是 bug
 *
 * 3) 报文坐标（Telemetry Space）—— 设备方给的原始单位
 *    - $TRJ：ECEF/ECI 三轴 km，经 distanceScale 进入空间 2
 *    - $MNT：月面经纬度(deg) + 高度(km)，经 llaToCartesian 进入空间 1
 *    - $PNT/$SAT/$JAM：经纬度(deg)，经纬度本身与比例无关
 *
 * ============================================================================
 * 二、线宽/标记尺寸的单位契约
 * ============================================================================
 *
 * TrajectoryLineGeometry.thickness、GroundWarningItem.radius 一律使用
 * **其轨迹点所在空间的单位**：
 *   - 轨道环（空间 1）        → km，见 ORBIT_THICKNESS_KM
 *   - 回放轨迹（空间 2）      → 空间 2 单位，见 REPLAY_THICKNESS_UNITS
 *   - 月面轨迹（空间 1 局部） → km，见 MOON_TRAIL_THICKNESS_KM
 *
 * 历史教训：曾出现轨道环传 0.4（应 40 km）、月面线传 30 但点被 ×0.01
 * 的错配，前者细成亚像素不可见、后者粗成 3000 km 巨带。今后**禁止在
 * QML 里写字面量**，一律引用本类常量（有 ctest 静态检查兜底）。
 *
 * ============================================================================
 * 三、新增渲染物时的检查清单
 * ============================================================================
 *   [ ] 这个物体的点坐标属于哪套空间？（1 天体局部 / 2 地月压缩）
 *   [ ] 节点 scale 是否由本类常量推导，而不是手写 127.42 / 34.74？
 *   [ ] thickness / radius 的单位是否与点坐标一致？
 *   [ ] 跨空间是否显式调用 toDisplaySpace() / toLocalKm()？
 *   [ ] qml_unit_lint ctest 是否通过（禁止字面量）？
 *
 * 详细文档：docs/坐标系与尺度规范.md
 */
class CoordinateSystem : public QObject {
    Q_OBJECT
    // 注册方式：main.cpp 用 qmlRegisterSingletonInstance 以 "CoordinateSystem"
    // 名称挂到 NavModels 模块（见该文件第 ~202 行）。因此这里**不能**再加
    // QML_SINGLETON/QML_ELEMENT，否则同一 URI 下会重复注册导致 QML 启动失败。

    // ---- 真实物理参数（km）----
    Q_PROPERTY(double earthRadiusKm READ earthRadius CONSTANT)
    Q_PROPERTY(double moonRadiusKm READ moonRadius CONSTANT)
    Q_PROPERTY(double earthMoonDistanceKm READ earthMoonDistance CONSTANT)

    // ---- 空间 1：天体局部节点缩放（scale = 目标半径 / #Sphere 基准半径）----
    Q_PROPERTY(double spherePrimitiveRadius READ spherePrimitiveRadius CONSTANT)
    Q_PROPERTY(double earthNodeScale READ earthNodeScale CONSTANT)
    Q_PROPERTY(double moonNodeScale READ moonNodeScale CONSTANT)
    Q_PROPERTY(double atmosphereNodeScale READ atmosphereNodeScale CONSTANT)

    // ---- 空间 2：地月压缩 ----
    Q_PROPERTY(double moonSceneX READ moonSceneX CONSTANT)
    Q_PROPERTY(double distanceScale READ distanceScale CONSTANT)

    // ---- 线宽（单位随所属空间）----
    Q_PROPERTY(double orbitThicknessKm READ orbitThicknessKm CONSTANT)
    Q_PROPERTY(double deceptionOrbitThicknessKm READ deceptionOrbitThicknessKm CONSTANT)
    Q_PROPERTY(double replayThicknessUnits READ replayThicknessUnits CONSTANT)
    Q_PROPERTY(double moonTrailThicknessKm READ moonTrailThicknessKm CONSTANT)
    Q_PROPERTY(double defaultCameraDistance READ defaultCameraDistance CONSTANT)
    Q_PROPERTY(double moonViewCameraDistance READ moonViewCameraDistance CONSTANT)
    Q_PROPERTY(double markerRefDistanceKm READ markerRefDistanceKm CONSTANT)
    Q_PROPERTY(double markerScaleExponent READ markerScaleExponent CONSTANT)
    Q_PROPERTY(double markerScaleMin READ markerScaleMin CONSTANT)
    Q_PROPERTY(double markerScaleMax READ markerScaleMax CONSTANT)

    // ---- 表面标记 ----
    Q_PROPERTY(double groundMarkerAltitudeKm READ groundMarkerAltitudeKm CONSTANT)
    Q_PROPERTY(double moonTrailRadiusKm READ moonTrailRadiusKm CONSTANT)

public:
    explicit CoordinateSystem(QObject *parent = nullptr) : QObject(parent) {}

    static CoordinateSystem* instance();

    // ================= 空间 1：天体局部空间（1 单位 = 1 km）=================
    static constexpr double SPHERE_PRIMITIVE_RADIUS = 50.0;   // Qt #Sphere 基准半径
    static constexpr double EARTH_RADIUS = 6371.0;            // km（WGS-84 平均半径）
    static constexpr double MOON_RADIUS = 1737.4;             // km（真实平均半径）
    static constexpr double MOON_RADIUS_DISPLAY = 1737.0;     // 显示用整数值（= 34.74 × 50）

    // 节点缩放：scale × #Sphere 基准半径 = 场景半径(单位)，且 1 单位 = 1 km
    static constexpr double EARTH_NODE_SCALE = EARTH_RADIUS / SPHERE_PRIMITIVE_RADIUS;   // 127.42
    static constexpr double MOON_NODE_SCALE = MOON_RADIUS_DISPLAY / SPHERE_PRIMITIVE_RADIUS; // 34.74

    // 地表标记抬升：略高于地球半径，避免被球面 z-fight 吃掉
    static constexpr double GROUND_MARKER_ALTITUDE = 6391.0;  // km

    // ================= 卫星标记尺寸（屏幕准恒定）=================
    // 卫星是符号化标记（真实卫星 ~10m，按真实尺寸不可见）。
    // 透视会让近轨卫星巨大、远轨卫星不可见；按相机距离做部分补偿：
    //   distScale = clamp((camDist / REF)^EXP, MIN, MAX)
    // EXP=1 为完全补偿（全部同大、无深度感）；0.75 使 GEO/MEO 远端
    // 屏幕大小差 <5%，同时近掠过被下限钳住（防贴脸）、保留少量深度感。
    static constexpr double MARKER_REF_DISTANCE_KM = 26000.0;  // 参考：MEO 轨道半径
    static constexpr double MARKER_SCALE_EXPONENT = 0.75;
    static constexpr double MARKER_SCALE_MIN = 0.25;
    static constexpr double MARKER_SCALE_MAX = 2.0;

    // 大气辉光层：地球半径 + 50 km，仅视觉用途
    static constexpr double ATMOSPHERE_RADIUS_KM = 6421.0;    // km
    static constexpr double ATMOSPHERE_NODE_SCALE =
        ATMOSPHERE_RADIUS_KM / SPHERE_PRIMITIVE_RADIUS;       // 128.42

    // 月面轨迹点半径：略高于月面，保证可见（与 GroundWarningItem 同口径）
    static constexpr double MOON_TRAIL_RADIUS = 1740.0;       // km

    // ================= 空间 2：地月压缩空间（1 单位 = 4.805 km）=================
    static constexpr double EARTH_MOON_DISTANCE_REAL = 384400.0;  // km
    static constexpr double MOON_SCENE_X = 80000.0;               // 月球显示位置（单位）
    // 压缩比：真实 km → 空间 2 单位。仅用于地月转移轨迹/月球摆位
    static constexpr double DISTANCE_SCALE = MOON_SCENE_X / EARTH_MOON_DISTANCE_REAL; // ≈0.2081

    // ================= 相机（线宽自适应基准）=================
    // 默认地心视角视距；线宽按 cameraDistance / DEFAULT_CAMERA_DISTANCE
    // 等比缩放，使屏幕上的视觉粗细保持稳定（否则放大后线变成粗带、
    // 缩小后细成发丝——这是世界单位线宽的固有问题）
    static constexpr double DEFAULT_CAMERA_DISTANCE = 30000.0;
    // 月心视角视距（约 4.6 倍月球半径）
    static constexpr double MOON_VIEW_CAMERA_DISTANCE = 8000.0;

    // ================= 线宽（数值与所属空间单位一致）=================
    // 轨道环：点坐标是 km（空间 1）。25 km 在默认相机(30000)下约 1.3 px；
    // ECEF 环近段可逼近相机(数千单位)，过粗会在近处变成巨带
    static constexpr double ORBIT_THICKNESS_KM = 25.0;
    static constexpr double DECEPTION_ORBIT_THICKNESS_KM = 45.0;
    // 回放轨迹：点已乘 DISTANCE_SCALE（空间 2），80 单位 ≈ 384 km 物理长度
    static constexpr double REPLAY_THICKNESS_UNITS = 80.0;
    // 月面轨迹：点坐标是月球局部 km（空间 1），30 km ≈ 1~2 px
    static constexpr double MOON_TRAIL_THICKNESS_KM = 10.0;   // 月球车地面轨迹刷痕

    // ================= 访问器（QML 只读属性）=================
    double earthRadius() const { return EARTH_RADIUS; }
    double moonRadius() const { return MOON_RADIUS; }
    double earthMoonDistance() const { return EARTH_MOON_DISTANCE_REAL; }
    double spherePrimitiveRadius() const { return SPHERE_PRIMITIVE_RADIUS; }
    double earthNodeScale() const { return EARTH_NODE_SCALE; }
    double moonNodeScale() const { return MOON_NODE_SCALE; }
    double atmosphereNodeScale() const { return ATMOSPHERE_NODE_SCALE; }
    double moonSceneX() const { return MOON_SCENE_X; }
    double distanceScale() const { return DISTANCE_SCALE; }
    double orbitThicknessKm() const { return ORBIT_THICKNESS_KM; }
    double deceptionOrbitThicknessKm() const { return DECEPTION_ORBIT_THICKNESS_KM; }
    double replayThicknessUnits() const { return REPLAY_THICKNESS_UNITS; }
    double moonTrailThicknessKm() const { return MOON_TRAIL_THICKNESS_KM; }
    double defaultCameraDistance() const { return DEFAULT_CAMERA_DISTANCE; }
    double moonViewCameraDistance() const { return MOON_VIEW_CAMERA_DISTANCE; }
    double markerRefDistanceKm() const { return MARKER_REF_DISTANCE_KM; }
    double markerScaleExponent() const { return MARKER_SCALE_EXPONENT; }
    double markerScaleMin() const { return MARKER_SCALE_MIN; }
    double markerScaleMax() const { return MARKER_SCALE_MAX; }
    double groundMarkerAltitudeKm() const { return GROUND_MARKER_ALTITUDE; }
    double moonTrailRadiusKm() const { return MOON_TRAIL_RADIUS; }

    // ================= 跨空间转换（必须显式调用）=================
    /** @brief 真实 km → 地月压缩空间单位（仅用于地月转移轨迹） */
    static QVector3D toDisplaySpace(const QVector3D &realKm) { return realKm * DISTANCE_SCALE; }

    /** @brief 地月压缩空间单位 → 真实 km */
    static QVector3D toRealKm(const QVector3D &displayUnits) {
        return displayUnits / DISTANCE_SCALE;
    }

    /** @brief 地月压缩空间单位 → 天体局部空间单位（1 单位 = 1 km） */
    static QVector3D displayToLocal(const QVector3D &displayUnits) {
        return displayUnits / DISTANCE_SCALE;
    }

    static QVector<QVector3D> toDisplaySpaceList(const QVector<QVector3D> &realPoints) {
        QVector<QVector3D> result;
        result.reserve(realPoints.size());
        for (const auto &p : realPoints)
            result.append(toDisplaySpace(p));
        return result;
    }

    // ================= 轴向约定映射（防"两套 ECEF"错位）=================
    // 标准 ECEF：λ=0 → +X，Z=北极（navcore geodeticToEcef 输出）。
    // 场景约定：λ=0 → +Z，Y=北极（llaToCartesian / 地球贴图）。
    // scene = (ecef.y, ecef.z, ecef.x)。历史教训：两者混用致星下点
    // 整体错位 ~90°+（见 docs/轨道参数报文接口.md §11）。
    static QVector3D sceneFromEcefStd(const QVector3D &ecefStd)
    {
        return QVector3D(ecefStd.y(), ecefStd.z(), ecefStd.x());
    }

    /** @brief 场景轴序 -> 标准 ECEF 轴序（ecefToGeodetic 等数据面计算用） */
    static QVector3D ecefStdFromScene(const QVector3D &scene)
    {
        return QVector3D(scene.z(), scene.x(), scene.y());
    }

    // ================= 场景球面坐标（唯一实现，OrbitalPhysics 转发）=====
    // 球体（WGS-84 简化）经纬度 -> 场景坐标，适配 Qt #Sphere 贴图：
    //   x = r·cos(lat)·sin(lon), y = r·sin(lat), z = r·cos(lat)·cos(lon)
    // radius 单位 km，返回 km（空间 1：1 场景单位 = 1 km）。
    // 常用半径：GROUND_MARKER_ALTITUDE(6391) / MOON_TRAIL_RADIUS(1740)。
    static QVector3D llaToScene(double lat, double lon, double radiusKm)
    {
        const double latRad = qDegreesToRadians(lat);
        const double lonRad = qDegreesToRadians(lon);
        return QVector3D(
            static_cast<float>(radiusKm * std::cos(latRad) * std::sin(lonRad)),
            static_cast<float>(radiusKm * std::sin(latRad)),
            static_cast<float>(radiusKm * std::cos(latRad) * std::cos(lonRad)));
    }

    /** @brief llaToScene 的逆变换（球面近似，见 OrbitalPhysics::cartesianToLla） */
    static void sceneToLla(const QVector3D &pos, double &lat, double &lon)
    {
        const double radius = pos.length();
        if (radius < 1e-6)
        {
            lat = 0.0;
            lon = 0.0;
            return;
        }
        lat = qRadiansToDegrees(std::asin(
            std::max(-1.0, std::min(1.0, static_cast<double>(pos.y()) / radius))));
        lon = qRadiansToDegrees(std::atan2(
            static_cast<double>(pos.x()), static_cast<double>(pos.z())));
    }

    /**
     * @brief 线宽合理性检查（供渲染层自检，防止再次出现量级错配）
     *
     * 判据：线宽 / 轨迹包围盒对角线 ∈ [1e-5, 0.5]。
     * - 轨道环 40 / ~24000 ≈ 1.7e-3  ✓
     * - 回放轨迹 80 / ~60000 ≈ 1.3e-3 ✓
     * - 月面 10 / ~3500 ≈ 2.9e-3     ✓
     * - 历史错误 0.4 / 24000 = 1.7e-5 ✗（过细，亚像素）
     * - 历史错误 30 / 35 = 0.86       ✗（过粗，巨带）
     */
    static bool thicknessIsPlausible(double extent, double thickness)
    {
        if (extent <= 0.0 || thickness <= 0.0)
            return true;  // 空轨迹/零宽度不判定
        const double ratio = thickness / extent;
        return ratio >= 1e-4 && ratio <= 0.5;
    }
};
