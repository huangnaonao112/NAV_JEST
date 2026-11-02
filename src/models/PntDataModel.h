#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QVector>
#include <QVector3D>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>

class QTimer;

/**
 * @brief 轨迹点结构
 */
struct TrajectoryPoint {
    double latitude;
    double longitude;
    double altitude;
};

/**
 * @brief 3D轨迹点结构（ECEF XYZ坐标）
 */
struct TrajectoryPoint3D {
    double x;  // ECEF X坐标 (km)
    double y;  // ECEF Y坐标 (km)
    double z;  // ECEF Z坐标 (km)
};

/**
 * @brief 位置速度数据模型
 * 存储模拟源和待测设备的位置和速度信息
 * 2D和3D轨迹数据分离，避免互相干扰
 */
class PntDataModel : public QObject {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    
    // 模拟源位置
    Q_PROPERTY(double simLatitude READ simLatitude WRITE setSimLatitude NOTIFY simLatitudeChanged)
    Q_PROPERTY(double simLongitude READ simLongitude WRITE setSimLongitude NOTIFY simLongitudeChanged)
    Q_PROPERTY(double simAltitude READ simAltitude WRITE setSimAltitude NOTIFY simAltitudeChanged)
    
    // 模拟源速度
    Q_PROPERTY(double simVelocityX READ simVelocityX WRITE setSimVelocityX NOTIFY simVelocityXChanged)
    Q_PROPERTY(double simVelocityY READ simVelocityY WRITE setSimVelocityY NOTIFY simVelocityYChanged)
    Q_PROPERTY(double simVelocityZ READ simVelocityZ WRITE setSimVelocityZ NOTIFY simVelocityZChanged)
    
    // 模拟源数据是否有效
    Q_PROPERTY(bool hasSimData READ hasSimData NOTIFY hasSimDataChanged)
    
    // 待测设备位置
    Q_PROPERTY(double dutLatitude READ dutLatitude WRITE setDutLatitude NOTIFY dutLatitudeChanged)
    Q_PROPERTY(double dutLongitude READ dutLongitude WRITE setDutLongitude NOTIFY dutLongitudeChanged)
    Q_PROPERTY(double dutAltitude READ dutAltitude WRITE setDutAltitude NOTIFY dutAltitudeChanged)
    
    // 待测设备速度
    Q_PROPERTY(double dutVelocityX READ dutVelocityX WRITE setDutVelocityX NOTIFY dutVelocityXChanged)
    Q_PROPERTY(double dutVelocityY READ dutVelocityY WRITE setDutVelocityY NOTIFY dutVelocityYChanged)
    Q_PROPERTY(double dutVelocityZ READ dutVelocityZ WRITE setDutVelocityZ NOTIFY dutVelocityZChanged)
    
    // 待测设备数据是否有效
    Q_PROPERTY(bool hasDutData READ hasDutData NOTIFY hasDutDataChanged)

    // ========== 转台姿态数据 ==========
    Q_PROPERTY(double turntableRoll READ turntableRoll NOTIFY turntableAttitudeChanged)
    Q_PROPERTY(double turntableRollSpeed READ turntableRollSpeed NOTIFY turntableAttitudeChanged)
    Q_PROPERTY(double turntablePitch READ turntablePitch NOTIFY turntableAttitudeChanged)
    Q_PROPERTY(double turntablePitchSpeed READ turntablePitchSpeed NOTIFY turntableAttitudeChanged)
    Q_PROPERTY(double turntableYaw READ turntableYaw NOTIFY turntableAttitudeChanged)
    Q_PROPERTY(double turntableYawSpeed READ turntableYawSpeed NOTIFY turntableAttitudeChanged)

    // ========== 待测设备姿态数据 ==========
    Q_PROPERTY(double dutRoll READ dutRoll NOTIFY dutAttitudeChanged)
    Q_PROPERTY(double dutRollSpeed READ dutRollSpeed NOTIFY dutAttitudeChanged)
    Q_PROPERTY(double dutPitch READ dutPitch NOTIFY dutAttitudeChanged)
    Q_PROPERTY(double dutPitchSpeed READ dutPitchSpeed NOTIFY dutAttitudeChanged)
    Q_PROPERTY(double dutYaw READ dutYaw NOTIFY dutAttitudeChanged)
    Q_PROPERTY(double dutYawSpeed READ dutYawSpeed NOTIFY dutAttitudeChanged)

    // 模拟源2D轨迹（用于2D地图，经纬度）
    Q_PROPERTY(int trajectoryCount READ trajectoryCount NOTIFY trajectoryChanged)
    
    // 待测设备2D轨迹
    Q_PROPERTY(int dutTrajectoryCount READ dutTrajectoryCount NOTIFY dutTrajectoryChanged)
    
    // 3D轨迹（用于3D场景，ECEF XYZ坐标）
    Q_PROPERTY(int trajectory3DCount READ trajectory3DCount NOTIFY trajectory3DChanged)

    // 3D 轨迹的批量场景坐标（一次性返回 QVector3D 列表，已乘地月压缩比）。
    // 供 TrajectoryLineGeometry.points 直接绑定：避免 QML 侧逐点调用
    // trajectory3DX/Y/Z（N 点 = 3N 次跨语言调用 + N 个 JS 对象），
    // 万点推流时这是掉帧与 GC 卡顿的主要来源。
    Q_PROPERTY(QVariantList trajectory3DScenePoints READ trajectory3DScenePoints
               NOTIFY trajectory3DChanged)
    
    // 任务状态: "空闲" / "任务中" / "任务结束"
    Q_PROPERTY(QString taskStatus READ taskStatus NOTIFY taskStatusChanged)

    // 场景测试类型 (从BEG/END报文获取)
    Q_PROPERTY(int testType READ testType NOTIFY testTypeChanged)

    // 卫星星下点数据（SAT）
    Q_PROPERTY(int satCount READ satCount NOTIFY satDataChanged)

    // ========== 单个干扰机数据 ==========
    Q_PROPERTY(bool jammerActive READ jammerActive WRITE setJammerActive NOTIFY jammerActiveChanged)
    Q_PROPERTY(double jammerLatitude READ jammerLatitude NOTIFY jammerDataChanged)
    Q_PROPERTY(double jammerLongitude READ jammerLongitude NOTIFY jammerDataChanged)
    Q_PROPERTY(double jammerRadius READ jammerRadius NOTIFY jammerDataChanged)
    Q_PROPERTY(double jammerDirection READ jammerDirection NOTIFY jammerDataChanged)

public:

public:
    explicit PntDataModel(QObject *parent = nullptr);
    
    // 模拟源位置
    double simLatitude() const;
    double simLongitude() const;
    double simAltitude() const;
    
    // 模拟源速度
    double simVelocityX() const;
    double simVelocityY() const;
    double simVelocityZ() const;
    
    // 待测设备位置
    double dutLatitude() const;
    double dutLongitude() const;
    double dutAltitude() const;
    
    // 待测设备速度
    double dutVelocityX() const;
    double dutVelocityY() const;
    double dutVelocityZ() const;
    
    // 数据有效性
    bool hasSimData() const;
    bool hasDutData() const;

    // 转台姿态数据
    double turntableRoll() const;
    double turntableRollSpeed() const;
    double turntablePitch() const;
    double turntablePitchSpeed() const;
    double turntableYaw() const;
    double turntableYawSpeed() const;

    // 待测设备姿态数据
    double dutRoll() const;
    double dutRollSpeed() const;
    double dutPitch() const;
    double dutPitchSpeed() const;
    double dutYaw() const;
    double dutYawSpeed() const;

    // 更新转台和待测设备姿态数据（从报文调用，12个参数）
    void updateAttitude(double ttRoll, double ttRollSpeed, double ttPitch, double ttPitchSpeed, double ttYaw, double ttYawSpeed,
                        double dutRoll, double dutRollSpeed, double dutPitch, double dutPitchSpeed, double dutYaw, double dutYawSpeed);

    // 模拟源2D轨迹（经纬度，用于2D地图）
    Q_INVOKABLE int trajectoryCount() const;
    Q_INVOKABLE double trajectoryLat(int index) const;
    Q_INVOKABLE double trajectoryLon(int index) const;
    Q_INVOKABLE double trajectoryAlt(int index) const;
    Q_INVOKABLE void clearTrajectory();
    
    // 待测设备2D轨迹
    int dutTrajectoryCount() const;
    Q_INVOKABLE double dutTrajectoryLat(int index) const;
    Q_INVOKABLE double dutTrajectoryLon(int index) const;
    Q_INVOKABLE void clearDutTrajectory();
    
    // 3D轨迹（ECEF XYZ，用于3D场景）
    Q_INVOKABLE int trajectory3DCount() const;
    Q_INVOKABLE double trajectory3DX(int index) const;
    Q_INVOKABLE double trajectory3DY(int index) const;
    Q_INVOKABLE double trajectory3DZ(int index) const;
    Q_INVOKABLE void clearTrajectory3D();

    // 批量：一次返回全部点的场景坐标（空间 2，已乘 DISTANCE_SCALE）
    QVariantList trajectory3DScenePoints() const;

    // 最新一个点的场景坐标（供"探测器"标记使用，单次调用）
    Q_INVOKABLE QVector3D trajectory3DLastScenePoint() const;
    
    // 任务状态
    QString taskStatus() const;
    void clearData();               // 只清除数据，不改变任务状态

    // 场景测试类型
    int testType() const;
    void setTestType(int type);

    // 允许QML侧在演示/回放时设置任务状态
    Q_INVOKABLE void setTaskStatusInvokable(const QString &status) { setTaskStatus(status); }
    
    // 批量更新数据
    void updateFromPnt(double simLat, double simLon, double simAlt,
                       double dutLat, double dutLon, double dutAlt,
                       double simVx, double simVy, double simVz,
                       double dutVx, double dutVy, double dutVz);
    
    // 从TRAJ报文更新3D轨迹（WGS84 XYZ坐标，km）
    void updateFromTraj(double x, double y, double z);
    
    // 从SAT报文更新卫星星下点
    void updateFromSat(int count, const QStringList &prns, const QVector<double> &lats, const QVector<double> &lons);

    // SAT 数据读取（QML）
    int satCount() const { return m_satCount; }
    Q_INVOKABLE QString satPrn(int index) const;
    Q_INVOKABLE double satLat(int index) const;
    Q_INVOKABLE double satLon(int index) const;

    // ========== 单个干扰机数据接口 ==========
    bool jammerActive() const { return m_jammerActive; }
    double jammerLatitude() const { return m_jammerLat; }
    double jammerLongitude() const { return m_jammerLon; }
    double jammerRadius() const { return m_jammerRadius; }
    double jammerDirection() const { return m_jammerDirection; }

    // 更新干扰机数据（从报文调用）
    void updateFromJam(double lat, double lon, double radius, double direction);

    // 设置干扰机激活状态
    Q_INVOKABLE void setJammerActive(bool active);

public slots:
    void setSimLatitude(double v);
    void setSimLongitude(double v);
    void setSimAltitude(double v);
    void setSimVelocityX(double v);
    void setSimVelocityY(double v);
    void setSimVelocityZ(double v);
    void setDutLatitude(double v);
    void setDutLongitude(double v);
    void setDutAltitude(double v);
    void setDutVelocityX(double v);
    void setDutVelocityY(double v);
    void setDutVelocityZ(double v);
    void setTaskStatus(const QString &status);
    
signals:
    void simLatitudeChanged();
    void simLongitudeChanged();
    void simAltitudeChanged();
    void simVelocityXChanged();
    void simVelocityYChanged();
    void simVelocityZChanged();
    void dutLatitudeChanged();
    void dutLongitudeChanged();
    void dutAltitudeChanged();
    void dutVelocityXChanged();
    void dutVelocityYChanged();
    void dutVelocityZChanged();
    void hasSimDataChanged();
    void hasDutDataChanged();
    void dataUpdated();
    void trajectoryChanged();       // 2D轨迹变化
    void dutTrajectoryChanged();    // 待测设备2D轨迹变化
    void trajectory3DChanged();     // 3D轨迹变化
    void taskStatusChanged();
    void testTypeChanged();         // 测试类型变化
    void satDataChanged();  // 卫星数据更新信号
    void jammerActiveChanged();
    void jammerDataChanged();  // 干扰机数据更新信号
    void turntableAttitudeChanged(); // 转台姿态数据更新信号
    void dutAttitudeChanged(); // 待测设备姿态数据更新信号

private:
    void checkTaskStatus();  // 检测任务状态
    
    // 模拟源当前位置
    double m_simLat = 0.0;
    double m_simLon = 0.0;
    double m_simAlt = 0.0;
    double m_simVx = 0.0;
    double m_simVy = 0.0;
    double m_simVz = 0.0;
    
    // 待测设备当前位置
    double m_dutLat = 0.0;
    double m_dutLon = 0.0;
    double m_dutAlt = 0.0;
    double m_dutVx = 0.0;
    double m_dutVy = 0.0;
    double m_dutVz = 0.0;

    // 转台姿态数据
    double m_ttRoll = 0.0;
    double m_ttRollSpeed = 0.0;
    double m_ttPitch = 0.0;
    double m_ttPitchSpeed = 0.0;
    double m_ttYaw = 0.0;
    double m_ttYawSpeed = 0.0;

    // 待测设备姿态数据
    double m_dutRoll = 0.0;
    double m_dutRollSpeed = 0.0;
    double m_dutPitch = 0.0;
    double m_dutPitchSpeed = 0.0;
    double m_dutYaw = 0.0;
    double m_dutYawSpeed = 0.0;

    // 数据有效性标志
    bool m_hasSimData = false;
    bool m_hasDutData = false;
    
    // 2D轨迹数据（用于2D地图，经纬度）
    QVector<TrajectoryPoint> m_trajectory;
    QVector<TrajectoryPoint> m_dutTrajectory;
    mutable QMutex m_trajectoryMutex;  // 2D轨迹互斥锁
    
    // 3D轨迹数据（用于3D场景，ECEF XYZ坐标）
    QVector<TrajectoryPoint3D> m_trajectory3D;
    mutable QMutex m_trajectory3DMutex;  // 3D轨迹互斥锁
    
    // 卫星星下点数据
    int m_satCount = 0;
    QStringList m_satPrns;
    QVector<double> m_satLats;
    QVector<double> m_satLons;
    mutable QMutex m_satMutex;  // 卫星数据互斥锁
    
    // 任务状态
    QString m_taskStatus = "空闲";
    QTimer *m_taskEndTimer = nullptr;  // 用于检测任务结束（一段时间没有新数据）

public slots:
    // 3D 轨迹通知节流窗口到期（33ms），见 updateFromTraj
    void flushTrajectory3DChanged();

private:
    QTimer m_traj3DFlushTimer;         // 3D 轨迹信号合并（33ms 单次触发）
    int m_lastTrajectoryCount = 0;

    // 场景测试类型
    int m_testType = 0;

    // 单个干扰机数据
    bool m_jammerActive = false;
    double m_jammerLat = 0.0;
    double m_jammerLon = 0.0;
    double m_jammerRadius = 0.0;
    double m_jammerDirection = 0.0;  // 干扰方向 (度)

    // 轨迹点最大数量限制，防止内存耗尽
    static constexpr int MAX_TRAJECTORY_POINTS = 10000;
};
