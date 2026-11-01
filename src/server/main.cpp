#include <QCoreApplication>
#include <QCommandLineParser>

#include "api/HttpServer.h"
#include "api/WsHub.h"
#include "infra/Db.h"
#include "domain/device/DeviceRepository.h"
#include "domain/user/UserRepository.h"
#include "domain/tiles/MbTiles.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("EarthServer");

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption hostOpt({"H", "host"}, "Listen host", "host", "0.0.0.0");
    QCommandLineOption portOpt({"p", "port"}, "Listen port", "port", "18080");
    parser.addOption(hostOpt);
    parser.addOption(portOpt);

    parser.process(app);

    const QString host = parser.value(hostOpt);
    const quint16 port = parser.value(portOpt).toUShort();

    // Default mbtiles path (can be replaced by config later)
    const QString mbtilesPath = QCoreApplication::applicationDirPath() + "/中国高德卫星03071704.mbtiles";

    Db db;
    if (!db.open()) {
        return 1;
    }

    DeviceRepository deviceRepo(db.database());
    deviceRepo.init();

    UserRepository userRepo(db.database());
    userRepo.init();

    MbTiles mbtiles(mbtilesPath);

    WsHub wsHub;
    wsHub.setListenHost(host);
    wsHub.setListenPort(static_cast<quint16>(port + 1));
    wsHub.start();

    HttpServer httpServer(deviceRepo, userRepo, mbtiles, wsHub);
    if (!httpServer.listen(host, port)) {
        return 2;
    }

    return app.exec();
}
