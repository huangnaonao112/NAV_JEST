#include "TransferOrbitModel.h"
#include <QDebug>

TransferOrbitModel::TransferOrbitModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

TransferOrbitModel::~TransferOrbitModel()
{
}

void TransferOrbitModel::loadDefaultOrbit()
{
    // 轨道数据将由用户通过调试页面手动设置
    // 这里不做任何操作
}

bool TransferOrbitModel::loadFromCSV(const QString &filePath)
{
    Q_UNUSED(filePath)
    return true;
}

void TransferOrbitModel::clear()
{
    beginResetModel();
    m_segments.clear();
    endResetModel();
}

void TransferOrbitModel::setControlPoints(const QList<QVector3D> &earthPoints,
                                           const QList<QVector3D> &transferPoints,
                                           const QList<QVector3D> &moonPoints)
{
    beginResetModel();
    m_segments.clear();
    
    // 地球轨道
    OrbitSegment earthSeg;
    earthSeg.name = "earth_orbit";
    earthSeg.displayName = "地球轨道";
    earthSeg.color = QColor("#FF6B6B");
    for (const auto &pt : earthPoints) {
        earthSeg.points.append(pt);
    }
    m_segments.append(earthSeg);
    
    // 转移轨道
    OrbitSegment transferSeg;
    transferSeg.name = "transfer";
    transferSeg.displayName = "地月转移";
    transferSeg.color = QColor("#4ECDC4");
    for (const auto &pt : transferPoints) {
        transferSeg.points.append(pt);
    }
    m_segments.append(transferSeg);
    
    // 月球轨道
    OrbitSegment moonSeg;
    moonSeg.name = "moon_orbit";
    moonSeg.displayName = "月球轨道";
    moonSeg.color = QColor("#45B7D1");
    for (const auto &pt : moonPoints) {
        moonSeg.points.append(pt);
    }
    m_segments.append(moonSeg);
    
    endResetModel();
    emit segmentsChanged();
}

int TransferOrbitModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_segments.size();
}

QVariant TransferOrbitModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_segments.size()) {
        return QVariant();
    }
    
    const OrbitSegment &segment = m_segments[index.row()];
    
    switch (role) {
    case NameRole: return segment.name;
    case DisplayNameRole: return segment.displayName;
    case ColorRole: return segment.color;
    case PointsRole:
        {
            QVariantList pointList;
            for (const QVector3D &pt : segment.points) {
                pointList.append(QVariant::fromValue(pt));
            }
            return pointList;
        }
    default: return QVariant();
    }
}

QHash<int, QByteArray> TransferOrbitModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DisplayNameRole] = "displayName";
    roles[ColorRole] = "color";
    roles[PointsRole] = "points";
    return roles;
}