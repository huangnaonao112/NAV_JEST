cmake_minimum_required(VERSION 3.21)

project(NAV_JEST LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Quick3D Core Gui Network Sql WebSockets Test)

qt_standard_project_setup()

# qml.qrc must be compiled by rcc (qmake did this natively via RESOURCES +=)
set(CMAKE_AUTORCC ON)

# ---------------------------------------------------------------------------
# earth_server_core: server-side code shared by the client (embedded offline
# tiles/HTTP/WS server) and the standalone appEarthServer
# ---------------------------------------------------------------------------
add_library(earth_server_core STATIC
    src/Config.h                                   src/Config.cpp
    src/server/api/HttpServer.h                    src/server/api/HttpServer.cpp
    src/server/api/WsHub.h                         src/server/api/WsHub.cpp
    src/server/infra/Db.h                          src/server/infra/Db.cpp
    src/server/infra/PasswordHasher.h              src/server/infra/PasswordHasher.cpp
    src/server/domain/device/DeviceRepository.h    src/server/domain/device/DeviceRepository.cpp
    src/server/domain/scene/SceneRepository.h      src/server/domain/scene/SceneRepository.cpp
    src/server/domain/scene/SceneConfigRepository.h src/server/domain/scene/SceneConfigRepository.cpp
    src/server/domain/scene/SceneService.h         src/server/domain/scene/SceneService.cpp
    src/server/domain/scene/SceneDetailMapper.h src/server/domain/scene/SceneDetailMapper.cpp src/server/domain/scene/SceneDetailMapper_DbMap.cpp src/server/domain/scene/SceneDetailMapper_Channels.cpp
    src/server/domain/user/UserRepository.h        src/server/domain/user/UserRepository.cpp
    src/server/domain/tiles/MbTiles.h              src/server/domain/tiles/MbTiles.cpp
)
target_include_directories(earth_server_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/models
)
target_link_libraries(earth_server_core PUBLIC
    Qt6::Core Qt6::Gui Qt6::Network Qt6::Sql Qt6::WebSockets
)
# Lean PCH for server-side targets: pch.h pulls in QtQuick3D which the server
# does not link, so only the core/network/sql set is precompiled here.
target_precompile_headers(earth_server_core PRIVATE
    <QtCore>
    <QFile>
    <QDir>
    <QTimer>
    <QDateTime>
    <QUrl>
    <QJsonDocument>
    <QJsonObject>
    <QJsonArray>
    <QTcpServer>
    <QTcpSocket>
    <QWebSocket>
    <QWebSocketServer>
    <QSqlDatabase>
    <QSqlQuery>
    <QSqlError>
    <QImage>
    <iostream>
    <vector>
    <map>
    <set>
    <memory>
    <functional>
    <algorithm>
    <cmath>
    <string>
)

# ---------------------------------------------------------------------------
# NAV_JEST: QML client with embedded server
# ---------------------------------------------------------------------------
qt_add_executable(NAV_JEST WIN32
    src/main.cpp
    src/models/OrbitalPhysics.h
    src/models/RackStatusModel.h        src/models/RackStatusModel.cpp
    src/models/MoonTrajectoryProvider.h src/models/MoonTrajectoryProvider.cpp
    # QQuickImageProvider-based tile provider: client-only (QtQuick dependency)
    src/server/domain/tiles/MbTilesImageProvider.h src/server/domain/tiles/MbTilesImageProvider.cpp
    qml.qrc
)

# C++ 模型以 QML 模块形式注册（QML_ELEMENT / QML_SINGLETON），QML 侧经
# import NavModels 按类型名访问，不再走 setContextProperty。
# RackStatusModel 需要两个实例（A/B 机柜），不在此注册：main.cpp 用
# qmlRegisterSingletonInstance 按实例注册为 RackStatusModelA/B。
qt_add_qml_module(NAV_JEST
    URI NavModels
    VERSION 1.0
    SOURCES
        src/models/SatelliteModel.h         src/models/SatelliteModel.cpp
        src/models/DeviceModel.h            src/models/DeviceModel.cpp
        src/models/SceneModel.h             src/models/SceneModel.cpp
        src/models/UserModel.h              src/models/UserModel.cpp
        src/models/PntDataModel.h           src/models/PntDataModel.cpp
        src/models/AntLightModel.h          src/models/AntLightModel.cpp
        src/models/ProbeMissionModel.h      src/models/ProbeMissionModel.cpp
        src/models/TrajectoryLoader.h       src/models/TrajectoryLoader.cpp
        src/models/TransferOrbitModel.h     src/models/TransferOrbitModel.cpp
        src/models/MoonTrajectoryModel.h    src/models/MoonTrajectoryModel.cpp
        src/models/MoonTrajectoryGeometry.h src/models/MoonTrajectoryGeometry.cpp
        src/RackStatusClient.h              src/RackStatusClient.cpp
        src/models/CoordinateSystem.h       src/models/CoordinateSystem.cpp
        src/models/WindowPlacement.h
        src/models/SignalEvaluationModel.h  src/models/SignalEvaluationModel.cpp
        src/models/RouteGeneratorModel.h    src/models/RouteGeneratorModel.cpp
)
target_link_libraries(NAV_JEST PRIVATE
    earth_server_core
    navcore
    Qt6::Quick Qt6::Quick3D Qt6::Core Qt6::Gui Qt6::Network Qt6::Sql Qt6::WebSockets
)
target_include_directories(NAV_JEST PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_precompile_headers(NAV_JEST PRIVATE src/pch.h)

# ---------------------------------------------------------------------------
# appEarthServer: standalone server
# ---------------------------------------------------------------------------
qt_add_executable(appEarthServer WIN32
    src/server/main.cpp
)
target_link_libraries(appEarthServer PRIVATE earth_server_core)
target_precompile_headers(appEarthServer PRIVATE
    <QtCore>
    <QFile>
    <QDir>
    <QJsonDocument>
    <QJsonObject>
    <QTcpServer>
    <QWebSocketServer>
    <QSqlDatabase>
    <QSqlError>
    <QImage>
    <iostream>
    <vector>
    <memory>
    <string>
)

# ---------------------------------------------------------------------------
# Runtime files copied next to the executable (was QMAKE_POST_LINK in the
# removed NAV-JEST.pro): large meshes stay out of rcc, configs ship as files
# ---------------------------------------------------------------------------
set(RUNTIME_DIR $<TARGET_FILE_DIR:NAV_JEST>)

add_custom_command(TARGET NAV_JEST POST_BUILD
    # liangge models
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RUNTIME_DIR}/models/liangge/meshes
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RUNTIME_DIR}/models/liangge/maps
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/Liangge.qml    ${RUNTIME_DIR}/models/liangge/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/ZhuanTai.qml   ${RUNTIME_DIR}/models/liangge/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/QiongDing.qml  ${RUNTIME_DIR}/models/liangge/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/meshes/__012_mesh.mesh                                    ${RUNTIME_DIR}/models/liangge/meshes/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/meshes/a660edf19bd0adcd41a95dc58293cbd2_mesh.mesh        ${RUNTIME_DIR}/models/liangge/meshes/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/maps/textureData.png                                      ${RUNTIME_DIR}/models/liangge/maps/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/liangge/maps/textureData12.png                                    ${RUNTIME_DIR}/models/liangge/maps/
    # satellite model
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RUNTIME_DIR}/models/meshes
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/meshes/node10477_Satellite_v1_SG.mesh                             ${RUNTIME_DIR}/models/meshes/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/meshes/tripo_mesh_6b8f1a17_e70c_4ec2_9dbc_3d96db177f92_mesh.mesh  ${RUNTIME_DIR}/models/meshes/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/resources/models/satellite_diffuse.jpg                                             ${RUNTIME_DIR}/models/
    # configs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/config.json   ${RUNTIME_DIR}/
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RUNTIME_DIR}/config
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RUNTIME_DIR}/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/config ${RUNTIME_DIR}/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resources/data   ${RUNTIME_DIR}/data
)
if(EXISTS ${CMAKE_SOURCE_DIR}/trajectory_wgs84.json)
    add_custom_command(TARGET NAV_JEST POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/trajectory_wgs84.json ${RUNTIME_DIR}/
    )
endif()

# ---------------------------------------------------------------------------
# navcore: 纯 C++17 GNSS 算法内核（自 NAV_JEST V5.5.0 移植，零 Qt 依赖）
#   OrbitalMechanics        六根数/开普勒传播/ECI-ECEF/GMST/WGS84/地面站观测/RTN ΔV
#   GnssSignalSimulation    24 通道信号体制/C-N0/干扰 J-S/欺骗参数数学评估
#   VirtualReceiver         伪距最小二乘定位解算/钟差/残差 RMS
# ---------------------------------------------------------------------------
add_library(navcore STATIC
    src/algo/OrbitalMechanics.h       src/algo/OrbitalMechanics.cpp
    src/algo/GnssSignalSimulation.h   src/algo/GnssSignalSimulation.cpp
    src/algo/VirtualReceiver.h        src/algo/VirtualReceiver.cpp
    src/algo/TrajectoryEngine.h      src/algo/TrajectoryEngine.cpp
)
target_include_directories(navcore PUBLIC ${CMAKE_SOURCE_DIR}/src/algo)
target_compile_features(navcore PUBLIC cxx_std_17)

# 算法回归测试（纯控制台，无 Qt）：ctest -C Debug/Release
enable_testing()
add_executable(navcore_orbit_tests   tests/algo/OrbitalMechanicsTests.cpp)
add_executable(navcore_signal_tests  tests/algo/GnssSignalSimulationTests.cpp)
add_executable(navcore_receiver_tests tests/algo/VirtualReceiverTests.cpp)
add_executable(navcore_trajectory_tests tests/algo/TrajectoryEngineTests.cpp)
foreach(navcore_test navcore_orbit_tests navcore_signal_tests navcore_receiver_tests navcore_trajectory_tests)
    target_link_libraries(${navcore_test} PRIVATE navcore)
    add_test(NAME ${navcore_test} COMMAND ${navcore_test})
endforeach()

# 桥接层回归测试（QTest，无 UI）：直接断言 SignalEvaluationModel 的
# 参数→评估结果链路，锚点值与评估页界面实测三场景一致。
add_executable(navcore_model_tests
    tests/model/SignalEvaluationModelTests.cpp
    src/models/SignalEvaluationModel.h
    src/models/SignalEvaluationModel.cpp
)
target_link_libraries(navcore_model_tests PRIVATE navcore Qt6::Core Qt6::Qml Qt6::Test)
target_include_directories(navcore_model_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/models)
add_test(NAME navcore_model_tests COMMAND navcore_model_tests)
# ctest 直接可跑：把 Qt bin 目录前置到测试进程 PATH（Qt6_DIR=.../lib/cmake/Qt6）
set_tests_properties(navcore_model_tests PROPERTIES
    ENVIRONMENT "PATH=${Qt6_DIR}/../../../bin\;$ENV{PATH}")

# SatelliteModel navcore 接口回归测试（QTest，无 UI）
add_executable(navcore_satmodel_tests
    tests/model/SatelliteModelNavcoreTests.cpp
    src/models/SatelliteModel.h
    src/models/SatelliteModel.cpp
)
target_link_libraries(navcore_satmodel_tests PRIVATE navcore Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
target_include_directories(navcore_satmodel_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/models)
add_test(NAME navcore_satmodel_tests COMMAND navcore_satmodel_tests)
set_tests_properties(navcore_satmodel_tests PROPERTIES
    ENVIRONMENT "PATH=${Qt6_DIR}/../../../bin\;$ENV{PATH}")

# 航路生成桥接层回归测试（QTest，无 UI）
add_executable(navcore_route_tests
    tests/model/RouteGeneratorModelTests.cpp
    src/models/RouteGeneratorModel.h
    src/models/RouteGeneratorModel.cpp
)
target_link_libraries(navcore_route_tests PRIVATE navcore Qt6::Core Qt6::Qml Qt6::Test)
target_include_directories(navcore_route_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/models)
add_test(NAME navcore_route_tests COMMAND navcore_route_tests)
set_tests_properties(navcore_route_tests PROPERTIES
    ENVIRONMENT "PATH=${Qt6_DIR}/../../../bin\;$ENV{PATH}")
