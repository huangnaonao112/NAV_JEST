#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

class UserRepository {
public:
    struct User {
        QString userId;         // User_ID - 用户ID（主键）
        QString userName;       // User_Name - 用户名
        QString userPassword;   // User_Password - 密码
        int userLevel;          // User_Level - 用户级别（0=超级用户, 1=管理员, 2=操作员）
        QString phone;          // phone - 电话（扩展字段）
        QString dept;           // dept - 部门（扩展字段）
        QString status;         // status - 状态（扩展字段）
        QDateTime updatedAt;
    };

    explicit UserRepository(QSqlDatabase db);

    bool init();

    QVector<User> list() const;

    bool insert(const User &user);
    bool update(const QString &userId, const User &user);
    bool remove(const QString &userId);
    bool exists(const QString &userId) const;

private:
    QSqlDatabase m_db;
};
