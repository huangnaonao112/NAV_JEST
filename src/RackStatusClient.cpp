#include "RackStatusClient.h"
#include "models/RackStatusModel.h"
#include "models/PntDataModel.h"
#include "models/AntLightModel.h"
#include "models/MoonTrajectoryModel.h"
#include "models/SatelliteModel.h"
#include "models/OrbitMessage.h"
#include "models/ProbeMissionModel.h"
#include "models/TransferOrbitModel.h"

#include <QRegularExpression>
#include <QDebug>
#include "utils/SceneLogger.h"

RackStatusClient::RackStatusClient(QObject *parent) : QObject(parent) {
    // 设置重连定时器 - 每3秒尝试重连一次
    m_reconnectTimer.setInterval(3000);
    m_reconnectTimer.setSingleShot(false);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &RackStatusClient::tryReconnect);
    
    connect(&m_sock, &QTcpSocket::connected, this, [this]() {
        setConnected(true);
        emit logLine(QStringLiteral("RackStatusClient connected to %1:%2").arg(m_host).arg(m_port));
    });
    connect(&m_sock, &QTcpSocket::disconnected, this, [this]() {
        setConnected(false);
        emit logLine(QStringLiteral("RackStatusClient disconnected"));
        // 如果启用了自动重连，断开后自动尝试重连
        if (m_autoReconnect && !m_reconnectTimer.isActive()) {
            m_reconnectTimer.start();
        }
    });
    connect(&m_sock, &QTcpSocket::readyRead, this, &RackStatusClient::onReadyRead);
    connect(&m_sock, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit logLine(QStringLiteral("RackStatusClient connection failed: %1").arg(m_sock.errorString()));
        setConnected(false);
        // 如果启用了自动重连，错误后自动尝试重连
        if (m_autoReconnect && !m_reconnectTimer.isActive()) {
            m_reconnectTimer.start();
        }
    });
}

void RackStatusClient::setHost(const QString &h) {
    if (m_host == h) return;
    m_host = h;
    emit hostChanged();
}

void RackStatusClient::setPort(int p) {
    if (m_port == p) return;
    m_port = p;
    emit portChanged();
}

void RackStatusClient::setModels(RackStatusModel *rackA, RackStatusModel *rackB) {
    m_rackA = rackA;
    m_rackB = rackB;
}

void RackStatusClient::setPntModel(PntDataModel *pntModel) {
    m_pntModel = pntModel;
}

void RackStatusClient::setAntLightModel(AntLightModel *antLightModel) {
    m_antLightModel = antLightModel;
}

void RackStatusClient::setMoonTrajectoryModel(MoonTrajectoryModel *moonTrajectoryModel) {
    m_moonTrajectoryModel = moonTrajectoryModel;
}

void RackStatusClient::setSatelliteModel(SatelliteModel *satelliteModel) {
    m_satelliteModel = satelliteModel;
}

void RackStatusClient::setProbeMissionModel(ProbeMissionModel *probeMissionModel) {
    m_probeMissionModel = probeMissionModel;
}

void RackStatusClient::setTransferOrbitModel(TransferOrbitModel *transferOrbitModel) {
    m_transferOrbitModel = transferOrbitModel;
}

void RackStatusClient::connectToServer() {
    if (m_sock.state() == QAbstractSocket::ConnectedState || m_sock.state() == QAbstractSocket::ConnectingState) return;
    m_buffer.clear();
    m_sock.connectToHost(m_host, static_cast<quint16>(m_port));
}

void RackStatusClient::disconnectFromServer() {
    m_autoReconnect = false;
    m_reconnectTimer.stop();
    m_sock.disconnectFromHost();
}

void RackStatusClient::startAutoReconnect() {
    m_autoReconnect = true;
    // 立即尝试连接
    connectToServer();
    // 启动重连定时器
    if (!m_reconnectTimer.isActive()) {
        m_reconnectTimer.start();
    }
    emit logLine(QStringLiteral("RackStatusClient auto-reconnect enabled, connecting to %1:%2").arg(m_host).arg(m_port));
}

void RackStatusClient::stopAutoReconnect() {
    m_autoReconnect = false;
    m_reconnectTimer.stop();
    emit logLine(QStringLiteral("RackStatusClient auto-reconnect disabled"));
}

void RackStatusClient::tryReconnect() {
    // 如果已连接，停止重连定时器
    if (m_sock.state() == QAbstractSocket::ConnectedState) {
        m_reconnectTimer.stop();
        return;
    }
    // 如果正在连接中，跳过本次
    if (m_sock.state() == QAbstractSocket::ConnectingState) {
        return;
    }
    // 尝试连接
    connectToServer();
}

void RackStatusClient::setConnected(bool v) {
    if (m_connected == v) return;
    m_connected = v;
    emit connectedChanged();
    
    // 连接成功后停止重连定时器
    if (v && m_reconnectTimer.isActive()) {
        m_reconnectTimer.stop();
    }
}

void RackStatusClient::onReadyRead() {
    m_buffer.append(m_sock.readAll());

    int idx;
    while ((idx = m_buffer.indexOf('\n')) >= 0) {
        QByteArray line = m_buffer.left(idx);
        m_buffer.remove(0, idx + 1);
        line = line.trimmed();
        if (!line.isEmpty()) processLine(line);
    }

    if (m_buffer.size() > 16 * 1024) {
        m_buffer = m_buffer.right(4 * 1024);
    }
}

void RackStatusClient::processLine(const QByteArray &line) {
    if (!line.startsWith('$')) return;

    const int star = line.indexOf('*');
    if (star < 0) return;

    const QByteArray payload = line.mid(1, star - 1);

    // 判断报文类型
    if (payload.startsWith("DEV,")) {
        QVector<int> states;
        QString err;
        if (!parseDevFrame(payload, states, err)) {
            emit logLine(QStringLiteral("RackStatusClient DEV parse error: %1").arg(err));
            return;
        }

        // 18个状态：前9个给机柜A，后9个给机柜B
        const int half = states.size() / 2;
        if (m_rackA && half > 0) {
            QVector<int> statesA(states.mid(0, half));
            m_rackA->updateStates(statesA);
        }
        if (m_rackB && states.size() > half) {
            QVector<int> statesB(states.mid(half));
            m_rackB->updateStates(statesB);
        }
    } else if (payload.startsWith("PNT,")) {
        double simLat, simLon, simAlt;
        double dutLat, dutLon, dutAlt;
        double simVx, simVy, simVz;
        double dutVx, dutVy, dutVz;
        QString err;
        
        if (!parsePntFrame(payload, simLat, simLon, simAlt, dutLat, dutLon, dutAlt,
                           simVx, simVy, simVz, dutVx, dutVy, dutVz, err)) {
            emit logLine(QStringLiteral("RackStatusClient PNT parse error: %1").arg(err));
            return;
        }
        
        if (m_pntModel) {
            m_pntModel->updateFromPnt(simLat, simLon, simAlt, dutLat, dutLon, dutAlt,
                                      simVx, simVy, simVz, dutVx, dutVy, dutVz);
        }
    } else if (payload.startsWith("ANT,")) {
        // ANT暗室灯光状态报文
        QVector<int> states;
        QString err;
        if (!parseAntFrame(payload, states, err)) {
            emit logLine(QStringLiteral("RackStatusClient ANT parse error: %1").arg(err));
            return;
        }
        
        if (m_antLightModel) {
            m_antLightModel->updateStates(states);
        }
    } else if (payload.startsWith("TRJ,")) {
        // TRJ 3D轨迹报文
        double x, y, z;
        QString err;
        if (!parseTrajFrame(payload, x, y, z, err)) {
            emit logLine(QStringLiteral("RackStatusClient TRJ parse error: %1").arg(err));
            return;
        }
        
        if (m_pntModel) {
            m_pntModel->updateFromTraj(x, y, z);
        }
    } else if (payload.startsWith("ORB,")) {
        // ORB 轨道参数报文（外部解算器给出开普勒根数）→ 真实轨道显示
        OrbitMessage::Frame frame;
        QString err;
        if (!OrbitMessage::parseOrbitFrame(payload, frame, err)) {
            SCENE_LOG_ERROR("ORB", QString("parse error: %1 (len=%2)").arg(err).arg(payload.size()));
            emit logLine(QStringLiteral("RackStatusClient ORB parse error: %1").arg(err));
            return;
        }
        if (m_satelliteModel) {
            m_satelliteModel->applyOrbitFrame(frame);
        }
        SCENE_LOG_CAT("ORB", QString("received %1 sats @ week %2 tow %3")
                      .arg(frame.satellites.size()).arg(frame.week)
                      .arg(frame.secondsOfWeek, 0, 'f', 1));
    } else if (payload.startsWith("SAT,")) {
        // SAT 卫星星下点报文
        int count;
        QStringList prns;
        QVector<double> lats, lons;
        QString err;
        if (!parseSatFrame(payload, count, prns, lats, lons, err)) {
            emit logLine(QStringLiteral("RackStatusClient SAT parse error: %1").arg(err));
            return;
        }
        
        if (m_pntModel) {
            m_pntModel->updateFromSat(count, prns, lats, lons);
        }
    } else if (payload.startsWith("MNT,")) {
        // MNT 月球轨迹报文
        double lat, lon, alt;
        QString err;
        if (!parseMoonTrajFrame(payload, lat, lon, alt, err)) {
            emit logLine(QStringLiteral("RackStatusClient MNT parse error: %1").arg(err));
            return;
        }

        if (m_moonTrajectoryModel) {
            m_moonTrajectoryModel->updateFromMoonTraj(lat, lon, alt);
        }
    } else if (payload.startsWith("JAM,")) {
        // JAM 干扰机报文（单个干扰机）
        double lat, lon, radius, direction;
        QString err;
        if (!parseJamFrame(payload, lat, lon, radius, direction, err)) {
            emit logLine(QStringLiteral("RackStatusClient JAM parse error: %1").arg(err));
            return;
        }

        if (m_pntModel) {
            m_pntModel->updateFromJam(lat, lon, radius, direction);
        }
    } else if (payload.startsWith("DUT,")) {
        // DUT 转台和待测设备姿态报文（12个参数）
        double ttRoll, ttRollSpeed, ttPitch, ttPitchSpeed, ttYaw, ttYawSpeed;
        double dutRoll, dutRollSpeed, dutPitch, dutPitchSpeed, dutYaw, dutYawSpeed;
        QString err;
        if (!parseDutFrame(payload, ttRoll, ttRollSpeed, ttPitch, ttPitchSpeed, ttYaw, ttYawSpeed, dutRoll, dutRollSpeed, dutPitch, dutPitchSpeed, dutYaw, dutYawSpeed, err)) {
            emit logLine(QStringLiteral("RackStatusClient DUT parse error: %1").arg(err));
            return;
        }

        if (m_pntModel) {
            m_pntModel->updateAttitude(ttRoll, ttRollSpeed, ttPitch, ttPitchSpeed, ttYaw, ttYawSpeed, dutRoll, dutRollSpeed, dutPitch, dutPitchSpeed, dutYaw, dutYawSpeed);
        }
    } else if (payload.startsWith("BEG,")) {
        // BEG 测试开始报文
        int testType;
        QString err;
        if (!parseBegFrame(payload, testType, err)) {
            emit logLine(QStringLiteral("RackStatusClient BEG parse error: %1").arg(err));
            return;
        }

        if (m_pntModel) {
            m_pntModel->setTestType(testType);
            m_pntModel->setTaskStatus("任务中");
        }
        emit logLine(QStringLiteral("RackStatusClient: 测试开始, 类型=%1").arg(testType));
    } else if (payload.startsWith("END,")) {
        // END 测试结束报文
        int testType;
        QString err;
        if (!parseEndFrame(payload, testType, err)) {
            emit logLine(QStringLiteral("RackStatusClient END parse error: %1").arg(err));
            return;
        }

        // 清除所有可视化数据
        if (m_pntModel) {
            m_pntModel->setTaskStatus("任务结束");
            m_pntModel->clearData();
        }
        if (m_moonTrajectoryModel) {
            m_moonTrajectoryModel->clear();
        }
        if (m_satelliteModel) {
            m_satelliteModel->clear();
        }
        if (m_antLightModel) {
            m_antLightModel->clearAll();
        }
        if (m_rackA) {
            m_rackA->clearAll();
        }
        if (m_rackB) {
            m_rackB->clearAll();
        }
        if (m_probeMissionModel) {
            m_probeMissionModel->resetMission();
        }
        if (m_transferOrbitModel) {
            m_transferOrbitModel->clear();
        }
        emit logLine(QStringLiteral("RackStatusClient: 测试结束, 类型=%1, 数据已清除").arg(testType));
    }
}

bool RackStatusClient::parseDevFrame(const QByteArray &payload, QVector<int> &states, QString &error) {
    // payload: DEV,101830.000,0,0,...
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 3) {
        error = QStringLiteral("too few fields");
        return false;
    }
    if (parts[0] != "DEV") {
        error = QStringLiteral("unsupported head=%1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    // parts[1] is time like 101830.000, keep but ignore.
    states.clear();
    for (int i = 2; i < parts.size(); i++) {
        bool ok = false;
        const int v = QString::fromUtf8(parts[i]).trimmed().toInt(&ok);
        if (!ok) {
            error = QStringLiteral("bad int at field %1").arg(i);
            return false;
        }
        states.push_back(v);
    }
    return true;
}

bool RackStatusClient::parsePntFrame(const QByteArray &payload,
                                      double &simLat, double &simLon, double &simAlt,
                                      double &dutLat, double &dutLon, double &dutAlt,
                                      double &simVx, double &simVy, double &simVz,
                                      double &dutVx, double &dutVy, double &dutVz,
                                      QString &error) {
    // payload: PNT,HHMMSS.SSS,simLat,simLon,simAlt,dutLat,dutLon,dutAlt,simVx,simVy,simVz,dutVx,dutVy,dutVz
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 14) {
        error = QStringLiteral("PNT frame requires 14 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "PNT") {
        error = QStringLiteral("expected PNT, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }
    
    bool ok = false;
    // parts[1] is time, skip
    simLat = QString::fromUtf8(parts[2]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad simLat"); return false; }
    
    simLon = QString::fromUtf8(parts[3]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad simLon"); return false; }
    
    simAlt = QString::fromUtf8(parts[4]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad simAlt"); return false; }
    
    dutLat = QString::fromUtf8(parts[5]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutLat"); return false; }
    
    dutLon = QString::fromUtf8(parts[6]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutLon"); return false; }
    
    dutAlt = QString::fromUtf8(parts[7]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutAlt"); return false; }
    
    simVx = QString::fromUtf8(parts[8]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad simVx"); return false; }
    
    simVy = QString::fromUtf8(parts[9]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad simVy"); return false; }
    
    simVz = QString::fromUtf8(parts[10]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad simVz"); return false; }
    
    dutVx = QString::fromUtf8(parts[11]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutVx"); return false; }
    
    dutVy = QString::fromUtf8(parts[12]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutVy"); return false; }
    
    dutVz = QString::fromUtf8(parts[13]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutVz"); return false; }
    
    return true;
}

bool RackStatusClient::parseAntFrame(const QByteArray &payload, QVector<int> &states, QString &error) {
    // payload: ANT,s1,s2,...,s36
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 37) {  // ANT + 36个状态
        error = QStringLiteral("ANT frame requires 37 fields (ANT + 36 states), got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "ANT") {
        error = QStringLiteral("expected ANT, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    states.clear();
    for (int i = 1; i < parts.size() && states.size() < 36; i++) {
        bool ok = false;
        const int v = QString::fromUtf8(parts[i]).trimmed().toInt(&ok);
        if (!ok) {
            error = QStringLiteral("bad int at field %1").arg(i);
            return false;
        }
        if (v < 0 || v > 2) {
            error = QStringLiteral("invalid state value %1 at field %2 (must be 0, 1, or 2)").arg(v).arg(i);
            return false;
        }
        states.push_back(v);
    }
    
    if (states.size() != 36) {
        error = QStringLiteral("expected 36 states, got %1").arg(states.size());
        return false;
    }
    
    return true;
}

bool RackStatusClient::parseTrajFrame(const QByteArray &payload, double &x, double &y, double &z, QString &error) {
    // payload: TRJ,HHMMSS.SSS,x,y,z
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 5) {
        error = QStringLiteral("TRJ frame requires 5 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "TRJ") {
        error = QStringLiteral("expected TRJ, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }
    
    bool ok = false;
    // parts[1] is time, skip
    x = QString::fromUtf8(parts[2]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad x"); return false; }
    
    y = QString::fromUtf8(parts[3]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad y"); return false; }
    
    z = QString::fromUtf8(parts[4]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad z"); return false; }
    
    return true;
}

bool RackStatusClient::parseSatFrame(const QByteArray &payload, int &count, QStringList &prns, QVector<double> &lats, QVector<double> &lons, QString &error) {
    // payload: SAT,HHMMSS.SSS,count,PRN1,lat1,lon1,PRN2,lat2,lon2...
    QList<QByteArray> parts = payload.split(',');
    
    if (parts.size() < 3) {
        error = QStringLiteral("SAT frame requires at least 3 fields");
        return false;
    }
    if (parts[0] != "SAT") {
        error = QStringLiteral("expected SAT, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }
    
    bool ok = false;
    // parts[1] is time, skip
    count = QString::fromUtf8(parts[2]).trimmed().toInt(&ok);
    if (!ok) { error = QStringLiteral("bad count"); return false; }
    
    // 检查字段数量: 3 (SAT,time,count) + 3*count (PRN,lat,lon)
    int expectedFields = 3 + 3 * count;
    if (parts.size() < expectedFields) {
        error = QStringLiteral("SAT frame requires %1 fields, got %2").arg(expectedFields).arg(parts.size());
        return false;
    }
    
    prns.clear();
    lats.clear();
    lons.clear();
    
    for (int i = 0; i < count; i++) {
        int baseIdx = 3 + i * 3;
        
        QString prn = QString::fromUtf8(parts[baseIdx]).trimmed();
        prns.append(prn);
        
        double lat = QString::fromUtf8(parts[baseIdx + 1]).trimmed().toDouble(&ok);
        if (!ok) { error = QStringLiteral("bad lat at satellite %1").arg(i + 1); return false; }
        lats.append(lat);
        
        double lon = QString::fromUtf8(parts[baseIdx + 2]).trimmed().toDouble(&ok);
        if (!ok) { error = QStringLiteral("bad lon at satellite %1").arg(i + 1); return false; }
        lons.append(lon);
    }
    
    return true;
}

bool RackStatusClient::parseMoonTrajFrame(const QByteArray &payload, double &lat, double &lon, double &alt, QString &error) {
    // payload: MNT,HHMMSS.SSS,lat,lon,alt
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 5) {
        error = QStringLiteral("MNT frame requires 5 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "MNT") {
        error = QStringLiteral("expected MNT, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    bool ok = false;
    // parts[1] is time, skip
    lat = QString::fromUtf8(parts[2]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad lat"); return false; }

    // 验证纬度范围 (-90 ~ +90)
    if (lat < -90.0 || lat > 90.0) {
        error = QStringLiteral("lat out of range (-90 to 90): %1").arg(lat);
        return false;
    }

    lon = QString::fromUtf8(parts[3]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad lon"); return false; }

    // 验证经度范围 (-180 ~ +180)
    if (lon < -180.0 || lon > 180.0) {
        error = QStringLiteral("lon out of range (-180 to 180): %1").arg(lon);
        return false;
    }

    alt = QString::fromUtf8(parts[4]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad alt"); return false; }

    return true;
}

bool RackStatusClient::parseJamFrame(const QByteArray &payload, double &lat, double &lon, double &radius, double &direction, QString &error) {
    // payload: JAM,HHMMSS.SSS,lat,lon,radius,direction
    QList<QByteArray> parts = payload.split(',');

    if (parts.size() < 6) {
        error = QStringLiteral("JAM frame requires 6 fields (JAM,time,lat,lon,radius,direction), got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "JAM") {
        error = QStringLiteral("expected JAM, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    bool ok = false;
    // parts[1] is time, skip
    lat = QString::fromUtf8(parts[2]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad lat"); return false; }

    lon = QString::fromUtf8(parts[3]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad lon"); return false; }

    radius = QString::fromUtf8(parts[4]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad radius"); return false; }

    direction = QString::fromUtf8(parts[5]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad direction"); return false; }

    return true;
}


bool RackStatusClient::parseDutFrame(const QByteArray &payload, double &ttRoll, double &ttRollSpeed, double &ttPitch, double &ttPitchSpeed, double &ttYaw, double &ttYawSpeed,
                                     double &dutRoll, double &dutRollSpeed, double &dutPitch, double &dutPitchSpeed, double &dutYaw, double &dutYawSpeed, QString &error) {
    // payload: DUT,HHMMSS.SSS,ttRoll,ttRollSpeed,ttPitch,ttPitchSpeed,ttYaw,ttYawSpeed,dutRoll,dutRollSpeed,dutPitch,dutPitchSpeed,dutYaw,dutYawSpeed
    QList<QByteArray> parts = payload.split(',');

    if (parts.size() < 14) {
        error = QStringLiteral("DUT frame requires 14 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "DUT") {
        error = QStringLiteral("expected DUT, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    bool ok = false;
    // parts[1] is time, skip
    // 转台姿态 (fields 2-7)
    ttRoll = QString::fromUtf8(parts[2]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad ttRoll"); return false; }

    ttRollSpeed = QString::fromUtf8(parts[3]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad ttRollSpeed"); return false; }

    ttPitch = QString::fromUtf8(parts[4]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad ttPitch"); return false; }

    ttPitchSpeed = QString::fromUtf8(parts[5]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad ttPitchSpeed"); return false; }

    ttYaw = QString::fromUtf8(parts[6]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad ttYaw"); return false; }

    ttYawSpeed = QString::fromUtf8(parts[7]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad ttYawSpeed"); return false; }

    // 待测设备姿态 (fields 8-13)
    dutRoll = QString::fromUtf8(parts[8]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutRoll"); return false; }

    dutRollSpeed = QString::fromUtf8(parts[9]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutRollSpeed"); return false; }

    dutPitch = QString::fromUtf8(parts[10]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutPitch"); return false; }

    dutPitchSpeed = QString::fromUtf8(parts[11]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutPitchSpeed"); return false; }

    dutYaw = QString::fromUtf8(parts[12]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutYaw"); return false; }

    dutYawSpeed = QString::fromUtf8(parts[13]).trimmed().toDouble(&ok);
    if (!ok) { error = QStringLiteral("bad dutYawSpeed"); return false; }

    return true;
}

bool RackStatusClient::parseBegFrame(const QByteArray &payload, int &testType, QString &error) {
    // payload: BEG,HHMMSS.SSS,testType
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 3) {
        error = QStringLiteral("BEG frame requires 3 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "BEG") {
        error = QStringLiteral("expected BEG, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    bool ok = false;
    // parts[1] is time, skip
    testType = QString::fromUtf8(parts[2]).trimmed().toInt(&ok);
    if (!ok) { error = QStringLiteral("bad testType"); return false; }

    return true;
}

bool RackStatusClient::parseEndFrame(const QByteArray &payload, int &testType, QString &error) {
    // payload: END,HHMMSS.SSS,testType
    QList<QByteArray> parts = payload.split(',');
    if (parts.size() < 3) {
        error = QStringLiteral("END frame requires 3 fields, got %1").arg(parts.size());
        return false;
    }
    if (parts[0] != "END") {
        error = QStringLiteral("expected END, got %1").arg(QString::fromUtf8(parts[0]));
        return false;
    }

    bool ok = false;
    // parts[1] is time, skip
    testType = QString::fromUtf8(parts[2]).trimmed().toInt(&ok);
    if (!ok) { error = QStringLiteral("bad testType"); return false; }

    return true;
}
