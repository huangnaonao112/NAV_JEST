#pragma once

#include <QSqlDatabase>
#include <QString>

class Db {
public:
    Db();
    ~Db();

    bool open();
    QSqlDatabase database() const { return m_db; }

    static QString host();
    static int port();
    static QString user();
    static QString password();
    static QString databaseName();

private:
    QSqlDatabase m_db;
    QString m_connName;

    void ensureSafeConnection();
};
