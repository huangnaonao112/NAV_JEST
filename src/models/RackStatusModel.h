#pragma once

#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QElapsedTimer>
#include <QHash>
#include <QVector>

class RackStatusModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        StateRole,
        BlinkRole
    };
    Q_ENUM(Roles)

    enum DeviceState {
        NormalUnused = 0,
        NormalInUse = 1,
        ErrorUnused = 2,
        ErrorInUse = 3
    };
    Q_ENUM(DeviceState)

    explicit RackStatusModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;

    void setDeviceNames(const QStringList &names);
    void updateStates(const QVector<int> &states);
    void clearAll();

private:
    struct Device {
        QString name;
        int state = NormalUnused;
        bool blink = false;
    };

    QVector<Device> m_devices;
};
