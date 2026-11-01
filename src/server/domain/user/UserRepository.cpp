#include "UserRepository.h"

#include <QDebug>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

UserRepository::UserRepository(QSqlDatabase db)
    : m_db(db) {
}

bool UserRepository::init() {
    // 表由 TestSystem 创建，这里只检查连接
    return m_db.isValid() && m_db.isOpen();
}

QVector<UserRepository::User> UserRepository::list() const {
    QVector<UserRepository::User> out;
    if (!m_db.isValid() || !m_db.isOpen()) return out;

    QSqlQuery q(m_db);
    // tb_userinfo 表字段：USER_ID, USER_NAME, USER_PASSWORD, USER_LEVEL, phone, dept, status, updated_at
    if (!q.exec(
            "SELECT USER_ID, USER_NAME, USER_PASSWORD, USER_LEVEL, phone, dept, status, updated_at "
            "FROM tb_userinfo ORDER BY USER_ID ASC")) {
        return out;
    }

    while (q.next()) {
        User u;
        u.userId = q.value(0).toString();
        u.userName = q.value(1).toString();
        u.userPassword = q.value(2).toString();
        u.userLevel = q.value(3).toInt();
        u.phone = q.value(4).toString();
        u.dept = q.value(5).toString();
        u.status = q.value(6).toString();
        u.updatedAt = QDateTime::fromString(q.value(7).toString(), Qt::ISODate);
        if (!u.updatedAt.isValid()) {
            u.updatedAt = QDateTime::currentDateTime();
        }
        out.push_back(u);
    }

    return out;
}

bool UserRepository::exists(const QString &userId) const {
    if (!m_db.isValid() || !m_db.isOpen()) return false;
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM tb_userinfo WHERE USER_ID = ? LIMIT 1");
    q.addBindValue(userId);
    if (!q.exec()) {
        return false;
    }
    return q.next();
}

bool UserRepository::insert(const User &user) {
    if (!m_db.isValid() || !m_db.isOpen()) return false;

    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO tb_userinfo (USER_ID, USER_NAME, USER_PASSWORD, USER_LEVEL, phone, dept, status, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    );
    q.addBindValue(user.userId);
    q.addBindValue(user.userName);
    q.addBindValue(user.userPassword);
    q.addBindValue(user.userLevel);
    q.addBindValue(user.phone);
    q.addBindValue(user.dept);
    q.addBindValue(user.status);
    q.addBindValue(user.updatedAt.toString(Qt::ISODate));
    if (!q.exec()) {
        return false;
    }
    return true;
}

bool UserRepository::update(const QString &userId, const User &user) {
    if (!m_db.isValid() || !m_db.isOpen()) return false;

    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE tb_userinfo SET "
        " USER_ID = ?,"
        " USER_NAME = ?,"
        " USER_PASSWORD = ?,"
        " USER_LEVEL = ?,"
        " phone = ?,"
        " dept = ?,"
        " status = ?,"
        " updated_at = ?"
        " WHERE USER_ID = ?"
    );
    q.addBindValue(user.userId);
    q.addBindValue(user.userName);
    q.addBindValue(user.userPassword);
    q.addBindValue(user.userLevel);
    q.addBindValue(user.phone);
    q.addBindValue(user.dept);
    q.addBindValue(user.status);
    q.addBindValue(user.updatedAt.toString(Qt::ISODate));
    q.addBindValue(userId);

    if (!q.exec()) {
        return false;
    }
    return true;
}

bool UserRepository::remove(const QString &userId) {
    if (!m_db.isValid() || !m_db.isOpen()) return false;

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM tb_userinfo WHERE USER_ID = ?");
    q.addBindValue(userId);
    if (!q.exec()) {
        return false;
    }
    return true;
}
