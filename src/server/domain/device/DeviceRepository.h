#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

class DeviceRepository {
public:
    struct Device {
        QString id;
        QString name;
        QString type;
        QString model;
        QString vendor;
        QString connection;
        QString address;
        QDateTime updatedAt;
    };

    explicit DeviceRepository(QSqlDatabase db);

    bool init();

    QVector<Device> list() const;
    bool insert(const Device &d);
    bool update(const QString &id, const Device &d);
    bool remove(const QString &id);
    bool exists(const QString &id) const;

private:
    QSqlDatabase m_db;
};
