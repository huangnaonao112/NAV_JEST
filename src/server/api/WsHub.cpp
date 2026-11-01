#include "WsHub.h"

#include <QDateTime>
#include <QJsonDocument>

WsHub::WsHub(QObject *parent)
    : QObject(parent),
      m_server(QStringLiteral("EarthServerWs"), QWebSocketServer::NonSecureMode, this) {
    m_tick.setInterval(1000);
    connect(&m_tick, &QTimer::timeout, this, [this]() {
        QJsonObject msg;
        msg["schemaVersion"] = 1;
        msg["topic"] = "world.state";
        msg["ts"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        QJsonObject payload;
        payload["note"] = "mock";
        msg["payload"] = payload;
        broadcast(msg);
    });
}

bool WsHub::start() {
    if (!m_server.listen(QHostAddress(m_host), m_port)) {
        return false;
    }

    connect(&m_server, &QWebSocketServer::newConnection, this, &WsHub::onNewConnection);
    m_tick.start();
    return true;
}

void WsHub::onNewConnection() {
    QWebSocket *socket = m_server.nextPendingConnection();
    m_clients.insert(socket);

    connect(socket, &QWebSocket::disconnected, this, &WsHub::onSocketDisconnected);
}

void WsHub::onSocketDisconnected() {
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (!socket) return;
    m_clients.remove(socket);
    socket->deleteLater();
}

void WsHub::broadcast(const QJsonObject &msg) {
    const QString text = QString::fromUtf8(QJsonDocument(msg).toJson(QJsonDocument::Compact));
    for (QWebSocket *c : m_clients) {
        if (c) c->sendTextMessage(text);
    }
}
