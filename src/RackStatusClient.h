#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QTcpSocket>
#include <QTimer>

class RackStatusModel;
class PntDataModel;
class AntLightModel;
class MoonTrajectoryModel;
class SatelliteModel;
class ProbeMissionModel;
class TransferOrbitModel;

class RackStatusClient : public QObject {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
public:
    explicit RackStatusClient(QObject *parent = nullptr);

    QString host() const { return m_host; }
    void setHost(const QString &h);

    int port() const { return m_port; }
    void setPort(int p);

    bool connected() const { return m_connected; }

    void setModels(RackStatusModel *rackA, RackStatusModel *rackB);
    void setPntModel(PntDataModel *pntModel);
    void setAntLightModel(AntLightModel *antLightModel);
    void setMoonTrajectoryModel(MoonTrajectoryModel *moonTrajectoryModel);
    void setSatelliteModel(SatelliteModel *satelliteModel);
    void setProbeMissionModel(ProbeMissionModel *probeMissionModel);
    void setTransferOrbitModel(TransferOrbitModel *transferOrbitModel);

    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();
    Q_INVOKABLE void startAutoReconnect();   // 启动自动重连
    Q_INVOKABLE void stopAutoReconnect();    // 停止自动重连

signals:
    void hostChanged();
    void portChanged();
    void connectedChanged();
    void logLine(const QString &line);

private:
    void setConnected(bool v);
    void onReadyRead();
    void processLine(const QByteArray &line);
    void tryReconnect();  // 尝试重连
    static bool parseDevFrame(const QByteArray &payload, QVector<int> &states, QString &error);
    static bool parsePntFrame(const QByteArray &payload, 
                               double &simLat, double &simLon, double &simAlt,
                               double &dutLat, double &dutLon, double &dutAlt,
                               double &simVx, double &simVy, double &simVz,
                               double &dutVx, double &dutVy, double &dutVz,
                               QString &error);
    static bool parseAntFrame(const QByteArray &payload, QVector<int> &states, QString &error);
    static bool parseTrajFrame(const QByteArray &payload, double &x, double &y, double &z, QString &error);
    static bool parseSatFrame(const QByteArray &payload, int &count, QStringList &prns, QVector<double> &lats, QVector<double> &lons, QString &error);
    static bool parseMoonTrajFrame(const QByteArray &payload, double &lat, double &lon, double &alt, QString &error);
    static bool parseJamFrame(const QByteArray &payload, double &lat, double &lon, double &radius, double &direction, QString &error);
    static bool parseDutFrame(const QByteArray &payload, double &ttRoll, double &ttRollSpeed, double &ttPitch, double &ttPitchSpeed, double &ttYaw, double &ttYawSpeed,
                              double &dutRoll, double &dutRollSpeed, double &dutPitch, double &dutPitchSpeed, double &dutYaw, double &dutYawSpeed, QString &error);
    static bool parseBegFrame(const QByteArray &payload, int &testType, QString &error);
    static bool parseEndFrame(const QByteArray &payload, int &testType, QString &error);

    QTcpSocket m_sock;
    QByteArray m_buffer;
    QTimer m_reconnectTimer;  // 重连定时器

    QString m_host = QStringLiteral("127.0.0.1");
    int m_port = 8000;
    bool m_connected = false;
    bool m_autoReconnect = false;  // 是否启用自动重连

    RackStatusModel *m_rackA = nullptr;
    RackStatusModel *m_rackB = nullptr;
    PntDataModel *m_pntModel = nullptr;
    AntLightModel *m_antLightModel = nullptr;
    MoonTrajectoryModel *m_moonTrajectoryModel = nullptr;
    SatelliteModel *m_satelliteModel = nullptr;
    ProbeMissionModel *m_probeMissionModel = nullptr;
    TransferOrbitModel *m_transferOrbitModel = nullptr;
};
