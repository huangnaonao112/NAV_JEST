#include "TrajectoryLoader.h"
#include "CoordinateSystem.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QtMath>
#include <QCoreApplication>
#include <QDebug>

TrajectoryLoader::TrajectoryLoader(QObject *parent)
    : QObject(parent)
{
}

bool TrajectoryLoader::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    QJsonObject root = doc.object();

    m_trajectoryPoints.clear();
    m_phases.clear();

    // 检测坐标系统类型
    QString coordSystem = "scene";  // 默认场景坐标
    QString units = "scene_unit";
    
    if (root.contains("metadata")) {
        QJsonObject meta = root["metadata"].toObject();
        coordSystem = meta.value("coordinateSystem").toString("scene");
        units = meta.value("units").toString("scene_unit");
    }
    
    // 新格式：root.points
    if (root.contains("points") && root["points"].isArray()) {
        QJsonArray pts = root["points"].toArray();
        m_trajectoryPoints.reserve(pts.size());
        
        // 场景坐标：直接使用
        if (coordSystem == "scene" || coordSystem == "scene_unit") {
            for (const auto& point : pts) {
                QJsonObject p = point.toObject();
                double x = p["x"].toDouble();
                double y = p["y"].toDouble();
                double z = p["z"].toDouble();
                m_trajectoryPoints.append(QVector3D(x, y, z));
            }
        }
        // ECI坐标（km）：需要转换
        else if (coordSystem == "ECI" && units == "km") {
            auto* coord = CoordinateSystem::instance();
            for (const auto& point : pts) {
                QJsonObject p = point.toObject();
                double x = p["x"].toDouble();
                double y = p["y"].toDouble();
                double z = p["z"].toDouble();
                QVector3D scenePt = coord->toScene(QVector3D(x, y, z));
                m_trajectoryPoints.append(scenePt);
            }
        }
        // WGS84/ECEF坐标（km）：与ECI处理相同（简化情况下）
        else if ((coordSystem == "WGS84" || coordSystem == "ECEF") && units == "km") {
            auto* coord = CoordinateSystem::instance();
            for (const auto& point : pts) {
                QJsonObject p = point.toObject();
                double x = p["x"].toDouble();
                double y = p["y"].toDouble();
                double z = p["z"].toDouble();
                QVector3D scenePt = coord->toScene(QVector3D(x, y, z));
                m_trajectoryPoints.append(scenePt);
            }
        }
        // 未知格式：尝试直接使用
        else {
            for (const auto& point : pts) {
                QJsonObject p = point.toObject();
                double x = p["x"].toDouble();
                double y = p["y"].toDouble();
                double z = p["z"].toDouble();
                m_trajectoryPoints.append(QVector3D(x, y, z));
            }
        }

        // phases 可选：根据 phase 字段自动切分
        QString lastPhase;
        int start = 0;
        for (int i = 0; i < pts.size(); ++i) {
            const auto obj = pts[i].toObject();
            const QString ph = obj.value("phase").toString();
            if (i == 0) {
                lastPhase = ph;
                start = 0;
            } else if (ph != lastPhase) {
                Phase seg;
                seg.name = lastPhase;
                seg.startIndex = start;
                seg.endIndex = i - 1;
                m_phases.append(seg);
                lastPhase = ph;
                start = i;
            }
        }
        if (!pts.isEmpty()) {
            Phase seg;
            seg.name = lastPhase;
            seg.startIndex = start;
            seg.endIndex = pts.size() - 1;
            m_phases.append(seg);
        }

        emit pointsChanged();
        emit phasesChanged();
        return true;
    }

    // 旧格式：root.trajectory (真实km坐标) + root.phases
    QVector<QVector3D> rawPoints;
    QJsonArray trajectory = root["trajectory"].toArray();
    rawPoints.reserve(trajectory.size());
    for (const auto& point : trajectory) {
        QJsonObject p = point.toObject();
        double x = p["x"].toDouble();
        double y = p["y"].toDouble();
        double z = p["z"].toDouble();
        rawPoints.append(QVector3D(x, y, z));
    }

    auto* coord = CoordinateSystem::instance();
    m_trajectoryPoints = coord->toSceneList(rawPoints);

    QJsonArray phases = root["phases"].toArray();
    for (const auto& phase : phases) {
        QJsonObject p = phase.toObject();
        Phase ph;
        ph.name = p["name"].toString();
        ph.startIndex = p["startIndex"].toInt();
        ph.endIndex = p["endIndex"].toInt();
        m_phases.append(ph);
    }

    emit pointsChanged();
    emit phasesChanged();
    return !m_trajectoryPoints.isEmpty();
}

bool TrajectoryLoader::loadFromResource(const QString& qrcPath) {
    QString path = qrcPath;
    if (path.startsWith("qrc:/")) {
        path = path.mid(3);
    }
    if (!path.startsWith(":/")) {
        path = ":" + path;
    }
    return loadFromFile(path);
}

bool TrajectoryLoader::loadFromAppDir(const QString& fileName) {
    QString appDir = QCoreApplication::applicationDirPath();
    QString fullPath = appDir + "/" + fileName;
    return loadFromFile(fullPath);
}

QVariantList TrajectoryLoader::points() const {
    QVariantList list;
    list.reserve(m_trajectoryPoints.size());
    for (const auto& p : m_trajectoryPoints) {
        list.append(QVariant::fromValue(p));
    }
    return list;
}

QVariantList TrajectoryLoader::phasesVariant() const {
    QVariantList list;
    list.reserve(m_phases.size());
    for (const auto& ph : m_phases) {
        QVariantMap m;
        m["name"] = ph.name;
        m["startIndex"] = ph.startIndex;
        m["endIndex"] = ph.endIndex;
        list.append(m);
    }
    return list;
}

QVector<QVector3D> TrajectoryLoader::generateDemoTrajectory() {
    m_trajectoryPoints.clear();
    m_phases.clear();
    
    double earthR = CoordinateSystem::EARTH_RADIUS;
    double moonR = CoordinateSystem::MOON_RADIUS;
    double moonDist = CoordinateSystem::MOON_DISTANCE_DISPLAY;  // 压缩后的距离
    
    int totalPoints = 0;
    
    // 发射阶段（近地，保持真实比例）
    Phase phase1;
    phase1.name = "发射";
    phase1.startIndex = totalPoints;
    for (int i = 0; i < 30; ++i) {
        double t = static_cast<double>(i) / 29;
        double radius = earthR + 400.0 * t;
        double x = radius;
        double y = 50.0 * sin(M_PI * t);
        double z = 30.0 * sin(M_PI * t);
        m_trajectoryPoints.append(QVector3D(x, y, z));
    }
    phase1.endIndex = totalPoints + 29;
    totalPoints += 30;
    m_phases.append(phase1);
    
    // 近地轨道
    Phase phase2;
    phase2.name = "近地轨道";
    phase2.startIndex = totalPoints;
    for (int i = 0; i < 60; ++i) {
        double t = static_cast<double>(i) / 59;
        double angle = 2.0 * M_PI * t;
        double r = earthR + 400;
        double x = r * cos(angle);
        double z = r * sin(angle);
        double y = r * 0.2 * sin(angle);
        m_trajectoryPoints.append(QVector3D(x, y, z));
    }
    phase2.endIndex = totalPoints + 59;
    totalPoints += 60;
    m_phases.append(phase2);
    
    // 地月转移（使用压缩距离）
    Phase phase3;
    phase3.name = "地月转移";
    phase3.startIndex = totalPoints;
    double startX = earthR * 2;
    double endX = moonDist - moonR * 3;
    for (int i = 0; i < 100; ++i) {
        double t = static_cast<double>(i) / 99;
        double x = startX + (endX - startX) * t;
        double z = moonDist * 0.15 * sin(M_PI * t);
        double y = moonDist * 0.05 * sin(M_PI * t);
        m_trajectoryPoints.append(QVector3D(x, y, z));
    }
    phase3.endIndex = totalPoints + 99;
    totalPoints += 100;
    m_phases.append(phase3);
    
    // 环月轨道
    Phase phase4;
    phase4.name = "环月轨道";
    phase4.startIndex = totalPoints;
    for (int i = 0; i < 80; ++i) {
        double t = static_cast<double>(i) / 79;
        double angle = 2.0 * M_PI * t;
        double r = moonR * 2;
        double localX = r * cos(angle);
        double localZ = r * sin(angle);
        double worldX = moonDist + localX;
        double y = r * 0.2 * sin(angle * 2);
        m_trajectoryPoints.append(QVector3D(worldX, y, localZ));
    }
    phase4.endIndex = totalPoints + 79;
    totalPoints += 80;
    m_phases.append(phase4);
    
    return m_trajectoryPoints;
}