#pragma once

#include <QObject>
#include <QtQml/qqml.h>
#include <QVector>

/**
 * @brief 暗室灯光状态模型 - 36个灯
 * 
 * 状态定义:
 *   0 = 未使用 (灰色)
 *   1 = 使用 (绿色)
 *   2 = 干扰 (红色)
 */
class AntLightModel : public QObject {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
    
    // 整体状态数组，用于QML绑定
    Q_PROPERTY(QVector<int> states READ states NOTIFY statesChanged)
    
public:
    // 灯光状态枚举
    enum LightState {
        Unused = 0,     // 未使用 (灰色)
        InUse = 1,      // 使用 (绿色)
        Interference = 2 // 干扰 (红色)
    };
    Q_ENUM(LightState)
    
    explicit AntLightModel(QObject *parent = nullptr);
    
    // 获取状态数组
    QVector<int> states() const { return m_states; }
    
    // 获取单个灯的状态
    Q_INVOKABLE int getState(int index) const;
    
    // 更新所有状态
    void updateStates(const QVector<int> &states);
    
    // 更新单个灯状态
    Q_INVOKABLE void setState(int index, int state);
    
    // 清除所有状态（设为未使用）
    Q_INVOKABLE void clearAll();
    
signals:
    void statesChanged();
    void stateChanged(int index, int state);
    
private:
    QVector<int> m_states;  // 36个灯的状态
};