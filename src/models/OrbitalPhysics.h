#pragma once
#include <QVector3D>
#include <QMatrix4x4>
#include <QtMath>

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

    // 求解开普勒方程: M = E - e * sin(E)
    static double solveKepler(double M_rad, double e) {
        double E = M_rad; 
        for (int i = 0; i < 10; ++i) {
            double delta = (E - e * sin(E) - M_rad) / (1.0 - e * cos(E));
            E -= delta;
            if (qAbs(delta) < 1e-10) break;
        }
        return E;
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
    // 重要：radius 参数单位是 km，不是场景单位！
    // - 地球地面点：llaToCartesian(lat, lon, 6391.0) - 6391 km
    // - 月球轨迹点：llaToCartesian(lat, lon, 1740.0) - 1740 km
    // - 返回值单位与 radius 相同（km）
    // 
    // QML 渲染约定：
    // - C++返回km单位，不做转换
    // - QML的GroundWarningItem自动处理km→scene转换（scaledRadius = radius / 100.0）
    // - 禁止在C++层预先转换！参见AGENTS.md"坐标系统约定"
    static QVector3D llaToCartesian(double lat, double lon, double radius = 6371.0) {
        double latRad = qDegreesToRadians(lat);
        double lonRad = qDegreesToRadians(lon);

        // Qt Sphere 贴图对齐公式:
        // x = r * cos(lat) * sin(lon)
        // y = r * sin(lat)
        // z = r * cos(lat) * cos(lon)
        float x = radius * cos(latRad) * sin(lonRad);
        float y = radius * sin(latRad);
        float z = radius * cos(latRad) * cos(lonRad);

        return QVector3D(x, y, z);
    }

    // 将三维空间坐标转换回经纬度 (适配 Qt Quick 3D #Sphere 贴图映射)
    static void cartesianToLla(const QVector3D& pos, double& lat, double& lon) {
        double radius = pos.length();
        if (radius < 1e-6) {
            lat = 0; lon = 0;
            return;
        }

        // lat = asin(y / r)
        lat = qRadiansToDegrees(asin(qBound(-1.0, (double)pos.y() / radius, 1.0)));

        // lon = atan2(x, z)
        lon = qRadiansToDegrees(atan2((double)pos.x(), (double)pos.z()));
    }

    // 重载版本：返回 LLA 结构体，方便链式调用
    static LLA cartesianToLla(double x, double y, double z) {
        LLA lla;
        cartesianToLla(QVector3D(x, y, z), lla.latitude, lla.longitude);
        return lla;
    }

    // 获取月球在特定时间的坐标 (基于真实比例)
    // 真实地月距离: 384400 km，按 1:100 缩放为 3844
    static QVector3D getMoonPosition(double timeInSeconds) {
        // QML 中 moonPivot 的动画是 1000000ms (1000s) 转一圈
        double angle = (timeInSeconds / 1000.0) * 2.0 * M_PI;
        float x = 3844.0 * cos(angle);
        float z = -3844.0 * sin(angle); // 顺时针旋转匹配 QML
        return QVector3D(x, 0, z);
    }
    
    // ============================================================
    // 月球坐标转换函数
    // ============================================================
    
    // 月球物理参数
    // Qt #Sphere 默认半径 = 1（单位球），scale 直接作用于半径
    // MoonItem.qml scale = 34.74（直径）→ 实际半径 = 34.74 / 2 = 17.37
static constexpr double MOON_SCENE_RADIUS = 17.37;       // 月球场景半径（scale 34.74 / 2）
static constexpr double MOON_SCENE_X = 80000.0;          // 月球在场景中的X偏移
    
    /**
     * @brief 月球经纬度转换为场景坐标
     * 
     * 将月心固定坐标系（Selenographic）的经纬度转换为Qt3D场景坐标。
     * 月球在场景中位于 x=80000 的位置。
     * 
     * QML 中月球 scale = 34.74（直径），真实月球半径 = 1737.4 km
     * 场景半径 = 34.74 / 2 = 17.37
     * 转换比例：17.37 / 1737.4 ≈ 0.01（1 km ≈ 0.01 场景单位）
     * 
     * @param lat 月球纬度（度），范围 -90 ~ +90
     * @param lon 月球经度（度），范围 -180 ~ +180
     * @param altKm 相对月面的高度（km），默认为0（月面）
     * @return 场景坐标（已包含月球位置偏移）
     */
    static QVector3D moonLlaToScene(double lat, double lon, double altKm = 0.0) {
        // 真实月球半径 1737.4 km，场景半径 17.37
        // 比例：1 km = 17.37 / 1737.4 ≈ 0.01 场景单位
        constexpr double KM_TO_SCENE = 17.37 / 1737.4;  // ≈ 0.01
        
        // 月球场景半径（加上高度）
        double sceneRadius = MOON_SCENE_RADIUS + altKm * KM_TO_SCENE;
        
        // 1. 计算相对于月球中心的局部坐标
        QVector3D localPos = llaToCartesian(lat, lon, sceneRadius);
        
        // 2. 加上月球在场景中的位置偏移
        return QVector3D(
            localPos.x() + MOON_SCENE_X,
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
        QVector3D localPos(scenePos.x() - MOON_SCENE_X, scenePos.y(), scenePos.z());
        
        // 2. 计算距离月球中心的距离
        double radius = localPos.length();
        // 高度（km）：(radius - MOON_SCENE_RADIUS) / KM_TO_SCENE
        constexpr double KM_TO_SCENE = 34.74 / 1737.4;
        alt = (radius - MOON_SCENE_RADIUS) / KM_TO_SCENE;
        
        // 3. 转换为经纬度
        cartesianToLla(localPos, lat, lon);
    }
};

