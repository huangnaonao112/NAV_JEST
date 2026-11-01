#include "Db.h"
#include "../Config.h"
#include "utils/SceneLogger.h"

#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QThread>
#include <QtGlobal>
#include <QString>

QString Db::host() {
    return Config::instance().dbHost();
}

int Db::port() {
    return Config::instance().dbPort();
}

QString Db::user() {
    return Config::instance().dbUser();
}

QString Db::password() {
    return Config::instance().dbPassword();
}

QString Db::databaseName() {
    return Config::instance().dbName();
}

void Db::ensureSafeConnection() {
    const auto tid = reinterpret_cast<quintptr>(QThread::currentThreadId());
    // 进程内单调递增序号保证连接名绝对唯一：addDatabase 遇到同名连接会隐式
    // 移除旧连接，而其他对象可能仍持有其 QSqlDatabase 副本（未定义行为）
    static QAtomicInteger<quintptr> s_seq(0);
    m_connName = QStringLiteral("navjest_mysql_%1_%2_%3")
                     .arg(QDateTime::currentMSecsSinceEpoch())
                     .arg(tid)
                     .arg(s_seq.fetchAndAddRelaxed(1));

    SCENE_LOG_CAT("DB_CONN", QString("ensureSafeConnection: threadId=%1, connName=%2")
        .arg(tid).arg(m_connName));

    m_db = QSqlDatabase::addDatabase("QMYSQL", m_connName);
    SCENE_LOG_CAT("DB_CONN", QString("Created new MySQL connection object: %1").arg(m_connName));
}

Db::Db() {
    ensureSafeConnection();
}

Db::~Db() {
    // 只 close，不 removeDatabase：仓库对象持有 QSqlDatabase 副本，析构顺序
    // 难以保证副本先于连接消亡；带活跃副本 removeDatabase 是 Qt 文档明示的
    // 未定义行为（退出期段错误的根源）。连接随进程结束由系统回收。
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool Db::open() {
    SCENE_LOG_CAT("DB_OPEN", QString("开始连接数据库..."));
    SCENE_LOG_CAT("DB_OPEN", QString("  主机: %1").arg(host()));
    SCENE_LOG_CAT("DB_OPEN", QString("  端口: %1").arg(port()));
    SCENE_LOG_CAT("DB_OPEN", QString("  用户: %1").arg(user()));
    SCENE_LOG_CAT("DB_OPEN", QString("  密码: %1").arg(password().isEmpty() ? "(空)" : "******"));
    SCENE_LOG_CAT("DB_OPEN", QString("  数据库: %1").arg(databaseName()));
    SCENE_LOG_CAT("DB_OPEN", QString("  连接名: %1").arg(m_connName));

    m_db.setHostName(host());
    m_db.setPort(port());
    m_db.setUserName(user());
    m_db.setPassword(password());
    m_db.setDatabaseName(databaseName());
    m_db.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=10");

    SCENE_LOG_CAT("DB_OPEN", QString("正在调用 db.open()..."));

    if (!m_db.open()) {
        QString errMsg = m_db.lastError().text();
        SCENE_LOG_ERROR("DB_OPEN", QString("数据库连接失败!"));
        SCENE_LOG_ERROR("DB_OPEN", QString("  错误类型: %1").arg(static_cast<int>(m_db.lastError().type())));
        SCENE_LOG_ERROR("DB_OPEN", QString("  错误信息: %1").arg(errMsg));
        SCENE_LOG_ERROR("DB_OPEN", QString("  驱动状态: %1").arg(m_db.isValid() ? "有效" : "无效"));
        qCritical() << "Db: failed to connect to MySQL:" << host() << ":" << port();
        qCritical() << "Db: error:" << errMsg;
        return false;
    }

    SCENE_LOG_CAT("DB_OPEN", QString("数据库连接成功!"));
    SCENE_LOG_CAT("DB_OPEN", QString("  连接状态: %1").arg(m_db.isOpen() ? "已打开" : "未打开"));
    SCENE_LOG_CAT("DB_OPEN", QString("  数据库名: %1").arg(m_db.databaseName()));
    return true;
}
