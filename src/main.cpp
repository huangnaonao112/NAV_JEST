#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlError>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QtNetwork/QHostAddress>
#include <QTimer>
#include <QElapsedTimer>
#include <QSurfaceFormat>
#include <QString>
#include "Config.h"
#include "models/SatelliteModel.h"
#include "models/DeviceModel.h"
#include "models/SceneModel.h"
#include "models/UserModel.h"
#include "models/RackStatusModel.h"
#include "models/PntDataModel.h"
#include "models/AntLightModel.h"
#include "models/ProbeMissionModel.h"
#include "models/CoordinateSystem.h"
#include "models/TransferOrbitModel.h"
#include "models/TrajectoryLoader.h"
#include "models/MoonTrajectoryModel.h"
#include "models/MoonTrajectoryProvider.h"
#include "RackStatusClient.h"
#include "models/WindowPlacement.h"
#include "utils/SceneLogger.h"

#include "server/api/HttpServer.h"
#include "server/api/WsHub.h"
#include "server/infra/Db.h"
#include "server/domain/device/DeviceRepository.h"
#include "server/domain/user/UserRepository.h"
#include "server/domain/tiles/MbTiles.h"
#include "server/domain/tiles/MbTilesImageProvider.h"

// ---------------------------------------------------------------------
// 离线瓦片源注册：依次探测 exe 目录 / 工作目录下的候选 mbtiles 文件。
// 找到哪个用哪个，缺失时跳过（纯离线模式允许多档覆盖：全球 0-6 级、
// 全国 1-10 级、石家庄 11-15 级）。
// ---------------------------------------------------------------------
namespace {
QString probeFile(const QStringList &candidates)
{
    for (const auto &p : candidates) {
        if (QFile::exists(p)) return p;
    }
    return {};
}

void registerTileSources(MbTilesImageProvider *tileProvider)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cwd = QDir::currentPath();
    const auto here = [&appDir, &cwd](const QString &f) {
        return probeFile({ QDir(appDir).filePath(f), QDir(cwd).filePath(f) });
    };

    // 全球瓦片 (ESRI卫星影像, 覆盖经度-180~180, 纬度-90~90)
    if (const QString p = here("global_0_2.mbtiles"); !p.isEmpty()) {
        tileProvider->addMbTiles("global", p, 0, 6, -180.0, -90.0, 180.0, 90.0);
    }
    // 全国 10 级地图
    if (const QString p = probeFile({
            QDir(appDir).filePath("China.mbtiles"),
            QDir(appDir).filePath("03091035.mbtiles"),
            QDir(appDir).filePath("basemap.mbtiles"),
            QDir(cwd).filePath("China.mbtiles"),
            QDir(cwd).filePath("03091035.mbtiles"),
        }); !p.isEmpty())
    {
        tileProvider->addMbTiles("china_10", p, 1, 10, 73.0, 18.0, 135.0, 54.0);
    }
    // 石家庄 15 级地图 (经度113.8-115.2, 纬度37.5-38.5)
    if (const QString p = here("sjz.mbtiles"); !p.isEmpty()) {
        tileProvider->addMbTiles("shijiazhuang_15", p, 11, 15, 113.8, 37.5, 115.2, 38.5);
    }
}
} // namespace

int main(int argc, char *argv[])
{
    // 设置 Qt Quick3D 渲染后端 (必须在 QGuiApplication 之前)
    // 使用 OpenGL 而非默认的 D3D11/Vulkan，避免某些兼容性问题
    qputenv("QSG_RHI_BACKEND", "opengl");

    // Qt 5.15 High DPI scaling
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);

    // 初始化日志系统 (必须在 QGuiApplication 之后)
    SceneLogger::instance().log("APP_START", "应用程序启动");

    qputenv("QT_QUICK_CONTROLS_STYLE", QByteArrayLiteral("Fusion"));

    // 堆分配且不显式 delete：QML 对象通过 context property 持有下方栈上模型的
    // 裸指针，引擎析构顺序一旦晚于模型就是退出段错误（实测）；而显式 delete 引擎
    // 在退出路径会卡死（GL 上下文拆除）。沿用本项目 embeddedDb 的做法：
    // 生命周期贯穿应用，进程退出时由操作系统统一回收。
    auto *engine = new QQmlApplicationEngine;
    engine->addImportPath("qrc:/");
    engine->addImportPath("qrc:/resources");
    engine->addImportPath("qrc:/resources/qml");
    engine->addImportPath("qrc:/resources/qml/pages");
    engine->addImportPath("qrc:/resources/qml/pages/scenario");
    engine->addImportPath("qrc:/resources/qml/pages/scenario/steps");
    engine->addImportPath("qrc:/resources/qml/components");

    // ---------------------------------------------------------------------
    // Embedded offline tiles server (pure offline mode)
    // - Serves: http://127.0.0.1:18080/tiles/{z}/{x}/{y}
    // - If port is already in use, we skip starting (allow external EarthServer).
    // ---------------------------------------------------------------------

    // 创建多数据源瓦片提供器并注册离线瓦片源
    auto *tileProvider = new MbTilesImageProvider();
    registerTileSources(tileProvider);
    const QString chinaTilesPath = QDir(QCoreApplication::applicationDirPath()).filePath("China.mbtiles");

    // 应用级别的单例对象，生命周期贯穿整个应用，不需要手动删除
    // 操作系统会在进程退出时回收所有资源

    SCENE_LOG_CAT("APP_INIT", QString("创建数据库连接对象..."));
    auto *embeddedDb = new Db();

    SCENE_LOG_CAT("APP_INIT", QString("打开数据库连接..."));
    bool dbOpenOk = embeddedDb->open();
    if (!dbOpenOk) {
        SCENE_LOG_ERROR("APP_INIT", QString("数据库连接失败! 应用可能无法正常工作"));
    } else {
        SCENE_LOG_CAT("APP_INIT", QString("数据库连接成功!"));
    }

    SCENE_LOG_CAT("APP_INIT", QString("初始化 DeviceRepository..."));
    auto *embeddedDeviceRepo = new DeviceRepository(embeddedDb->database());
    embeddedDeviceRepo->init();

    SCENE_LOG_CAT("APP_INIT", QString("初始化 UserRepository..."));
    auto *embeddedUserRepo = new UserRepository(embeddedDb->database());
    embeddedUserRepo->init();

    auto *embeddedMbtiles = new MbTiles(chinaTilesPath);
    embeddedMbtiles->open();

    engine->addImageProvider(QStringLiteral("mbtiles"), tileProvider);

    auto *embeddedWsHub = new WsHub(&app);
    auto *embeddedHttpServer = new HttpServer(*embeddedDeviceRepo, *embeddedUserRepo, *embeddedMbtiles, *embeddedWsHub, &app);

    embeddedHttpServer->listen("127.0.0.1", 18080);

    embeddedWsHub->setListenHost("127.0.0.1");
    embeddedWsHub->setListenPort(18081);
    embeddedWsHub->start();

    // ---------------------------------------------------------------------
    // 模型装配：所有 QML_ELEMENT / QML_SINGLETON 模型由 QML 引擎按需创建，
    // main() 通过 singletonInstance 强制创建并按原顺序取回引用做相互装配。
    // 实例由引擎持有（引擎有意存活至进程退出），不再依赖栈上析构顺序。
    // ---------------------------------------------------------------------
    auto *satModel           = engine->singletonInstance<SatelliteModel *>("NavModels", "SatelliteModel");
    auto *deviceModel        = engine->singletonInstance<DeviceModel *>("NavModels", "DeviceModel");
    deviceModel->load();
    auto *sceneModel         = engine->singletonInstance<SceneModel *>("NavModels", "SceneModel");
    sceneModel->load();
    auto *userModel          = engine->singletonInstance<UserModel *>("NavModels", "UserModel");
    userModel->load();
    SCENE_LOG_CAT("APP_INIT", QString("UserModel loaded"));

    auto *pntModel           = engine->singletonInstance<PntDataModel *>("NavModels", "PntDataModel");
    auto *antLightModel      = engine->singletonInstance<AntLightModel *>("NavModels", "AntLightModel");
    auto *probeModel         = engine->singletonInstance<ProbeMissionModel *>("NavModels", "ProbeMissionModel");
    auto *transferOrbitModel = engine->singletonInstance<TransferOrbitModel *>("NavModels", "TransferOrbitModel");
    auto *trajectoryLoader   = engine->singletonInstance<TrajectoryLoader *>("NavModels", "TrajectoryLoader");
    auto *moonTrajectoryModel = engine->singletonInstance<MoonTrajectoryModel *>("NavModels", "MoonTrajectoryModel");
    auto *rackClient         = engine->singletonInstance<RackStatusClient *>("NavModels", "RackStatusClient");

    // 窗口放置辅助：按物理屏（screenAt 判定）计算窗口模式几何。
    // 类本身不标注 QML_SINGLETON —— 该 header-only 类经静态注册器注册后在
    // QML 侧不可见（实测），改走与 RackStatusModelA/B 相同的实例注册通道
    static WindowPlacement g_windowPlacement;
    qmlRegisterSingletonInstance("NavModels", 1, 0, "WindowPlacement", &g_windowPlacement);

    MoonTrajectoryProvider moonTrajectoryProvider;   // image provider：引擎接管其所有权
    moonTrajectoryModel->setTrajectoryProvider(&moonTrajectoryProvider);
    engine->addImageProvider(QStringLiteral("moonTrajectory"), &moonTrajectoryProvider);
    SCENE_LOG_CAT("APP_INIT", QString("Models constructed, image providers registered"));

    // A/B 两机柜需要同名类的两个实例：栈上创建并按实例注册为 QML 单例
    RackStatusModel rackModelA;
    RackStatusModel rackModelB;
    qmlRegisterSingletonInstance("NavModels", 1, 0, "RackStatusModelA", &rackModelA);
    qmlRegisterSingletonInstance("NavModels", 1, 0, "RackStatusModelB", &rackModelB);

    // CoordinateSystem 既有 C++ 单例：按实例注册，保证 C++/QML 同一实例
    qmlRegisterSingletonInstance("NavModels", 1, 0, "CoordinateSystem", CoordinateSystem::instance());

    rackModelA.setDeviceNames({
        QStringLiteral("原子钟频率分配器"),
        QStringLiteral("原子钟脉冲分配器"),
        QStringLiteral("NTP授时设备"),
        QStringLiteral("时间频率接收机"),
        QStringLiteral("WR时间同步设备"),
        QStringLiteral("高精度原子钟"),
        QStringLiteral("安全认证模拟器"),
        QStringLiteral("安全认证信号转发式欺诈源"),
        QStringLiteral("地月空间导航信号模拟器")
    });
    rackModelB.setDeviceNames({
        QStringLiteral("原子钟频率分配器"),
        QStringLiteral("原子钟脉冲分配器"),
        QStringLiteral("小型化导航干扰信号模拟器"),
        QStringLiteral("多功能导航干扰信号模拟器1"),
        QStringLiteral("多功能导航干扰信号模拟器2"),
        QStringLiteral("多功能导航干扰信号模拟器3"),
        QStringLiteral("高精度卫星信号差分源1"),
        QStringLiteral("高精度卫星信号差分源2"),
        QStringLiteral("导航干扰开关矩阵")
    });

    rackClient->setHost(Config::instance().rackClientHost());
    rackClient->setPort(Config::instance().rackClientPort());
    rackClient->setModels(&rackModelA, &rackModelB);
    rackClient->setPntModel(pntModel);
    rackClient->setAntLightModel(antLightModel);
    rackClient->setMoonTrajectoryModel(moonTrajectoryModel);
    rackClient->setSatelliteModel(satModel);
    rackClient->setProbeMissionModel(probeModel);
    rackClient->setTransferOrbitModel(transferOrbitModel);

    // 设置SatelliteModel的PntDataModel引用，用于同步干扰点数据
    satModel->setPntDataModel(pntModel);
    SCENE_LOG_CAT("APP_INIT", QString("Model wiring complete (NavModels module)"));

    // 性能测试钩子（仅测试用途）: NAVJEST_SIMTEST=1 时 QML 侧模拟持续拖动地球
    engine->rootContext()->setContextProperty(
        "_perfSimTest", QVariant(!qEnvironmentVariableIsEmpty("NAVJEST_SIMTEST")));

    // 视角测试钩子（仅测试用途）: NAVJEST_MOON_VIEW_MS=N 时登录后 N 毫秒切到月心视角，
    // 用于无 UI 自动化验证月面轨迹线渲染（0 = 不启用）
    bool moonViewOk = false;
    const int moonViewMs = qEnvironmentVariableIntValue("NAVJEST_MOON_VIEW_MS", &moonViewOk);
    engine->rootContext()->setContextProperty(
        "_moonViewAtMs", QVariant(moonViewOk ? moonViewMs : 0));

    // 2D 视图测试钩子（仅测试用途）: NAVJEST_2D_VIEW_MS=N 时登录后 N 毫秒切到
    // sim 页 2D 地图，用于自动化验证 2D 星下点与 3D 的一致性
    bool map2DOk = false;
    const int map2DMs = qEnvironmentVariableIntValue("NAVJEST_2D_VIEW_MS", &map2DOk);
    engine->rootContext()->setContextProperty(
        "_map2DViewMs", QVariant(map2DOk ? map2DMs : 0));

    // 相机角度钩子（仅测试用途）: NAVJEST_CAMERA_ROT_X/Y 覆盖初始俯仰/方位角，
    // 便于自动化在多角度取证轨道环渲染
    bool rotXOk = false, rotYOk = false;
    const int rotX = qEnvironmentVariableIntValue("NAVJEST_CAMERA_ROT_X", &rotXOk);
    const int rotY = qEnvironmentVariableIntValue("NAVJEST_CAMERA_ROT_Y", &rotYOk);
    engine->rootContext()->setContextProperty("_cameraRotX", QVariant(rotXOk ? rotX : -30));
    engine->rootContext()->setContextProperty("_cameraRotY", QVariant(rotYOk ? rotY : 15));

    // 相机距离钩子（仅测试用途）: NAVJEST_CAMERA_DIST 覆盖初始视距，
    // 用于近距离审计线宽/拐角接缝/抗锯齿
    bool distOk = false;
    const int camDist = qEnvironmentVariableIntValue("NAVJEST_CAMERA_DIST", &distOk);
    engine->rootContext()->setContextProperty("_cameraDist", QVariant(distOk ? camDist : 0));

    // Setup simulation timer (60 FPS)
    QTimer timer;
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    QObject::connect(&timer, &QTimer::timeout, [satModel, probeModel, &elapsedTimer](){
        // Update simulation state with elapsed time in seconds
        double elapsedSeconds = elapsedTimer.elapsed() / 1000.0;
        satModel->updateSimulation(elapsedSeconds);
        probeModel->updateSimulation(0.016);
    });
    timer.start(16);  // 60fps

    // Standard QRC loading
    const QUrl url(QStringLiteral("qrc:/resources/qml/main.qml"));

    // QML 运行时告警接入场景日志：Release GUI 下 stderr 为空，
    // 绑定表达式/类型实例化错误若只看 stderr 会"静默丢失"（曾导致
    // 整屏不渲染却无任何日志）。这里统一落到 logs/scene_debug.log。
    QObject::connect(engine, &QQmlEngine::warnings, &app,
                     [](const QList<QQmlError> &warnings) {
        for (const QQmlError &e : warnings)
            SCENE_LOG_ERROR("QML", e.toString());
    });

    QObject::connect(engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    SCENE_LOG_CAT("APP_INIT", QString("Loading QML: %1").arg(url.toString()));
    engine->load(url);
    SCENE_LOG_CAT("APP_INIT", QString("QML loaded, entering event loop"));

    // 启动自动重连 - 客户端会持续尝试连接服务端
    rackClient->startAutoReconnect();
    SCENE_LOG_CAT("APP_INIT", QString("RackClient auto-reconnect started"));

    // 调试钩子: NAVJEST_AUTOTOGGLE_ORBITS=1 时 3 秒后自动勾选低轨(前5颗可见)，
    // 等价于轨道显示面板第一行开关，用于无 UI 自动化验证轨道环渲染。
    // NAVJEST_AUTOTOGGLE_FROM/TO 可指定卫星索引区间（如 5-9=中轨蓝环）。
    if (!qEnvironmentVariableIsEmpty("NAVJEST_AUTOTOGGLE_ORBITS")) {
        int from = 0;
        int to = 4;
        if (qEnvironmentVariableIsSet("NAVJEST_AUTOTOGGLE_FROM")) {
            from = qEnvironmentVariableIntValue("NAVJEST_AUTOTOGGLE_FROM");
            to = qEnvironmentVariableIntValue("NAVJEST_AUTOTOGGLE_TO");
            if (to < from)
                to = from;
        }
        QTimer::singleShot(3000, satModel, [satModel, from, to]() {
            const int n = satModel->satelliteCount();
            QVariantList indices, visibles;
            for (int i = from; i <= qMin(to, n - 1); ++i) {
                indices << i;
                visibles << true;
            }
            satModel->setVisibilityBatch(indices, visibles);
            SCENE_LOG_CAT("APP_INIT", QString("AUTOTOGGLE: sats %1-%2 on (%3)")
                .arg(from).arg(to).arg(indices.size()));
        });
    }

    QTimer::singleShot(500, []{ SCENE_LOG_CAT("APP_ALIVE", QString("500ms")); });
    QTimer::singleShot(2000, []{ SCENE_LOG_CAT("APP_ALIVE", QString("2000ms")); });
    QTimer::singleShot(5000, []{ SCENE_LOG_CAT("APP_ALIVE", QString("5000ms")); });

    // 冒烟测试钩子: 设置 NAVJEST_AUTOCLOSE_MS 后 N 毫秒自动退出（验证干净关机路径）
    bool autocloseOk = false;
    const int autocloseMs = qEnvironmentVariableIntValue("NAVJEST_AUTOCLOSE_MS", &autocloseOk);
    if (autocloseOk && autocloseMs > 0) {
        QTimer::singleShot(autocloseMs, []{
            SCENE_LOG_CAT("APP_EXIT", QString("auto-close triggered"));
            QCoreApplication::quit();
        });
    }

    // 性能探针: NAVJEST_PERF=1 时统计实际渲染帧率与最大卡顿间隔
    // NAVJEST_AUTOTOUR_MS=N 时每 N 毫秒自动轮换 sim/management/cabinet/dome 页面
    QObject *perfRoot = engine->rootObjects().value(0, nullptr);
    if (QQuickWindow *perfWin = qobject_cast<QQuickWindow *>(perfRoot)) {
        perfWin->setPersistentGraphics(true);
        if (!qEnvironmentVariableIsEmpty("NAVJEST_PERF")) {
            auto frameTimer = new QElapsedTimer();
            frameTimer->start();
            auto lastFrame = new QElapsedTimer();
            lastFrame->start();
            auto stats = new QElapsedTimer();
            stats->start();
            auto frameCount = new quint64(0);
            auto maxGap = new double(0.0);
            QObject::connect(perfWin, &QQuickWindow::frameSwapped, perfWin, [frameTimer, lastFrame, stats, frameCount, maxGap](){
                const double gap = lastFrame->restart() / 1000.0;
                // 首帧及 500ms 内的长停顿（页面切换/编译着色器）不计入空闲帧统计
                if (*frameCount > 0 && gap > *maxGap && gap < 0.5)
                    *maxGap = gap;
                ++*frameCount;
                if (stats->elapsed() >= 2000) {
                    const double fps = *frameCount / (stats->restart() / 1000.0);
                    SCENE_LOG_CAT("PERF", QString("fps=%1 maxGap=%2ms totalFrames=%3 wall=%4s")
                        .arg(fps, 0, 'f', 1)
                        .arg(*maxGap * 1000.0, 0, 'f', 1)
                        .arg(*frameCount)
                        .arg(frameTimer->elapsed() / 1000.0));
                    *maxGap = 0.0;
                    *frameCount = 0;
                }
            });
            QObject::connect(perfWin, &QQuickWindow::sceneGraphError, perfWin,
                             [](QQuickWindow::SceneGraphError, const QString &msg){
                SCENE_LOG_ERROR("PERF", QString("sceneGraphError: %1").arg(msg));
            });
            SCENE_LOG_CAT("PERF", QString("frame probe attached"));
        }
        bool tourOk = false;
        const int tourMs = qEnvironmentVariableIntValue("NAVJEST_AUTOTOUR_MS", &tourOk);
        if (tourOk && tourMs > 0) {
            const QStringList pages = {QStringLiteral("sim"), QStringLiteral("management"),
                                       QStringLiteral("cabinet"), QStringLiteral("dome")};
            auto pageIdx = new int(0);
            auto tourTimer = new QTimer(perfWin);
            tourTimer->setInterval(tourMs);
            QObject::connect(tourTimer, &QTimer::timeout, perfWin, [perfRoot, pages, pageIdx](){
                const QString key = pages.at(*pageIdx % pages.size());
                ++*pageIdx;
                QMetaObject::invokeMethod(perfRoot, "openPage", Q_ARG(QVariant, key));
                SCENE_LOG_CAT("PERF", QString("tour -> %1").arg(key));
            });
            tourTimer->start();
            SCENE_LOG_CAT("PERF", QString("auto-tour every %1ms").arg(tourMs));
        }

        // NAVJEST_MAPVIEW=1: 登录后自动切到 2D 地图并全屏（验证地图铺满视口）
        if (!qEnvironmentVariableIsEmpty("NAVJEST_MAPVIEW")) {
            QTimer::singleShot(2000, perfWin, [perfRoot](){ perfRoot->setProperty("simCurrentView", "2D"); });
            QTimer::singleShot(4000, perfWin, [perfWin](){ perfWin->showFullScreen(); });
        }

        // NAVJEST_AUTOLOGIN_MS=N: N 毫秒后自动触发 loginSuccess，测量登录->主界面的过渡耗时
        bool alOk = false;
        const int autoLoginMs = qEnvironmentVariableIntValue("NAVJEST_AUTOLOGIN_MS", &alOk);
        if (alOk && autoLoginMs > 0) {
            QTimer::singleShot(autoLoginMs, perfWin, [perfWin, perfRoot](){
                QObject *loginItem = nullptr;
                const auto items = perfRoot->findChildren<QObject *>();
                for (QObject *o : items) {
                    if (o->metaObject()->indexOfSignal("loginSuccess(QString)") >= 0) { loginItem = o; break; }
                }
                if (!loginItem) {
                    SCENE_LOG_CAT("PERF", QString("auto-login: login screen not found"));
                    return;
                }
                // 层序验证: 登录页应仍处于窗口中心的最顶层（用户必须先看到/点它）
                const auto kids = perfWin->contentItem()->childItems();
                for (const QQuickItem *k : kids) {
                    SCENE_LOG_CAT("PERF", QString("child: %1 z=%2 visible=%3 isLogin=%4")
                        .arg(k->metaObject()->className())
                        .arg(k->z()).arg(k->isVisible() ? "y" : "n")
                        .arg(loginItem->parent() == k || k->isAncestorOf(qobject_cast<const QQuickItem *>(loginItem)) ? "y" : "n"));
                }
                SCENE_LOG_CAT("PERF", QString("loginSuccess emitted, measuring transition"));
                auto *stamp = new QElapsedTimer();
                stamp->start();
                QObject::connect(perfWin, &QQuickWindow::frameSwapped, perfWin, [stamp](){
                    SCENE_LOG_CAT("PERF", QString("first shell frame %1ms after login").arg(stamp->elapsed()));
                    delete stamp;
                }, Qt::SingleShotConnection);
                QMetaObject::invokeMethod(loginItem, "loginSuccess", Q_ARG(QString, QStringLiteral("auto")));
                // 状态机验证: isLoggedIn 是否保持 true（排除“闪回登录界面”）
                auto logWin = [perfWin](const char *tag){
                    SCENE_LOG_CAT("PERF", QString("%1: visibility=%2 geo=%3x%4+%5+%6")
                        .arg(tag).arg(perfWin->visibility()).arg(perfWin->width())
                        .arg(perfWin->height()).arg(perfWin->x()).arg(perfWin->y()));
                };
                QTimer::singleShot(500, perfWin, [perfRoot, logWin](){
                    SCENE_LOG_CAT("PERF", QString("t+500ms isLoggedIn=%1")
                        .arg(perfRoot->property("isLoggedIn").toBool() ? "true" : "false"));
                    logWin("AFTER_LOGIN");
                });
                // 全屏 <-> 窗口 往返验证：还原几何必须与 AFTER_LOGIN 完全一致
                QTimer::singleShot(2500, perfWin, [perfWin, logWin](){ logWin("BEFORE_FS"); perfWin->showFullScreen(); });
                QTimer::singleShot(3500, perfWin, [perfWin, logWin](){ logWin("IN_FULLSCREEN"); perfWin->showNormal(); });
                QTimer::singleShot(5000, perfWin, [perfWin, logWin](){ logWin("RESTORED"); });
            });
        }
    }

    // 事件循环返回后不再拆卸 QML 引擎（见上方 engine 的注释），
    // 栈上模型在此之后按逆序正常析构
    const int exitCode = app.exec();
    SCENE_LOG_CAT("APP_EXIT", QString("exiting with code %1").arg(exitCode));
    return exitCode;
}