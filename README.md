# NAV-JEST — 三维导航装备仿真与场景管理平台

Qt 6 / QML 客户端（Quick3D 三维场景、2D 离线瓦片地图）+ 内嵌 HTTP/WebSocket 服务端
（MySQL 场景与用户数据、MBTiles 瓦片服务），另有独立的 `appEarthServer` 服务端目标。

## 目录结构

```
├── CMakeLists.txt          # 唯一构建脚本（earth_server_core 静态库 + 双可执行目标）
├── qml.qrc                 # QML/JS/图标资源清单（rcc 编译进 exe）
├── config.json             # 数据库/服务端口/机柜客户端配置（构建时复制到输出目录）
├── src/
│   ├── main.cpp            # 客户端入口：模型装配 + 内嵌服务启动 + QML 引擎
│   ├── Config.*            # config.json 读取（exe 目录优先）
│   ├── RackStatusClient.*  # 机柜状态 TCP/WebSocket 客户端
│   ├── models/             # QML 暴露的 C++ 模型（卫星/设备/场景/用户/轨道/月轨迹…）
│   ├── server/             # api（HttpServer/WsHub）/ domain（device|scene|user|tiles）/ infra（Db/PasswordHasher）
│   └── utils/              # SceneLogger（logs/scene_debug.log）等仅头文件工具
├── resources/
│   ├── qml/                # main.qml + AppShell + pages/scenario/steps 业务界面
│   ├── components/         # 可复用 QML 组件（qmldir 注册）
│   ├── models/             # 3D 模型源（大 mesh 不入 qrc，构建后复制到 bin/models/）
│   └── config|data/        # 运行时 JSON（构建后复制到 bin/config、bin/data）
├── assets/model/           # 原始 GLB/OBJ 素材（不入库）
├── deploy/mysql/           # 按配置区分的 MySQL 运行时（debug/ release/）
│                             release: 自编译 Release 版 qsqlmysql.dll + MySQL 8.0 libmysql
│                             debug:   调试版 qsqlmysqld.dll + 5.7 libmysql（沿用旧库）
├── scripts/deploy.bat      # windeployqt + MySQL + 数据文件一键部署
└── sql/                    # MySQL 建表脚本
```

## 构建（Qt 6.11.0 MSVC2022）

```bat
cmake -S . -B build/cmake -G "Visual Studio 18 2026" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:\Qt6\6.11.0\msvc2022_64
cmake --build build/cmake --config Debug --target NAV_JEST
cmake --build build/cmake --config Debug --target appEarthServer
```

构建后自动复制：`models/`（liangge、卫星 mesh）、`config.json`、`config/`、`data/`
到输出目录（见 CMakeLists 尾部 POST_BUILD 命令）。

## 部署运行

```bat
scripts\deploy.bat Debug C:\Qt6\6.11.0\msvc2022_64
build\cmake\Debug\NAV_JEST.exe
```

依赖 MySQL 5.7 服务（连接参数见 `config.json`，建表脚本在 `sql/`）。

## 冒烟测试（无窗口）

```bat
set PATH=C:\Qt6\6.11.0\msvc2022_64\bin;%PATH%
set QT_QPA_PLATFORM=offscreen
set NAVJEST_AUTOCLOSE_MS=4000
build\cmake\Debug\NAV_JEST.exe
```

登录链路自测：应用无头运行后
`curl -X POST http://127.0.0.1:18080/api/v1/login -H "Content-Type: application/json" -d "{\"username\":\"lysk\",\"password\":\"lysk123\"}"`
应返回 `{"ok":true,...}`（账号表 tb_userinfo 已并入旧 users 表账户）。
正常标准：进程退出码 0；`logs/scene_debug.log` 出现
`APP_ALIVE 500ms/2000ms` 与 `APP_EXIT exiting with code 0`；
无 `Driver not loaded`（需先部署 MySQL 插件）。
窗口化冒烟（不加 QT_QPA_PLATFORM，窗口自动开关 3.5s）额外检查 stderr：
不得出现 `is not defined`/`is not a type`/`not installed`（tiles 图片请求
失败属预期——数据文件缺失时范围外瓦片无图）。注意 `resources/models/`
下也有 QML（module models），批量改引用时勿漏。

## 注意事项

- QML 引擎与内嵌服务对象有意存活至进程退出（OS 回收）：QML 通过 context
  property 持有栈上模型裸指针，析构顺序颠倒即崩溃，勿改回栈上引擎。
- `Db` 析构只 close 不 removeDatabase（仓库持有连接副本，带副本移除连接是
  Qt 文档明示的未定义行为）。
- 新增 QML 文件需同步登记 `qml.qrc`；放入 `components/` 的还要登记其 `qmldir`。
- 数据库配置含敏感信息，`config.json` 不要提交到远端仓库。
- `Map2DView.zoomLevel` 是只读派生属性（源头是 `tileMap.zoomContinuous`），
  缩放必须写 zoomContinuous；QML 中动画（`Animation on opacity`）与静态
  绑定不能共存，新增动画时删掉静态值。
- C++ 模型经 `qt_add_qml_module(URI NavModels)` 注册为 QML_ELEMENT/QML_SINGLETON，
  QML 侧 `import NavModels` 后按类型名访问（如 `PntDataModel.simLatitude`）；
  例外：`RackStatusModelA/B`（双机柜实例）与 `CoordinateSystem`（既有 C++ 单例）
  由 main.cpp 用 `qmlRegisterSingletonInstance` 按实例注册。新增模型时在类体
  Q_OBJECT 后加 `QML_SINGLETON`/`QML_ELEMENT` 并登记 qt_add_qml_module SOURCES。
- 已知限制：窗口化 GUI 实机运行尚未在本环境验证（无头仅覆盖引导/数据/
  QML 加载路径）；qmllint 剩余 unqualified 提示为既有 id 访问风格问题。
