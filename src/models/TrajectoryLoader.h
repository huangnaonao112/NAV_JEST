#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QVector3D>
#include <QVector>
#include <QString>
#include "CoordinateSystem.h"

/**
 * @brief 轨道数据加载器
 * 
 * 读取真实 km 坐标，自动转换为场景坐标（压缩距离）
 */
class TrajectoryLoader : public QObject {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    Q_PROPERTY(QVariantList points READ points NOTIFY pointsChanged)
    Q_PROPERTY(QVariantList phases READ phasesVariant NOTIFY phasesChanged)
    
public:
    struct Phase {
        QString name;
        int startIndex;
        int endIndex;
    };
    
    explicit TrajectoryLoader(QObject *parent = nullptr);
    
    /**
     * @brief 从JSON文件加载轨迹数据（真实 km 坐标）
     */
    bool loadFromFile(const QString& filePath);

    Q_INVOKABLE bool loadFromResource(const QString& qrcPath);

    /**
     * @brief 从应用程序目录加载轨迹文件
     * @param fileName 文件名（相对于应用程序目录）
     * @return 是否加载成功
     */
    Q_INVOKABLE bool loadFromAppDir(const QString& fileName);
    
    /**
     * @brief 生成演示轨迹（场景坐标）
     */
    QVector<QVector3D> generateDemoTrajectory();
    
    /**
     * @brief 获取轨迹点（场景坐标，已转换）
     */
    QVector<QVector3D> trajectoryPoints() const { return m_trajectoryPoints; }

    QVariantList points() const;
    
    /**
     * @brief 获取阶段信息
     */
    QVector<Phase> phases() const { return m_phases; }
    QVariantList phasesVariant() const;

signals:
    void pointsChanged();
    void phasesChanged();
    
private:
    QVector<QVector3D> m_trajectoryPoints;  // 场景坐标
    QVector<Phase> m_phases;
};