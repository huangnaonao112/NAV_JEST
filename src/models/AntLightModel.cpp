#include "AntLightModel.h"

AntLightModel::AntLightModel(QObject *parent)
    : QObject(parent)
    , m_states(36, 0)  // 初始化36个灯，默认状态为未使用(0)
{
}

int AntLightModel::getState(int index) const
{
    if (index < 0 || index >= m_states.size()) {
        return 0;  // 无效索引返回未使用状态
    }
    return m_states[index];
}

void AntLightModel::updateStates(const QVector<int> &states)
{
    if (states.size() != 36) {
        return;  // 必须是36个状态
    }
    
    m_states = states;
    emit statesChanged();
    
    // 发送每个灯的状态变化信号
    for (int i = 0; i < m_states.size(); ++i) {
        emit stateChanged(i, m_states[i]);
    }
}

void AntLightModel::setState(int index, int state)
{
    if (index < 0 || index >= m_states.size()) {
        return;
    }
    
    if (state < 0 || state > 2) {
        return;  // 状态值必须在0-2之间
    }
    
    if (m_states[index] != state) {
        m_states[index] = state;
        emit statesChanged();
        emit stateChanged(index, state);
    }
}

void AntLightModel::clearAll()
{
    m_states.fill(0);
    emit statesChanged();
    
    for (int i = 0; i < m_states.size(); ++i) {
        emit stateChanged(i, 0);
    }
}