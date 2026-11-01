#include "RackStatusModel.h"

RackStatusModel::RackStatusModel(QObject *parent) : QAbstractListModel(parent) {}

int RackStatusModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_devices.size());
}

QVariant RackStatusModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size()) return {};
    const auto &d = m_devices[index.row()];
    switch (role) {
    case NameRole: return d.name;
    case StateRole: return d.state;
    case BlinkRole: return d.blink;
    default: return {};
    }
}

QHash<int, QByteArray> RackStatusModel::roleNames() const {
    return {
        {NameRole, "name"},
        {StateRole, "state"},
        {BlinkRole, "blink"}
    };
}

QVariantMap RackStatusModel::get(int index) const {
    if (index < 0 || index >= m_devices.size()) return {};
    const auto &d = m_devices[index];
    return {
        {"name", d.name},
        {"state", d.state},
        {"blink", d.blink}
    };
}

void RackStatusModel::setDeviceNames(const QStringList &names) {
    beginResetModel();
    m_devices.clear();
    m_devices.reserve(names.size());
    for (const auto &n : names) {
        Device d;
        d.name = n;
        d.state = NormalUnused;
        d.blink = false;
        m_devices.push_back(d);
    }
    endResetModel();
}

void RackStatusModel::updateStates(const QVector<int> &states) {
    const int n = qMin(static_cast<int>(m_devices.size()), static_cast<int>(states.size()));
    if (n <= 0) return;

    bool anyChanged = false;
    for (int i = 0; i < n; i++) {
        const int s = states[i];
        // 只有状态1(正常使用中)才闪烁
        const bool b = (s == NormalInUse);
        if (m_devices[i].state != s || m_devices[i].blink != b) {
            m_devices[i].state = s;
            m_devices[i].blink = b;
            anyChanged = true;
        }
    }

    if (anyChanged) {
        emit dataChanged(this->index(0, 0), this->index(n - 1, 0), {StateRole, BlinkRole});
    }
}

void RackStatusModel::clearAll() {
    if (m_devices.isEmpty()) return;

    bool anyChanged = false;
    for (int i = 0; i < m_devices.size(); i++) {
        if (m_devices[i].state != NormalUnused || m_devices[i].blink != false) {
            m_devices[i].state = NormalUnused;
            m_devices[i].blink = false;
            anyChanged = true;
        }
    }

    if (anyChanged) {
        emit dataChanged(this->index(0, 0), this->index(m_devices.size() - 1, 0), {StateRole, BlinkRole});
    }
}
