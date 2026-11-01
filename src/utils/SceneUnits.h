#pragma once

// SceneUnits.h - 统一单位换算常量
//
// 场景单位系统规范：
// ==================
// 1. 场景单位(SU): Qt3D场景中的基本单位
// 2. 换算关系: 1 SU = 50 km (由地球scale=127.42对应6371km推导)
//    - 地球真实半径: 6371 km
//    - 地球scale: 127.42 (Qt3D #Sphere默认半径1，scale=127.42)
//    - 推导: 6371 km / 127.42 SU = 50 km/SU
//
// 3. 所有物理计算使用公里(km)
// 4. 所有渲染坐标使用场景单位(SU)
// 5. 换算必须在数据传递给渲染层前完成
//
// 使用规则：
// - 物理层: 所有距离、速度、轨道半径使用km
// - 渲染层(QML): 所有position、scale使用SU
// - 换算: kmToScene() / sceneToKm()
//
// 示例：
//   double orbitRadius_km = 12000.0;  // LEO轨道
//   double orbitRadius_su = SceneUnits::kmToScene(orbitRadius_km);  // = 240.0
//   // 验证: 240.0 SU > 127.42 SU (地球半径) ✓

namespace SceneUnits {

// 核心换算常量
constexpr double KM_PER_SCENE_UNIT = 50.0;       // 1场景单位 = 50公里
constexpr double SCENE_UNITS_PER_KM = 1.0 / KM_PER_SCENE_UNIT;  // 1公里 = 0.02场景单位

// 真实物理常量(公里)
constexpr double EARTH_RADIUS_KM = 6371.0;
constexpr double MOON_RADIUS_KM = 1737.4;
constexpr double MOON_DISTANCE_KM = 384400.0;    // 地月平均距离

// 场景中的缩放值(场景单位)
constexpr double EARTH_SCALE = EARTH_RADIUS_KM / KM_PER_SCENE_UNIT;  // = 127.42
constexpr double MOON_SCALE = MOON_RADIUS_KM / KM_PER_SCENE_UNIT;    // = 34.74
constexpr double MOON_POSITION = MOON_DISTANCE_KM / KM_PER_SCENE_UNIT;  // = 7688.0

// 轨道半径参考(公里)
constexpr double LEO_RADIUS_KM = 12000.0;   // 低地球轨道
constexpr double MEO_RADIUS_KM = 26560.0;   // 中地球轨道 (GPS轨道)
constexpr double GEO_RADIUS_KM = 42164.0;   // 地球同步轨道

// 换算函数
inline double kmToScene(double km) {
    return km * SCENE_UNITS_PER_KM;
}

inline double sceneToKm(double su) {
    return su * KM_PER_SCENE_UNIT;
}

// 验证轨道是否在星球外
inline bool isOrbitOutsidePlanet(double orbitRadiusKm, double planetRadiusKm) {
    return orbitRadiusKm > planetRadiusKm;
}

} // namespace SceneUnits
