#pragma once
#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

#include "CoordinateSystem.h"
#include "OrbitalMechanics.h"

struct OrbitalElements {
    double semiMajorAxis;    // a (轨道半长轴)
    double eccentricity;     // e (偏心率)
    double inclination;      // i (轨道倾角, 角度)
    double raan;             // Ω (升交点赤经, 角度)
    double argPerigee;       // ω (近地点幅角, 角度)
    double meanAnomaly;      // M0 (参考历元平近点角, 角度)
    double meanMotion;       // n (平均运动, 圈/日 revs/day)
    double bStar;            // B* (摄动参数)
    
    // 辅助显示属性
    QColor color;
};

class OrbitalPhysics {
public:
    struct LLA {
        double latitude;
        double longitude;
    };

    // 求解开普勒方程: M = E - e * sin(E)。
    // 唯一实现见 navcore OrbitalMath::solveEccentricAnomaly（本类只做转发）
    static double solveKepler(double M_rad, double e) {
        return OrbitMath::solveEccentricAnomaly(M_rad, e);
    }

    // 根据六根数计算 ECI 坐标
    static QVector3D calculatePosition(const OrbitalElements& el, double timeInSeconds, bool useVisualSymmetry = true) {
        // 1. 计算当前平近点角 M
        double n_deg_per_sec = (el.meanMotion * 360.0) / 86400.0;
        double M_deg = el.meanAnomaly + n_deg_per_sec * timeInSeconds;
        double M_rad = qDegreesToRadians(fmod(M_deg, 360.0));

        // 2. 求解偏近点角 E
        double E_rad = solveKepler(M_rad, el.eccentricity);

        if (useVisualSymmetry) {
            // --- 视觉对称模式 (版本一风格) ---
            double x_orb = el.semiMajorAxis * (cos(E_rad) - el.eccentricity);
            double z_orb = el.semiMajorAxis * sqrt(1.0 - el.eccentricity * el.eccentricity) * sin(E_rad);

            QMatrix4x4 orbitRotation;
            orbitRotation.rotate(el.raan, 0, 1, 0);        // 绕 Y 轴旋转 RAAN
            orbitRotation.rotate(el.inclination, 1, 0, 0); // 绕 X 轴旋转倾角
            orbitRotation.rotate(el.argPerigee, 0, 1, 0);  // 近地点幅角

            return orbitRotation.map(QVector3D(x_orb, 0, z_orb));
        } else {
            // --- 物理标准模式 (ECI 坐标系) ---
            double x_orb = el.semiMajorAxis * (cos(E_rad) - el.eccentricity);
            double y_orb = el.semiMajorAxis * sqrt(1.0 - el.eccentricity * el.eccentricity) * sin(E_rad);

            QMatrix4x4 orbitRotation;
            orbitRotation.rotate(el.raan, 0, 1, 0);        // Ω
            orbitRotation.rotate(el.inclination, 1, 0, 0); // i
            orbitRotation.rotate(el.argPerigee, 0, 0, 1);  // ω

            // 标准 ECI 中，X-Y 是赤道面，Z 是北极方向
            // 为了匹配 View3D 的坐标系（Y 为上），我们需要做一个简单的映射
            QVector3D pos = orbitRotation.map(QVector3D(x_orb, y_orb, 0));
            return QVector3D(pos.x(), pos.z(), -pos.y()); 
        }
    }

    static QVector<QVector3D> generateOrbitPoints(const OrbitalElements& el, int segments = 180, bool useVisualSymmetry = true) {
        QVector<QVector3D> points;
        for (int i = 0; i <= segments; ++i) {
            double E_rad = (2.0 * M_PI * i) / segments;
            
            if (useVisualSymmetry) {
                double x_orb = el.semiMajorAxis * (cos(E_rad) - el.eccentricity);
                double z_orb = el.semiMajorAxis * sqrt(1.0 - el.eccentricity * el.eccentricity) * sin(E_rad);
                QMatrix4x4 orbitRotation;
                orbitRotation.rotate(el.raan, 0, 1, 0);
                orbitRotation.rotate(el.inclination, 1, 0, 0);
                orbitRotation.rotate(el.argPerigee, 0, 1, 0);
                points.append(orbitRotation.map(QVector3D(x_orb, 0, z_orb)));
            } else {
                double x_orb = el.semiMajorAxis * (cos(E_rad) - el.eccentricity);
                double y_orb = el.semiMajorAxis * sqrt(1.0 - el.eccentricity * el.eccentricity) * sin(E_rad);
                QMatrix4x4 orbitRotation;
                orbitRotation.rotate(el.raan, 0, 1, 0);
                orbitRotation.rotate(el.inclination, 1, 0, 0);
                orbitRotation.rotate(el.argPerigee, 0, 0, 1);
                QVector3D pos = orbitRotation.map(QVector3D(x_orb, y_orb, 0));
                points.append(QVector3D(pos.x(), pos.z(), -pos.y()));
            }
        }
        return points;
    }

    // 将经纬度转换为三维空间坐标 (WGS-84 简化球体)
    // 适配 Qt Quick 3D #Sphere 贴图映射: 经度 0 对应 +Z 轴
    // 
    // 重要：radius 参数单位是 km，返回值同样是 km（空间 1：1 单位 = 1 km）
    // - 地球地面点：CoordinateSystem::GROUND_MARKER_ALTITUDE（6391 km）
    // - 月球轨迹点：CoordinateSystem::MOON_TRAIL_RADIUS（1740 km）
    // - 卫星轨道点：直接使用半长轴 a（km）
    //
    // QML 渲染约定：
    // - C++ 返回 km，不预乘任何系数（禁止 ×0.01、/100 之类的"随手换算"）
    // - QML 侧节点缩放/线宽一律取自 CoordinateSystem 常量
    // - 详见 docs/坐标系与尺度规范.md
    static QVector3D llaToCartesian(double lat, double lon,
                                    double radius = CoordinateSystem::EARTH_RADIUS) {
        // 公式唯一实现见 CoordinateSystem::llaToScene（本类只做转发，
        // 防止"场景球面公式"出现第二份拷贝各自演化）
        return CoordinateSystem::llaToScene(lat, lon, radius);
    }

    // 将三维空间坐标转换回经纬度 (适配 Qt Quick 3D #Sphere 贴图映射)
    static void cartesianToLla(const QVector3D& pos, double& lat, double& lon) {
        CoordinateSystem::sceneToLla(pos, lat, lon);
    }

    // 重载版本：返回 LLA 结构体，方便链式调用
    static LLA cartesianToLla(double x, double y, double z) {
        LLA lla;
        cartesianToLla(QVector3D(x, y, z), lla.latitude, lla.longitude);
        return lla;
    }

    // ============================================================
    // 月球坐标转换函数
    // ============================================================
    
    // 月球物理参数
    // Qt #Sphere 默认半径 = 50（直径 100），scale 直接乘在半径上
    // MoonItem.qml scale = 34.74 → 场景半径 = 50 × 34.74 = 1737 单位 = 1737 km
    // 场景比例：1 场景单位 = 1 km
// 月球尺度常量统一来自 CoordinateSystem（MOON_RADIUS_DISPLAY / MOON_SCENE_X），
// 本类不再另行定义，避免出现"第二套事实来源"
    
    /**
     * @brief 月球经纬度转换为场景坐标
     * 
     * 将月心固定坐标系（Selenographic）的经纬度转换为Qt3D场景坐标。
     * 月球在场景中位于 x=80000 的位置。
     * 
     * QML 中月球 scale = 34.74，#Sphere 基准半径 50 → 场景半径 = 1737 单位
     * 场景比例：1 场景单位 = 1 km（与地球一致）
     * 
     * @param lat 月球纬度（度），范围 -90 ~ +90
     * @param lon 月球经度（度），范围 -180 ~ +180
     * @param altKm 相对月面的高度（km），默认为0（月面）
     * @return 场景坐标（已包含月球位置偏移）
     */
    static QVector3D moonLlaToScene(double lat, double lon, double altKm = 0.0) {
        constexpr double KM_TO_SCENE = 1.0;  // 1 场景单位 = 1 km
        
        // 月球场景半径（加上高度）
        double sceneRadius = CoordinateSystem::MOON_RADIUS_DISPLAY + altKm * KM_TO_SCENE;
        
        // 1. 计算相对于月球中心的局部坐标
        QVector3D localPos = llaToCartesian(lat, lon, sceneRadius);
        
        // 2. 加上月球在场景中的位置偏移
        return QVector3D(
            localPos.x() + CoordinateSystem::MOON_SCENE_X,
            localPos.y(),
            localPos.z()
        );
    }
    
    /**
     * @brief 场景坐标转换为月球经纬度
     * 
     * 将Qt3D场景坐标转换回月心固定坐标系（Selenographic）。
     * 
     * @param scenePos 场景坐标
     * @param lat 输出：月球纬度（度）
     * @param lon 输出：月球经度（度）
     * @param alt 输出：相对月面的高度（km）
     */
    static void sceneToMoonLla(const QVector3D& scenePos, double& lat, double& lon, double& alt) {
        // 1. 减去月球位置偏移，得到相对于月球中心的坐标
        QVector3D localPos(scenePos.x() - CoordinateSystem::MOON_SCENE_X, scenePos.y(), scenePos.z());
        
        // 2. 计算距离月球中心的距离
        double radius = localPos.length();
        // 高度（km）：(radius - CoordinateSystem::MOON_RADIUS_DISPLAY) / KM_TO_SCENE
        constexpr double KM_TO_SCENE = 1.0;  // 1 场景单位 = 1 km
        alt = (radius - CoordinateSystem::MOON_RADIUS_DISPLAY) / KM_TO_SCENE;
        
        // 3. 转换为经纬度
        cartesianToLla(localPos, lat, lon);
    }
};

