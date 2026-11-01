#include "HttpServer.h"

#include "../infra/PasswordHasher.h"
#include "utils/SceneLogger.h"
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>

static QByteArray bytesResponse(int code, const QByteArray &body, const QByteArray &contentType) {
    QByteArray res;
    res += "HTTP/1.1 " + QByteArray::number(code) + " OK\r\n";
    res += "Content-Type: " + contentType + "\r\n";
    res += "Access-Control-Allow-Origin: *\r\n";
    res += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    res += "Connection: close\r\n\r\n";
    res += body;
    return res;
}

static QByteArray jsonResponse(int code, const QJsonObject &obj) {
    QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray res;
    res += "HTTP/1.1 " + QByteArray::number(code) + " OK\r\n";
    res += "Content-Type: application/json\r\n";
    res += "Access-Control-Allow-Origin: *\r\n";
    res += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    res += "Connection: close\r\n\r\n";
    res += body;
    return res;
}

static QByteArray jsonArrayResponse(int code, const QJsonArray &arr) {
    QByteArray body = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    QByteArray res;
    res += "HTTP/1.1 " + QByteArray::number(code) + " OK\r\n";
    res += "Content-Type: application/json\r\n";
    res += "Access-Control-Allow-Origin: *\r\n";
    res += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    res += "Connection: close\r\n\r\n";
    res += body;
    return res;
}

static QByteArray notFound() {
    QJsonObject o;
    o["error"] = "not_found";
    return jsonResponse(404, o);
}

static QByteArray badRequest(const QString &msg) {
    QJsonObject o;
    o["error"] = "bad_request";
    o["message"] = msg;
    return jsonResponse(400, o);
}

static QString pathFromRequestLine(const QByteArray &line) {
    // e.g. "GET /api/v1/health HTTP/1.1"
    const QList<QByteArray> parts = line.split(' ');
    if (parts.size() < 2) return {};
    return QString::fromUtf8(parts[1]);
}

static QString methodFromRequestLine(const QByteArray &line) {
    const QList<QByteArray> parts = line.split(' ');
    if (parts.isEmpty()) return {};
    return QString::fromUtf8(parts[0]);
}

HttpServer::HttpServer(DeviceRepository &deviceRepo, UserRepository &userRepo, MbTiles &mbtiles, WsHub &wsHub, QObject *parent)
    : QObject(parent), m_deviceRepo(deviceRepo), m_userRepo(userRepo), m_mbtiles(mbtiles), m_wsHub(wsHub) {
    connect(&m_server, &QTcpServer::newConnection, this, &HttpServer::handleConnection);
}

bool HttpServer::listen(const QString &host, quint16 port) {
    return m_server.listen(QHostAddress(host), port);
}

void HttpServer::handleConnection() {
    while (m_server.hasPendingConnections()) {
        QTcpSocket *sock = m_server.nextPendingConnection();
        
        // 设置 socket 超时（30秒），防止恶意连接占用资源
        sock->setSocketOption(QAbstractSocket::LowDelayOption, 1);
        
        // 超时定时器
        QTimer *timeoutTimer = new QTimer(sock);
        timeoutTimer->setSingleShot(true);
        timeoutTimer->setInterval(30000); // 30秒超时
        
        connect(timeoutTimer, &QTimer::timeout, sock, [sock]() {
            sock->abort();
            sock->deleteLater();
        });
        
        connect(sock, &QTcpSocket::disconnected, timeoutTimer, &QTimer::stop);
        
        connect(sock, &QTcpSocket::readyRead, this, [this, sock, timeoutTimer]() {
            // 收到数据时重置超时
            timeoutTimer->start();
            
            const QByteArray raw = sock->readAll();
            const int lineEnd = raw.indexOf("\r\n");
            if (lineEnd < 0) {
                sock->write(badRequest("invalid request line"));
                sock->disconnectFromHost();
                return;
            }

            const QByteArray requestLine = raw.left(lineEnd);
            const QString method = methodFromRequestLine(requestLine);
            const QString path = pathFromRequestLine(requestLine);

            // Body (very naive parsing, sufficient for V1 demo)
            const int headerEnd = raw.indexOf("\r\n\r\n");
            QByteArray body;
            if (headerEnd >= 0) body = raw.mid(headerEnd + 4);

            if (method == "GET" && path == "/api/v1/health") {
                QJsonObject o;
                o["ok"] = true;
                o["ts"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
                sock->write(jsonResponse(200, o));
                sock->disconnectFromHost();
                return;
            }

            // POST /api/v1/login
            if (method == "POST" && path == "/api/v1/login") {
                const QJsonDocument doc = QJsonDocument::fromJson(body);
                if (!doc.isObject()) {
                    sock->write(badRequest("body must be json object"));
                    sock->disconnectFromHost();
                    return;
                }

                const QJsonObject o = doc.object();
                const QString username = o.value("username").toString().trimmed();
                const QString password = o.value("password").toString();

                if (username.isEmpty() || password.isEmpty()) {
                    QJsonObject err;
                    err["ok"] = false;
                    err["message"] = "用户名和密码不能为空";
                    sock->write(jsonResponse(401, err));
                    sock->disconnectFromHost();
                    return;
                }

                const auto users = m_userRepo.list();
                bool found = false;
                QString storedPassword;
                for (const auto &u : users) {
                    if (u.userId.compare(username, Qt::CaseInsensitive) == 0) {
                        found = true;
                        storedPassword = u.userPassword;
                        break;
                    }
                }
                // 登录审计: 只记录用户名与结果判定依据，不记录明文密码
                SCENE_LOG_CAT("LOGIN", QString("attempt user=%1 listSize=%2 found=%3 storedLen=%4 storedIsHash=%5")
                    .arg(username).arg(users.size()).arg(found)
                    .arg(storedPassword.length())
                    .arg(storedPassword.contains('$')));

                if (!found || !PasswordHasher::verify(password, storedPassword)) {
                    QJsonObject err;
                    err["ok"] = false;
                    err["message"] = "用户名或密码错误";
                    sock->write(jsonResponse(401, err));
                    sock->disconnectFromHost();
                    return;
                }

                QJsonObject ok;
                ok["ok"] = true;
                ok["username"] = username;
                sock->write(jsonResponse(200, ok));
                sock->disconnectFromHost();
                return;
            }

            // GET /tiles/{z}/{x}/{y}
            if (method == "GET" && path.startsWith("/tiles/")) {
                const QString rest = path.mid(QString("/tiles/").size());
                const QStringList parts = rest.split('/', Qt::SkipEmptyParts);
                if (parts.size() != 3) {
                    sock->write(badRequest("invalid tile path"));
                    sock->disconnectFromHost();
                    return;
                }

                bool okZ=false, okX=false, okY=false;
                const int z = parts[0].toInt(&okZ);
                const int x = parts[1].toInt(&okX);
                QString yPart = parts[2];
                // strip extension if any (.png/.jpg)
                const int dot = yPart.indexOf('.');
                if (dot > 0) yPart = yPart.left(dot);
                const int y = yPart.toInt(&okY);
                if (!okZ || !okX || !okY) {
                    sock->write(badRequest("z/x/y must be integers"));
                    sock->disconnectFromHost();
                    return;
                }

                QByteArray data;
                QString mime;
                if (!m_mbtiles.getTile(z, x, y, data, mime)) {
                    sock->write(notFound());
                    sock->disconnectFromHost();
                    return;
                }

                sock->write(bytesResponse(200, data, mime.toUtf8()));
                sock->disconnectFromHost();
                return;
            }

            if (path == "/api/v1/devices" && method == "GET") {
                QJsonArray arr;
                const auto list = m_deviceRepo.list();
                for (const auto &d : list) {
                    QJsonObject o;
                    o["id"] = d.id;
                    o["name"] = d.name;
                    o["type"] = d.type;
                    o["model"] = d.model;
                    o["vendor"] = d.vendor;
                    o["connection"] = d.connection;
                    o["address"] = d.address;
                    o["updatedAt"] = d.updatedAt.toString(Qt::ISODateWithMs);
                    arr.push_back(o);
                }
                sock->write(jsonArrayResponse(200, arr));
                sock->disconnectFromHost();
                return;
            }

            if (path == "/api/v1/devices" && method == "POST") {
                const QJsonDocument doc = QJsonDocument::fromJson(body);
                if (!doc.isObject()) {
                    sock->write(badRequest("body must be json object"));
                    sock->disconnectFromHost();
                    return;
                }
                const QJsonObject o = doc.object();

                DeviceRepository::Device d;
                d.id = o.value("id").toString().trimmed();
                if (d.id.isEmpty()) {
                    sock->write(badRequest("id required"));
                    sock->disconnectFromHost();
                    return;
                }
                d.name = o.value("name").toString();
                d.type = o.value("type").toString();
                d.model = o.value("model").toString();
                d.vendor = o.value("vendor").toString();
                d.connection = o.value("connection").toString();
                d.address = o.value("address").toString();
                d.updatedAt = QDateTime::currentDateTimeUtc();

                if (m_deviceRepo.exists(d.id)) {
                    sock->write(badRequest("device already exists"));
                    sock->disconnectFromHost();
                    return;
                }

                if (!m_deviceRepo.insert(d)) {
                    sock->write(badRequest("insert failed"));
                    sock->disconnectFromHost();
                    return;
                }

                sock->write(jsonResponse(200, QJsonObject{{"ok", true}}));
                sock->disconnectFromHost();
                return;
            }

            // PUT /api/v1/devices/{id}
            if (path.startsWith("/api/v1/devices/") && method == "PUT") {
                const QString id = path.mid(QString("/api/v1/devices/").size()).trimmed();
                if (id.isEmpty()) {
                    sock->write(badRequest("id required"));
                    sock->disconnectFromHost();
                    return;
                }

                const QJsonDocument doc = QJsonDocument::fromJson(body);
                if (!doc.isObject()) {
                    sock->write(badRequest("body must be json object"));
                    sock->disconnectFromHost();
                    return;
                }
                const QJsonObject o = doc.object();

                DeviceRepository::Device d;
                d.id = id;
                d.name = o.value("name").toString();
                d.type = o.value("type").toString();
                d.model = o.value("model").toString();
                d.vendor = o.value("vendor").toString();
                d.connection = o.value("connection").toString();
                d.address = o.value("address").toString();
                d.updatedAt = QDateTime::currentDateTimeUtc();

                if (!m_deviceRepo.exists(id)) {
                    sock->write(notFound());
                    sock->disconnectFromHost();
                    return;
                }

                if (!m_deviceRepo.update(id, d)) {
                    sock->write(badRequest("update failed"));
                    sock->disconnectFromHost();
                    return;
                }

                sock->write(jsonResponse(200, QJsonObject{{"ok", true}}));
                sock->disconnectFromHost();
                return;
            }

            // DELETE /api/v1/devices/{id}
            if (path.startsWith("/api/v1/devices/") && method == "DELETE") {
                const QString id = path.mid(QString("/api/v1/devices/").size()).trimmed();
                if (id.isEmpty()) {
                    sock->write(badRequest("id required"));
                    sock->disconnectFromHost();
                    return;
                }

                if (!m_deviceRepo.exists(id)) {
                    sock->write(notFound());
                    sock->disconnectFromHost();
                    return;
                }

                if (!m_deviceRepo.remove(id)) {
                    sock->write(badRequest("delete failed"));
                    sock->disconnectFromHost();
                    return;
                }

                sock->write(jsonResponse(200, QJsonObject{{"ok", true}}));
                sock->disconnectFromHost();
                return;
            }

            sock->write(notFound());
            sock->disconnectFromHost();
        });

        connect(sock, &QTcpSocket::disconnected, sock, &QObject::deleteLater);
    }
}
