#pragma once
#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QColor>
#include <QVector>
#include <QVariantList>
#include "OrbitalPhysics.h"

class PntDataModel;  // 前向声明

class SatelliteModel : public QAbstractListModel {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    Q_PROPERTY(QVariantList orbitPaths READ orbitPaths NOTIFY orbitPathsChanged)
    Q_PROPERTY(bool visualSymmetry READ visualSymmetry WRITE setVisualSymmetry NOTIFY visualSymmetryChanged)
    Q_PROPERTY(QVariantList groundPoints READ groundPoints NOTIFY groundPointsChanged)
    Q_PROPERTY(int satelliteCount READ satelliteCount NOTIFY satelliteCountChanged)
    Q_PROPERTY(bool paused READ paused WRITE setPaused NOTIFY pausedChanged)
public:
    enum Roles {
        PositionRole = Qt::UserRole + 1,
        RotationRole,
        ColorRole,
        VisibleRole,
        LongitudeRole,
        LatitudeRole,
        NameRole,
        PhaseRole,
        ScaleRole,
        IsDeceptionRole,     // 是否为欺骗卫星
        SignalActiveRole     // 干扰信号是否激活
    };

    enum GroundPointType {
        BasePoint = 0,
        JammerPoint = 1,
        TargetPoint = 2,
        TrajectoryPoint = 3,
        SpoofedPoint = 4
    };
    Q_ENUM(GroundPointType)

    enum MissionPhase {
        NoMission = 0,
        LaunchPhase = 1      // 发射与进入停泊轨道
    };
    Q_ENUM(MissionPhase)

    struct SatelliteData {
        QVector3D position;
        QVector3D rotation;
        QColor color;
        bool visible = false;
        double longitude = 0;
        double latitude = 0;
        OrbitalElements elements;

        // 任务相关
        MissionPhase phase = NoMission;
        double missionStartTime = 0;
        double lastPathRecordTime = 0;
        QString name;
        QList<QVector3D> missionPath;
        QVector3D phaseTransitionPos;
        double visualScale = 1.0;

        // 欺骗卫星相关
        bool isDeception = false;       // 是否为欺骗卫星
        bool signalActive = false;      // 干扰信号是否激活
    };

    explicit SatelliteModel(QObject *parent = nullptr);

    Q_INVOKABLE void updateSimulation(double time);
    Q_INVOKABLE void addSatellite(double a, double e, double i, double raan, double w, double m0, double n, QColor color, bool visible = true);
    Q_INVOKABLE void addDeceptionSatellite(double altitude, double lat, double lon, QColor color);
    Q_INVOKABLE void updateSatellite(int index, double a, double e, double i, double raan, double w, double m0, double n);
    Q_INVOKABLE void setColor(int index, QColor color);
    Q_INVOKABLE QVariantMap getElements(int index) const;
    Q_INVOKABLE void setVisible(int index, bool visible);
    Q_INVOKABLE bool isVisible(int index) const;
    Q_INVOKABLE void setSignalActive(int index, bool active);  // 设置干扰信号激活状态
    Q_INVOKABLE void setVisibilityBatch(QVariantList indices, QVariantList visibles);  // 批量设置
    Q_INVOKABLE void clear();
    Q_INVOKABLE void addGroundPoint(double lat, double lon, QString name = "", int type = 0, double radius = 500.0, QColor color = QColor("red"), double errorRadius = 0.0);
    Q_INVOKABLE void updateGroundPoint(int index, double lat, double lon, QString name, int type, double radius, QColor color, double errorRadius);
    Q_INVOKABLE QVariantMap getGroundPoint(int index) const;
    Q_INVOKABLE void removeGroundPoint(int index);

    Q_INVOKABLE int satelliteCount() const { return m_satellites.size(); }
    Q_INVOKABLE void setPaused(bool paused);
    bool paused() const { return m_paused; }

    // 设置PntDataModel引用，用于同步干扰点数据
    void setPntDataModel(PntDataModel *pntModel) { m_pntModel = pntModel; }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList orbitPaths() const;
    bool visualSymmetry() const { return m_visualSymmetry; }
    void setVisualSymmetry(bool enabled);
    QVariantList groundPoints() const { return m_groundPoints; }

signals:
    void orbitPathsChanged();
    void visualSymmetryChanged();
    void groundPointsChanged();
    void satelliteCountChanged();
    void pausedChanged();

private:
    QVector<SatelliteData> m_satellites;
    double m_elapsedTime = 0;
    bool m_visualSymmetry = true;
    bool m_paused = false;
    QVariantList m_groundPoints;
    PntDataModel *m_pntModel = nullptr;  // 用于同步干扰点数据

    // 轨道缓存 - 避免每帧重新计算
    mutable QVariantList m_cachedOrbitPaths;
    mutable bool m_orbitPathsCacheValid = false;

    void invalidateOrbitCache() { m_orbitPathsCacheValid = false; }
};
