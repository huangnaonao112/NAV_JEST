#include "ProbeMissionModel.h"
#include "CoordinateSystem.h"
#include <QtMath>

ProbeMissionModel::ProbeMissionModel(QObject *parent)
    : QObject(parent)
{
}

QString ProbeMissionModel::currentPhaseName() const {
    const auto& phases = m_loader.phases();
    if (m_currentPhase >= 0 && m_currentPhase < phases.size()) {
        return phases[m_currentPhase].name;
    }
    return "完成";
}

bool ProbeMissionModel::loadTrajectory(const QString& filePath) {
    bool ok = false;
    if (filePath.startsWith("qrc:/") || filePath.startsWith(":/")) {
        ok = m_loader.loadFromResource(filePath);
    } else {
        ok = m_loader.loadFromFile(filePath);
    }

    if (ok) {
        m_trajectoryData = m_loader.trajectoryPoints();
        return true;
    }
    return false;
}

void ProbeMissionModel::generateDemoTrajectory() {
    m_trajectoryData = m_loader.generateDemoTrajectory();
}

void ProbeMissionModel::startMission() {
    if (m_trajectoryData.isEmpty()) {
        generateDemoTrajectory();
    }
    
    m_elapsedTime = 0;
    m_currentPhase = 0;
    m_isRunning = true;
    m_lastDrawnIndex = -1;
    
    if (!m_trajectoryData.isEmpty()) {
        m_probePosition = m_trajectoryData.first();
    }
    m_altitude = 0;
    m_velocity = 0;
    
    m_drawnTrajectoryPoints.clear();
    
    emit isRunningChanged();
    emit phaseChanged();
    emit positionChanged();
    emit elapsedTimeChanged();
    emit drawnTrajectoryChanged();
}

void ProbeMissionModel::resetMission() {
    m_isRunning = false;
    m_currentPhase = 0;
    m_elapsedTime = 0;
    m_probePosition = QVector3D();
    m_altitude = 0;
    m_velocity = 0;
    m_drawnTrajectoryPoints.clear();
    m_lastDrawnIndex = -1;
    
    emit isRunningChanged();
    emit phaseChanged();
    emit positionChanged();
    emit elapsedTimeChanged();
    emit drawnTrajectoryChanged();
}

void ProbeMissionModel::updateSimulation(double dt) {
    if (!m_isRunning || m_trajectoryData.isEmpty()) {
        return;
    }
    
    m_elapsedTime += dt;
    emit elapsedTimeChanged();
    
    if (m_elapsedTime >= m_totalMissionTime) {
        m_currentPhase = m_loader.phases().size();
        m_probePosition = m_trajectoryData.last();
        m_isRunning = false;
        m_lastDrawnIndex = m_trajectoryData.size() - 1;
        
        for (int i = 0; i < m_trajectoryData.size(); ++i) {
            m_drawnTrajectoryPoints.append(QVariant::fromValue(m_trajectoryData[i]));
        }
        
        emit isRunningChanged();
        emit phaseChanged();
        emit missionCompleted();
        emit drawnTrajectoryChanged();
        return;
    }
    
    double progress = m_elapsedTime / m_totalMissionTime;
    int currentIndex = static_cast<int>(progress * (m_trajectoryData.size() - 1));
    currentIndex = qBound(0, currentIndex, m_trajectoryData.size() - 1);
    
    m_probePosition = m_trajectoryData[currentIndex];
    
    // 根据索引更新阶段
    const auto& phases = m_loader.phases();
    for (int i = 0; i < phases.size(); ++i) {
        if (currentIndex >= phases[i].startIndex && currentIndex <= phases[i].endIndex) {
            if (m_currentPhase != i) {
                m_currentPhase = i;
                emit phaseChanged();
            }
            break;
        }
    }
    
    // 绘制轨迹
    if (currentIndex > m_lastDrawnIndex) {
        for (int i = m_lastDrawnIndex + 1; i <= currentIndex; ++i) {
            m_drawnTrajectoryPoints.append(QVariant::fromValue(m_trajectoryData[i]));
        }
        m_lastDrawnIndex = currentIndex;
        emit drawnTrajectoryChanged();
    }
    
    // 计算高度
    m_altitude = m_probePosition.length() - CoordinateSystem::EARTH_RADIUS;
    
    // 计算速度
    if (currentIndex > 0 && currentIndex < m_trajectoryData.size() - 1) {
        QVector3D prevPos = m_trajectoryData[currentIndex - 1];
        QVector3D nextPos = m_trajectoryData[currentIndex + 1];
        double distKm = (nextPos - prevPos).length();
        m_velocity = distKm / (0.032 * 100);
    }
    
    emit positionChanged();
}