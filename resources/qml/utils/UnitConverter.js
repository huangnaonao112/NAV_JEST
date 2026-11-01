// UnitConverter.js - 统一单位换算工具
// 
// 场景单位系统规范：
// ==================
// 1. 场景单位(SU): Qt3D场景中的基本单位
// 2. 换算关系: 1 SU = 50 km (由地球scale=127.42对应6371km推导)
// 3. 所有物理计算使用公里(km)
// 4. 所有渲染坐标使用场景单位(SU)
// 5. 换算必须在数据传递给渲染层前完成
//
// 使用规则：
// - 物理层(C++): 所有距离、速度、轨道半径使用km
// - 渲染层(QML): 所有position、scale使用SU
// - 换算: kmToScene() / sceneToKm()

.pragma library

// 核心换算常量
var KM_PER_SCENE_UNIT = 50.0  // 1场景单位 = 50公里
var SCENE_UNITS_PER_KM = 1.0 / KM_PER_SCENE_UNIT  // 1公里 = 0.02场景单位

// 真实物理常量(公里)
var EARTH_RADIUS_KM = 6371.0
var MOON_RADIUS_KM = 1737.4
var MOON_DISTANCE_KM = 384400.0  // 地月平均距离

// 场景中的缩放值(场景单位)
var EARTH_SCALE = EARTH_RADIUS_KM / KM_PER_SCENE_UNIT  // = 127.42
var MOON_SCALE = MOON_RADIUS_KM / KM_PER_SCENE_UNIT    // = 34.74

// 轨道半径参考(公里)
var LEO_RADIUS_KM = 12000.0    // 低地球轨道
var MEO_RADIUS_KM = 26560.0    // 中地球轨道
var GEO_RADIUS_KM = 42164.0    // 地球同步轨道

// 换算函数
function kmToScene(km) {
    return km * SCENE_UNITS_PER_KM
}

function sceneToKm(su) {
    return su * KM_PER_SCENE_UNIT
}

// 批量换算轨道点数组
function kmArrayToScene(kmArray) {
    var result = []
    for (var i = 0; i < kmArray.length; i++) {
        result.push(kmToScene(kmArray[i]))
    }
    return result
}

// 验证轨道是否在星球外
function isOrbitOutsidePlanet(orbitRadiusKm, planetRadiusKm) {
    return orbitRadiusKm > planetRadiusKm
}

// 获取轨道半径建议值(返回公里)
function getSuggestedOrbitRadius(planetType, orbitType) {
    if (planetType === "earth") {
        switch(orbitType) {
            case "leo": return LEO_RADIUS_KM
            case "meo": return MEO_RADIUS_KM
            case "geo": return GEO_RADIUS_KM
            case "parking": return 12000.0  // 停泊轨道
            default: return LEO_RADIUS_KM
        }
    } else if (planetType === "moon") {
        switch(orbitType) {
            case "low": return 2000.0   // 近月轨道
            case "mid": return 3000.0
            case "high": return 4000.0
            default: return 2000.0
        }
    }
    return LEO_RADIUS_KM
}
