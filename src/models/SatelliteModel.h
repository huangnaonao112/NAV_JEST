#pragma once
#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QColor>
#include <QVector>
#include <QVariantList>
#include "OrbitalPhysics.h"
#include "OrbitMessage.h"

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
    // 真实轨道模式：由 $ORB 报文驱动（外部解算器给出开普勒根数）
    Q_PROPERTY(bool realOrbitMode READ realOrbitMode NOTIFY realOrbitModeChanged)
    // 可见性版本号：真实模式下 health 翻转（卫星升/降）时递增，
    // QML 轨道环绑定引用它以触发重估（避免重建整个 Repeater）
    Q_PROPERTY(int visibilityEpoch READ visibilityEpoch
               NOTIFY satellitesVisibilityChanged)
    // 当前格林尼治恒星时角 deg（真实模式逐帧更新）。轨道环缓存保持 ECI，
    // 轨道环随地球自转：QML 环节点 eulerRotation.y = earthSpinDeg。
    // 卫星位置含 -Ωe×tow 自转补偿，环缓存是周起始惯性系采样——
    // 两者必须转同一个角才重合（曾因环不转致卫星滑离轨道环 360°/天）。
    Q_PROPERTY(double earthSpinDeg READ earthSpinDeg NOTIFY earthSpinChanged)
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

        // 真实轨道相关（$ORB 报文）
        int health = 0;                 // 0 健康 / 1 不健康
        double meanMotionRevPerDay = 0.0;

        // 逐星时间基准（帧锚点）：M0 与 tow 必须同帧到达才不会互相错位。
        // $ORB 星：随 applyOrbitFrame 整体锚定；$DSP 星：随自己的 $DSP 帧
        // 锚定——否则快照内 $ORB 先到会拿新 tow 配旧 M，位置来回跳（左右闪动）。
        double frameSimT0 = -1.0;       // 收到该星当前帧时的仿真时刻（<0 未锚定）
        double frameTow = 0.0;          // 该星当前帧的周内秒
    };

    explicit SatelliteModel(QObject *parent = nullptr);

    Q_INVOKABLE void updateSimulation(double time);

    bool realOrbitMode() const { return m_realOrbitMode; }
    int visibilityEpoch() const { return m_visibilityEpoch; }
    double earthSpinDeg() const { return m_earthSpinDeg; }

    /// 展示参照系：true=惯性系（地球+地面标记自转 +Ωe·tow，轨道环静止），
    /// false=地固系（地球静止，轨道环随 −Ωe·tow 转）。两种模式下卫星
    /// 经纬度/2D 星下点始终按 ECEF 计算，仅渲染取位不同。
    Q_PROPERTY(bool inertialView READ inertialView
               WRITE setInertialView NOTIFY inertialViewChanged)
    bool inertialView() const { return m_inertialView; }
    void setInertialView(bool on);

    /// 真实模式下 health==0（该分钟参与定位）的卫星才可见。
    /// QML 轨道环 delegate 绑定此方法 + visibilityEpoch 实现零重建显隐。
    Q_INVOKABLE bool isSatelliteVisible(int index) const;

    /// 应用一帧 $ORB 轨道参数（替代仿真星座，进入真实轨道模式）
    void applyOrbitFrame(const OrbitMessage::Frame &frame);

    /// 应用一帧 $DSP 欺骗干扰参数（报文驱动的干扰演示）
    void applyDeceptionFrame(const QVector<OrbitMessage::DeceptionEntry> &entries);

    /// 欺骗卫星数量（$DSP 管理，独立于 $ORB 星座）
    Q_PROPERTY(int deceptionCount READ deceptionCount NOTIFY satellitesVisibilityChanged)
    int deceptionCount() const;
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
    /// $JAM 报文驱动：更新地面干扰机标记（位置+波纹半径 km，与 2D 同源）
    void updateJammerGroundPoint(double lat, double lon, double radiusKm);
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
    void realOrbitModeChanged();
    void inertialViewChanged();
    void satellitesVisibilityChanged();
    void earthSpinChanged();

private:
    QVector<SatelliteData> m_satellites;
    double m_elapsedTime = 0;
    bool m_visualSymmetry = true;
    bool m_paused = false;
    QVariantList m_groundPoints;
    PntDataModel *m_pntModel = nullptr;  // 用于同步干扰点数据

    // ===== 真实轨道模式（$ORB 报文）=====
    bool m_realOrbitMode = false;
    int m_frameWeek = 0;                 // 轨道根数参考历元（GPS 周）
    double m_frameSecondsOfWeek = 0.0;   // 轨道根数参考历元（周内秒）
    double m_frameSimT0 = 0.0;           // 收到该帧时的仿真时刻
    double m_dayRate = 1.0;              // 报文日时间/真实时间（回放快进倍率，
                                         //  由相邻两帧的 tow 差与到达间隔推算，
                                         //  经 EMA 平滑防 TCP 突发抖动）
    double m_rateEma = 1.0;              // dayRate 的滑动平均
    bool m_rateEmaInit = false;          // EMA 是否已初始化
    double m_lastDayDelta = 300.0;       // 上一帧间隔的日时间（外推上限用）
    double m_lastSubpointSync = -1.0;    // 2D 星下点上次同步的仿真时刻
    int m_visibilityEpoch = 0;           // health 翻转计数（QML 绑定重估触发器）
    int m_orbCount = 0;                  // $ORB 星座的卫星数（欺骗星不计入，追加其后）
    double m_earthSpinDeg = 0.0;         // 环整体旋转角 deg（=-Ωe×tow_render）
    bool m_inertialView = true;          // 展示参照系：惯性系(地球自转)/地固系

    // 把当前星下点同步给 2D 地图（与 3D 同源，保证一一对应）
    void syncSubpointsToPnt(double simTime);

    // 由相邻帧推算日时间倍率（见 m_dayRate 注释）
    void updateDayRate(const OrbitMessage::Frame &frame);

    // 轨道缓存 - 避免每帧重新计算
    mutable QVariantList m_cachedOrbitPaths;
    mutable bool m_orbitPathsCacheValid = false;

    void invalidateOrbitCache() { m_orbitPathsCacheValid = false; }
};
