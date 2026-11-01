#include "DeviceRepository.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

static QString toIso(const QDateTime &dt) {
    return dt.toString(Qt::ISODateWithMs);
}

static QDateTime fromIso(const QString &s) {
    QDateTime dt = QDateTime::fromString(s, Qt::ISODateWithMs);
    if (!dt.isValid()) dt = QDateTime::fromString(s, Qt::ISODate);
    return dt;
}

DeviceRepository::DeviceRepository(QSqlDatabase db)
    : m_db(std::move(db)) {
}

bool DeviceRepository::init() {
    // 表由 schema.sql 在 MySQL 中创建，这里只检查连接
    return m_db.isValid() && m_db.isOpen();
}

QVector<DeviceRepository::Device> DeviceRepository::list() const {
    QVector<Device> out;
    QSqlQuery q(m_db);
    q.exec("SELECT id,name,type,model,vendor,connection,address,updated_at FROM devices ORDER BY id");
    while (q.next()) {
        Device d;
        d.id = q.value(0).toString();
        d.name = q.value(1).toString();
        d.type = q.value(2).toString();
        d.model = q.value(3).toString();
        d.vendor = q.value(4).toString();
        d.connection = q.value(5).toString();
        d.address = q.value(6).toString();
        d.updatedAt = fromIso(q.value(7).toString());
        out.push_back(d);
    }
    return out;
}

bool DeviceRepository::exists(const QString &id) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM devices WHERE id=? LIMIT 1");
    q.addBindValue(id);
    if (!q.exec()) return false;
    return q.next();
}

bool DeviceRepository::insert(const Device &d) {
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO devices(id,name,type,model,vendor,connection,address,updated_at) VALUES(?,?,?,?,?,?,?,?)");
    q.addBindValue(d.id);
    q.addBindValue(d.name);
    q.addBindValue(d.type);
    q.addBindValue(d.model);
    q.addBindValue(d.vendor);
    q.addBindValue(d.connection);
    q.addBindValue(d.address);
    q.addBindValue(toIso(d.updatedAt));
    return q.exec();
}

bool DeviceRepository::update(const QString &id, const Device &d) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE devices SET name=?,type=?,model=?,vendor=?,connection=?,address=?,updated_at=? WHERE id=?");
    q.addBindValue(d.name);
    q.addBindValue(d.type);
    q.addBindValue(d.model);
    q.addBindValue(d.vendor);
    q.addBindValue(d.connection);
    q.addBindValue(d.address);
    q.addBindValue(toIso(d.updatedAt));
    q.addBindValue(id);
    return q.exec();
}

bool DeviceRepository::remove(const QString &id) {
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM devices WHERE id=?");
    q.addBindValue(id);
    return q.exec();
}
