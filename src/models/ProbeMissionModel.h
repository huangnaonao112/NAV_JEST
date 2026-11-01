#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QVariantList>
#include "TrajectoryLoader.h"

/**
 * @brief 探测器任务模型 - 管理轨迹数据和任务状态
 */
class ProbeMissionModel : public QObject {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    
    Q_PROPERTY(int currentPhase READ currentPhase NOTIFY phaseChanged)
    Q_PROPERTY(QString currentPhaseName READ currentPhaseName NOTIFY phaseChanged)
    Q_PROPERTY(QVector3D probePosition READ probePosition NOTIFY positionChanged)
    Q_PROPERTY(double altitude READ altitude NOTIFY positionChanged)
    Q_PROPERTY(double velocity READ velocity NOTIFY positionChanged)
    Q_PROPERTY(double elapsedTime READ elapsedTime NOTIFY elapsedTimeChanged)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
    Q_PROPERTY(QVariantList drawnTrajectoryPoints READ drawnTrajectoryPoints NOTIFY drawnTrajectoryChanged)

public:
    explicit ProbeMissionModel(QObject *parent = nullptr);

    int currentPhase() const { return m_currentPhase; }
    QString currentPhaseName() const;
    QVector3D probePosition() const { return m_probePosition; }
    double altitude() const { return m_altitude; }
    double velocity() const { return m_velocity; }
    double elapsedTime() const { return m_elapsedTime; }
    bool isRunning() const { return m_isRunning; }
    QVariantList drawnTrajectoryPoints() const { return m_drawnTrajectoryPoints; }

    Q_INVOKABLE bool loadTrajectory(const QString& filePath);
    Q_INVOKABLE void generateDemoTrajectory();
    Q_INVOKABLE void startMission();
    Q_INVOKABLE void resetMission();
    Q_INVOKABLE void updateSimulation(double dt);

signals:
    void phaseChanged();
    void positionChanged();
    void missionCompleted();
    void drawnTrajectoryChanged();
    void elapsedTimeChanged();
    void isRunningChanged();

private:
    TrajectoryLoader m_loader;
    QVector<QVector3D> m_trajectoryData;
    
    int m_currentPhase = 0;
    QVector3D m_probePosition;
    double m_altitude = 0;
    double m_velocity = 0;
    double m_elapsedTime = 0;
    bool m_isRunning = false;
    
    QVariantList m_drawnTrajectoryPoints;
    int m_lastDrawnIndex = -1;
    
    double m_totalMissionTime = 120.0;
};