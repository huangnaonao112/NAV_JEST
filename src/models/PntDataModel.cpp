#include "PntDataModel.h"
#include "CoordinateSystem.h"
#include <QTimer>

PntDataModel::PntDataModel(QObject *parent)
    : QObject(parent)
    , m_taskStatus("空闲")
{
    // 3D 轨迹通知合并：33ms 窗口，推流期最多 30 次/秒
    m_traj3DFlushTimer.setSingleShot(true);
    m_traj3DFlushTimer.setInterval(33);
    connect(&m_traj3DFlushTimer, &QTimer::timeout,
            this, &PntDataModel::flushTrajectory3DChanged);

    // 创建任务结束检测定时器
    m_taskEndTimer = new QTimer(this);
    m_taskEndTimer->setSingleShot(true);
    m_taskEndTimer->setInterval(5000); // 5秒没有新数据视为任务结束
    connect(m_taskEndTimer, &QTimer::timeout, [this]() {
        if (m_taskStatus == "任务中") {
            setTaskStatus("任务结束");
        }
    });
}

// 任务状态
QString PntDataModel::taskStatus() const { return m_taskStatus; }

void PntDataModel::setTaskStatus(const QString &status) {
    if (m_taskStatus == status) return;
    m_taskStatus = status;
    emit taskStatusChanged();
}

void PntDataModel::clearData() {
    // 清空2D轨迹
    {
        QMutexLocker locker(&m_trajectoryMutex);
        m_trajectory.clear();
        m_dutTrajectory.clear();
    }
    emit trajectoryChanged();
    emit dutTrajectoryChanged();

    // 清空3D轨迹（先取消挂起的合并通知，避免清空后再推一次脏数据）
    m_traj3DFlushTimer.stop();
    {
        QMutexLocker locker(&m_trajectory3DMutex);
        m_trajectory3D.clear();
    }
    emit trajectory3DChanged();

    m_lastTrajectoryCount = 0;

    // 清空数据有效性
    if (m_hasSimData) { m_hasSimData = false; emit hasSimDataChanged(); }
    if (m_hasDutData) { m_hasDutData = false; emit hasDutDataChanged(); }

    // 清空当前位置数据
    m_simLat = 0.0; m_simLon = 0.0; m_simAlt = 0.0;
    m_simVx = 0.0; m_simVy = 0.0; m_simVz = 0.0;
    emit simLatitudeChanged(); emit simLongitudeChanged(); emit simAltitudeChanged();
    emit simVelocityXChanged(); emit simVelocityYChanged(); emit simVelocityZChanged();
    m_dutLat = 0.0; m_dutLon = 0.0; m_dutAlt = 0.0;
    m_dutVx = 0.0; m_dutVy = 0.0; m_dutVz = 0.0;
    emit dutLatitudeChanged(); emit dutLongitudeChanged(); emit dutAltitudeChanged();
    emit dutVelocityXChanged(); emit dutVelocityYChanged(); emit dutVelocityZChanged();

    // 清空转台和待测设备姿态数据
    m_ttRoll = 0.0; m_ttRollSpeed = 0.0;
    m_ttPitch = 0.0; m_ttPitchSpeed = 0.0;
    m_ttYaw = 0.0; m_ttYawSpeed = 0.0;
    emit turntableAttitudeChanged();
    m_dutRoll = 0.0; m_dutRollSpeed = 0.0;
    m_dutPitch = 0.0; m_dutPitchSpeed = 0.0;
    m_dutYaw = 0.0; m_dutYawSpeed = 0.0;
    emit dutAttitudeChanged();

    // 清空干扰机数据
    m_jammerLat = 0.0;
    m_jammerLon = 0.0;
    m_jammerRadius = 0.0;
    m_jammerDirection = 0.0;
    if (m_jammerActive) { m_jammerActive = false; emit jammerActiveChanged(); }
    emit jammerDataChanged();

    // 清空卫星数据
    {
        QMutexLocker locker(&m_satMutex);
        m_satCount = 0;
        m_satPrns.clear();
        m_satLats.clear();
        m_satLons.clear();
    }
    emit satDataChanged();
}

// 场景测试类型
int PntDataModel::testType() const { return m_testType; }

void PntDataModel::setTestType(int type) {
    if (m_testType == type) return;
    m_testType = type;
    emit testTypeChanged();
}

// 模拟源位置
double PntDataModel::simLatitude() const { return m_simLat; }
double PntDataModel::simLongitude() const { return m_simLon; }
double PntDataModel::simAltitude() const { return m_simAlt; }

// 模拟源速度
double PntDataModel::simVelocityX() const { return m_simVx; }
double PntDataModel::simVelocityY() const { return m_simVy; }
double PntDataModel::simVelocityZ() const { return m_simVz; }

// 待测设备位置
double PntDataModel::dutLatitude() const { return m_dutLat; }
double PntDataModel::dutLongitude() const { return m_dutLon; }
double PntDataModel::dutAltitude() const { return m_dutAlt; }

// 待测设备速度
double PntDataModel::dutVelocityX() const { return m_dutVx; }
double PntDataModel::dutVelocityY() const { return m_dutVy; }
double PntDataModel::dutVelocityZ() const { return m_dutVz; }

// 数据有效性
bool PntDataModel::hasSimData() const { return m_hasSimData; }
bool PntDataModel::hasDutData() const { return m_hasDutData; }

// 转台姿态数据
double PntDataModel::turntableRoll() const { return m_ttRoll; }
double PntDataModel::turntableRollSpeed() const { return m_ttRollSpeed; }
double PntDataModel::turntablePitch() const { return m_ttPitch; }
double PntDataModel::turntablePitchSpeed() const { return m_ttPitchSpeed; }
double PntDataModel::turntableYaw() const { return m_ttYaw; }
double PntDataModel::turntableYawSpeed() const { return m_ttYawSpeed; }

// 待测设备姿态数据
double PntDataModel::dutRoll() const { return m_dutRoll; }
double PntDataModel::dutRollSpeed() const { return m_dutRollSpeed; }
double PntDataModel::dutPitch() const { return m_dutPitch; }
double PntDataModel::dutPitchSpeed() const { return m_dutPitchSpeed; }
double PntDataModel::dutYaw() const { return m_dutYaw; }
double PntDataModel::dutYawSpeed() const { return m_dutYawSpeed; }

void PntDataModel::updateAttitude(double ttRoll, double ttRollSpeed, double ttPitch, double ttPitchSpeed, double ttYaw, double ttYawSpeed,
                                   double dutRoll, double dutRollSpeed, double dutPitch, double dutPitchSpeed, double dutYaw, double dutYawSpeed) {
    m_ttRoll = ttRoll;
    m_ttRollSpeed = ttRollSpeed;
    m_ttPitch = ttPitch;
    m_ttPitchSpeed = ttPitchSpeed;
    m_ttYaw = ttYaw;
    m_ttYawSpeed = ttYawSpeed;
    emit turntableAttitudeChanged();

    m_dutRoll = dutRoll;
    m_dutRollSpeed = dutRollSpeed;
    m_dutPitch = dutPitch;
    m_dutPitchSpeed = dutPitchSpeed;
    m_dutYaw = dutYaw;
    m_dutYawSpeed = dutYawSpeed;
    emit dutAttitudeChanged();
}

// 模拟源2D轨迹（经纬度）
int PntDataModel::trajectoryCount() const {
    QMutexLocker locker(&m_trajectoryMutex);
    return m_trajectory.size();
}

double PntDataModel::trajectoryLat(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].latitude;
    }
    return 0.0;
}

double PntDataModel::trajectoryLon(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].longitude;
    }
    return 0.0;
}

double PntDataModel::trajectoryAlt(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_trajectory.size()) {
        return m_trajectory[index].altitude;
    }
    return 0.0;
}

QString PntDataModel::satPrn(int index) const {
    QMutexLocker locker(&m_satMutex);
    if (index >= 0 && index < m_satPrns.size()) {
        return m_satPrns[index];
    }
    return QString();
}

double PntDataModel::satLat(int index) const {
    QMutexLocker locker(&m_satMutex);
    if (index >= 0 && index < m_satLats.size()) {
        return m_satLats[index];
    }
    return 0.0;
}

double PntDataModel::satLon(int index) const {
    QMutexLocker locker(&m_satMutex);
    if (index >= 0 && index < m_satLons.size()) {
        return m_satLons[index];
    }
    return 0.0;
}

void PntDataModel::clearTrajectory() {
    {
        QMutexLocker locker(&m_trajectoryMutex);
        m_trajectory.clear();
    }
    emit trajectoryChanged();
}

// 待测设备2D轨迹
int PntDataModel::dutTrajectoryCount() const {
    QMutexLocker locker(&m_trajectoryMutex);
    return m_dutTrajectory.size();
}

double PntDataModel::dutTrajectoryLat(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_dutTrajectory.size()) {
        return m_dutTrajectory[index].latitude;
    }
    return 0.0;
}

double PntDataModel::dutTrajectoryLon(int index) const {
    QMutexLocker locker(&m_trajectoryMutex);
    if (index >= 0 && index < m_dutTrajectory.size()) {
        return m_dutTrajectory[index].longitude;
    }
    return 0.0;
}

void PntDataModel::clearDutTrajectory() {
    {
        QMutexLocker locker(&m_trajectoryMutex);
        m_dutTrajectory.clear();
    }
    emit dutTrajectoryChanged();
}

// 3D轨迹（ECEF XYZ坐标）
int PntDataModel::trajectory3DCount() const {
    QMutexLocker locker(&m_trajectory3DMutex);
    return m_trajectory3D.size();
}

double PntDataModel::trajectory3DX(int index) const {
    QMutexLocker locker(&m_trajectory3DMutex);
    if (index >= 0 && index < m_trajectory3D.size()) {
        return m_trajectory3D[index].x;
    }
    return 0.0;
}

double PntDataModel::trajectory3DY(int index) const {
    QMutexLocker locker(&m_trajectory3DMutex);
    if (index >= 0 && index < m_trajectory3D.size()) {
        return m_trajectory3D[index].y;
    }
    return 0.0;
}

double PntDataModel::trajectory3DZ(int index) const {
    QMutexLocker locker(&m_trajectory3DMutex);
    if (index >= 0 && index < m_trajectory3D.size()) {
        return m_trajectory3D[index].z;
    }
    return 0.0;
}

void PntDataModel::clearTrajectory3D() {
    m_traj3DFlushTimer.stop();
    {
        QMutexLocker locker(&m_trajectory3DMutex);
        m_trajectory3D.clear();
    }
    emit trajectory3DChanged();
}

QVariantList PntDataModel::trajectory3DScenePoints() const {
    // $TRJ 是 ECEF/ECI km（空间 C），回放轨迹属于地月压缩空间（空间 B），
    // 必须显式乘压缩比；缩放只在这里做一次，QML 侧不再碰坐标。
    QMutexLocker locker(&m_trajectory3DMutex);
    QVariantList result;
    result.reserve(m_trajectory3D.size());
    for (const TrajectoryPoint3D &p : m_trajectory3D) {
        result.append(QVariant::fromValue(
            CoordinateSystem::toDisplaySpace(QVector3D(float(p.x), float(p.y), float(p.z)))));
    }
    return result;
}

QVector3D PntDataModel::trajectory3DLastScenePoint() const {
    QMutexLocker locker(&m_trajectory3DMutex);
    if (m_trajectory3D.isEmpty())
        return QVector3D();
    const TrajectoryPoint3D &p = m_trajectory3D.last();
    return CoordinateSystem::toDisplaySpace(QVector3D(float(p.x), float(p.y), float(p.z)));
}

void PntDataModel::updateFromPnt(double simLat, double simLon, double simAlt,
                                  double dutLat, double dutLon, double dutAlt,
                                  double simVx, double simVy, double simVz,
                                  double dutVx, double dutVy, double dutVz)
{
    setSimLatitude(simLat);
    setSimLongitude(simLon);
    setSimAltitude(simAlt);
    setSimVelocityX(simVx);
    setSimVelocityY(simVy);
    setSimVelocityZ(simVz);

    setDutLatitude(dutLat);
    setDutLongitude(dutLon);
    setDutAltitude(dutAlt);
    setDutVelocityX(dutVx);
    setDutVelocityY(dutVy);
    setDutVelocityZ(dutVz);

    // 标记数据有效
    if (!m_hasSimData) { m_hasSimData = true; emit hasSimDataChanged(); }
    if (!m_hasDutData) { m_hasDutData = true; emit hasDutDataChanged(); }
    
    // 添加模拟源2D轨迹点（经纬度）
    {
        QMutexLocker locker(&m_trajectoryMutex);
        if (m_trajectory.isEmpty() || 
            !qFuzzyCompare(m_trajectory.last().latitude, simLat) ||
            !qFuzzyCompare(m_trajectory.last().longitude, simLon)) {
            TrajectoryPoint pt;
            pt.latitude = simLat;
            pt.longitude = simLon;
            pt.altitude = simAlt;
            m_trajectory.append(pt);
            
            // 超过最大数量时移除最旧的点
            if (m_trajectory.size() > MAX_TRAJECTORY_POINTS) {
                m_trajectory.removeFirst();
            }
        }
    }
    emit trajectoryChanged();
    
    // 添加待测设备2D轨迹点
    {
        QMutexLocker locker(&m_trajectoryMutex);
        if (m_dutTrajectory.isEmpty() || 
            !qFuzzyCompare(m_dutTrajectory.last().latitude, dutLat) ||
            !qFuzzyCompare(m_dutTrajectory.last().longitude, dutLon)) {
            TrajectoryPoint pt;
            pt.latitude = dutLat;
            pt.longitude = dutLon;
            pt.altitude = dutAlt;
            m_dutTrajectory.append(pt);
            
            if (m_dutTrajectory.size() > MAX_TRAJECTORY_POINTS) {
                m_dutTrajectory.removeFirst();
            }
        }
    }
    emit dutTrajectoryChanged();
    
    // 检测任务状态
    checkTaskStatus();
    
    emit dataUpdated();
}

void PntDataModel::updateFromTraj(double x, double y, double z) {
    // 存储XYZ坐标（WGS84 ECEF, km）到3D轨迹
    // 不再覆盖simLat/simLon/simAlt，保持2D和3D数据独立
    
    // 添加3D轨迹点
    {
        QMutexLocker locker(&m_trajectory3DMutex);
        if (m_trajectory3D.isEmpty() || 
            !qFuzzyCompare(m_trajectory3D.last().x, x) ||
            !qFuzzyCompare(m_trajectory3D.last().y, y)) {
            TrajectoryPoint3D pt;
            pt.x = x;
            pt.y = y;
            pt.z = z;
            m_trajectory3D.append(pt);
            
            if (m_trajectory3D.size() > MAX_TRAJECTORY_POINTS) {
                m_trajectory3D.removeFirst();
            }
        }
    }
    
    // 通知节流（33ms）：推流期可能每秒上百点，若逐点/每10点通知，
    // QML 会以同样频率重建万点几何体并上传顶点缓冲。
    // 这里用时间窗口合并，无论来点多快，下游最多 30 次/秒。
    if (!m_traj3DFlushTimer.isActive())
        m_traj3DFlushTimer.start();
}

void PntDataModel::flushTrajectory3DChanged() {
    emit trajectory3DChanged();
    checkTaskStatus();
}

void PntDataModel::updateFromSat(int count, const QStringList &prns, const QVector<double> &lats, const QVector<double> &lons) {
    // 存储卫星星下点数据
    {
        QMutexLocker locker(&m_satMutex);
        m_satCount = count;
        m_satPrns = prns;
        m_satLats = lats;
        m_satLons = lons;
    }
    
    emit satDataChanged();
}

void PntDataModel::checkTaskStatus() {
    // 任务状态只由 BEG/END 报文控制，这里只更新轨迹计数
    int currentCount = 0;
    {
        QMutexLocker locker(&m_trajectoryMutex);
        currentCount = m_trajectory.size();
    }
    m_lastTrajectoryCount = currentCount;
}

// Setters
void PntDataModel::setSimLatitude(double v) {
    if (qFuzzyCompare(m_simLat, v)) return;
    m_simLat = v;
    emit simLatitudeChanged();
}

void PntDataModel::setSimLongitude(double v) {
    if (qFuzzyCompare(m_simLon, v)) return;
    m_simLon = v;
    emit simLongitudeChanged();
}

void PntDataModel::setSimAltitude(double v) {
    if (qFuzzyCompare(m_simAlt, v)) return;
    m_simAlt = v;
    emit simAltitudeChanged();
}

void PntDataModel::setSimVelocityX(double v) {
    if (qFuzzyCompare(m_simVx, v)) return;
    m_simVx = v;
    emit simVelocityXChanged();
}

void PntDataModel::setSimVelocityY(double v) {
    if (qFuzzyCompare(m_simVy, v)) return;
    m_simVy = v;
    emit simVelocityYChanged();
}

void PntDataModel::setSimVelocityZ(double v) {
    if (qFuzzyCompare(m_simVz, v)) return;
    m_simVz = v;
    emit simVelocityZChanged();
}

void PntDataModel::setDutLatitude(double v) {
    if (qFuzzyCompare(m_dutLat, v)) return;
    m_dutLat = v;
    emit dutLatitudeChanged();
}

void PntDataModel::setDutLongitude(double v) {
    if (qFuzzyCompare(m_dutLon, v)) return;
    m_dutLon = v;
    emit dutLongitudeChanged();
}

void PntDataModel::setDutAltitude(double v) {
    if (qFuzzyCompare(m_dutAlt, v)) return;
    m_dutAlt = v;
    emit dutAltitudeChanged();
}

void PntDataModel::setDutVelocityX(double v) {
    if (qFuzzyCompare(m_dutVx, v)) return;
    m_dutVx = v;
    emit dutVelocityXChanged();
}

void PntDataModel::setDutVelocityY(double v) {
    if (qFuzzyCompare(m_dutVy, v)) return;
    m_dutVy = v;
    emit dutVelocityYChanged();
}

void PntDataModel::setDutVelocityZ(double v) {
    if (qFuzzyCompare(m_dutVz, v)) return;
    m_dutVz = v;
    emit dutVelocityZChanged();
}

// 干扰机数据
void PntDataModel::setJammerActive(bool active) {
    if (m_jammerActive == active) return;
    m_jammerActive = active;
    emit jammerActiveChanged();
}

void PntDataModel::updateFromJam(double lat, double lon, double radius, double direction) {
    m_jammerLat = lat;
    m_jammerLon = lon;
    m_jammerRadius = radius;
    m_jammerDirection = direction;

    // 自动激活干扰机显示
    if (!m_jammerActive) {
        m_jammerActive = true;
        emit jammerActiveChanged();
    }

    emit jammerDataChanged();
}
