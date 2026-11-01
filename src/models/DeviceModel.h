#pragma once

#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

class Db;
class DeviceRepository;

class DeviceModel : public QAbstractListModel {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
public:
    enum Roles {
        DeviceIdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        ModelRole,
        VendorRole,
        ConnectionRole,
        AddressRole,
        UpdatedAtRole
    };

    explicit DeviceModel(QObject *parent = nullptr);
    ~DeviceModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE bool add(const QString &deviceId,
                         const QString &name,
                         const QString &type,
                         const QString &model,
                         const QString &vendor,
                         const QString &connection,
                         const QString &address);

    Q_INVOKABLE bool update(int index,
                            const QString &deviceId,
                            const QString &name,
                            const QString &type,
                            const QString &model,
                            const QString &vendor,
                            const QString &connection,
                            const QString &address);

    Q_INVOKABLE bool removeAt(int index);

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool save() const;

    Q_INVOKABLE QString storagePath() const;

private:
    struct Device {
        QString deviceId;
        QString name;
        QString type;
        QString model;
        QString vendor;
        QString connection;
        QString address;
        QDateTime updatedAt;
    };

    int findByDeviceId(const QString &deviceId) const;
    static QString isoString(const QDateTime &dt);
    static QDateTime fromIsoString(const QString &s);

    QVector<Device> m_devices;

    Db *m_db;
    QSqlDatabase m_sql;
    DeviceRepository *m_repo;
};
