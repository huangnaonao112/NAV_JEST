#pragma once

#include <QObject>
#include <QTcpServer>

#include "../domain/device/DeviceRepository.h"
#include "../domain/user/UserRepository.h"
#include "../domain/tiles/MbTiles.h"
#include "WsHub.h"

class HttpServer : public QObject {
    Q_OBJECT
public:
    explicit HttpServer(DeviceRepository &deviceRepo, UserRepository &userRepo, MbTiles &mbtiles, WsHub &wsHub, QObject *parent = nullptr);

    bool listen(const QString &host, quint16 port);

private:
    void handleConnection();

    DeviceRepository &m_deviceRepo;
    UserRepository &m_userRepo;
    MbTiles &m_mbtiles;
    WsHub &m_wsHub;
    QTcpServer m_server;
};