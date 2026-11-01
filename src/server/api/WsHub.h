#pragma once

#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QtWebSockets/QWebSocketServer>
#include <QtWebSockets/QWebSocket>

class WsHub : public QObject {
    Q_OBJECT
public:
    explicit WsHub(QObject *parent = nullptr);

    bool start();
    void broadcast(const QJsonObject &msg);

    void setListenHost(const QString &host) { m_host = host; }
    void setListenPort(quint16 port) { m_port = port; }
    QString listenHost() const { return m_host; }
    quint16 listenPort() const { return m_port; }

private:
    void onNewConnection();
    void onSocketDisconnected();

    QString m_host = "0.0.0.0";
    quint16 m_port = 18081;

    QWebSocketServer m_server;
    QSet<QWebSocket*> m_clients;
    QTimer m_tick;
};
