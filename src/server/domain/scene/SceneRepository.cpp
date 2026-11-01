#include "SceneRepository.h"
#include "utils/SceneLogger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QString>

SceneRepository::SceneRepository(QSqlDatabase db)
    : m_db(db) {
    SCENE_LOG_CAT("DB_REPO", QString("SceneRepository 构造函数"));
    SCENE_LOG_CAT("DB_REPO", QString("  m_db.isValid: %1").arg(m_db.isValid() ? "true" : "false"));
    SCENE_LOG_CAT("DB_REPO", QString("  m_db.isOpen: %1").arg(m_db.isOpen() ? "true" : "false"));
    SCENE_LOG_CAT("DB_REPO", QString("  m_db.connectionName: %1").arg(m_db.connectionName()));
}

bool SceneRepository::init() {
    if (!m_db.isValid() || !m_db.isOpen()) return false;
    // 添加扩展字段（如果不存在）
    QSqlQuery q(m_db);
    q.exec("ALTER TABLE TB_Process ADD COLUMN PROCESS_SCENE_TYPE VARCHAR(255) DEFAULT ''");
    q.exec("ALTER TABLE TB_Process ADD COLUMN scene_id VARCHAR(64) UNIQUE");
    q.exec("ALTER TABLE TB_Process ADD COLUMN file_path VARCHAR(512)");
    q.exec("ALTER TABLE TB_Process ADD COLUMN access_device VARCHAR(256)");
    q.exec("ALTER TABLE TB_Process ADD COLUMN description TEXT");
    q.exec("ALTER TABLE TB_Process ADD COLUMN status TINYINT DEFAULT 1");
    q.exec("ALTER TABLE TB_Process ADD COLUMN created_at DATETIME DEFAULT CURRENT_TIMESTAMP");
    q.exec("ALTER TABLE TB_Process ADD COLUMN updated_at DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP");
    return true;
}

QVector<SceneRepository::Scene> SceneRepository::list() const {
    SCENE_LOG_CAT("DB_LIST", QString("SceneRepository::list() 开始查询"));

    QVector<SceneRepository::Scene> out;
    if (!m_db.isValid() || !m_db.isOpen()) {
        SCENE_LOG_ERROR("DB_LIST", QString("数据库无效或未打开!"));
        SCENE_LOG_ERROR("DB_LIST", QString("  m_db.isValid: %1").arg(m_db.isValid() ? "true" : "false"));
        SCENE_LOG_ERROR("DB_LIST", QString("  m_db.isOpen: %1").arg(m_db.isOpen() ? "true" : "false"));
        qWarning() << "SceneRepository::list() - Database not valid or not open";
        return out;
    }

    SCENE_LOG_CAT("DB_LIST", QString("数据库状态正常，执行查询..."));
    SCENE_LOG_CAT("DB_LIST", QString("  连接名: %1").arg(m_db.connectionName()));
    SCENE_LOG_CAT("DB_LIST", QString("  数据库名: %1").arg(m_db.databaseName()));

    QSqlQuery q(m_db);
    QString sql = "SELECT PROCESS_ID, PROCESS_SCENE_NAME, PROCESS_SCENE_TYPE, PROCESS_NAME, "
            "COALESCE(scene_id, ''), COALESCE(file_path, ''), COALESCE(access_device, ''), "
            "COALESCE(description, ''), COALESCE(status, 1), created_at, updated_at "
            "FROM TB_Process ORDER BY PROCESS_ID ASC";

    SCENE_LOG_CAT("DB_LIST", QString("执行 SQL: %1").arg(sql.left(100) + "..."));

    if (!q.exec(sql)) {
        QString errMsg = q.lastError().text();
        SCENE_LOG_ERROR("DB_LIST", QString("SQL 查询失败!"));
        SCENE_LOG_ERROR("DB_LIST", QString("  错误: %1").arg(errMsg));
        SCENE_LOG_ERROR("DB_LIST", QString("  错误类型: %1").arg(static_cast<int>(q.lastError().type())));
        qWarning() << "SceneRepository::list() - Query failed:" << errMsg;
        return out;
    }

    int rowCount = 0;
    while (q.next()) {
        rowCount++;
        Scene s;
        s.processId = q.value(0).toInt();
        s.sceneName = q.value(1).toString();
        s.sceneType = q.value(2).toString();
        s.testName = q.value(3).toString();
        s.sceneId = q.value(4).toString();
        s.filePath = q.value(5).toString();
        s.accessDevice = q.value(6).toString();
        s.description = q.value(7).toString();
        s.status = q.value(8).toInt();
        s.createdAt = QDateTime::fromString(q.value(9).toString(), Qt::ISODateWithMs);
        if (!s.createdAt.isValid()) {
            s.createdAt = QDateTime::fromString(q.value(9).toString(), Qt::ISODate);
        }
        s.updatedAt = QDateTime::fromString(q.value(10).toString(), Qt::ISODateWithMs);
        if (!s.updatedAt.isValid()) {
            s.updatedAt = QDateTime::fromString(q.value(10).toString(), Qt::ISODate);
        }
        out.push_back(s);

        SCENE_LOG_CAT("DB_LIST_ROW", QString("行 %1: id=%2, sceneId=%3, sceneName=%4, sceneType=%5")
            .arg(rowCount)
            .arg(s.processId)
            .arg(s.sceneId)
            .arg(s.sceneName.left(20))
            .arg(s.sceneType));
    }

    SCENE_LOG_CAT("DB_LIST", QString("查询完成，共 %1 行").arg(rowCount));

    return out;
}

bool SceneRepository::exists(const QString &sceneId) const {
    if (!m_db.isValid() || !m_db.isOpen()) {
        qWarning() << "SceneRepository::exists() - Database not valid or not open";
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare("SELECT 1 FROM TB_Process WHERE scene_id = ? LIMIT 1");
    q.addBindValue(sceneId);
    if (!q.exec()) {
        qWarning() << "SceneRepository::exists() - Query failed:" << q.lastError().text();
        return false;
    }
    return q.next();
}

bool SceneRepository::insert(const Scene &scene) {
    if (!m_db.isValid() || !m_db.isOpen()) {
        qWarning() << "SceneRepository::insert() - Database not valid or not open";
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO TB_Process (PROCESS_SCENE_NAME, PROCESS_SCENE_TYPE, PROCESS_NAME, "
        "scene_id, file_path, access_device, description, status, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );
    q.addBindValue(scene.sceneName);
    q.addBindValue(scene.sceneType);
    q.addBindValue(scene.testName);
    q.addBindValue(scene.sceneId);
    q.addBindValue(scene.filePath);
    q.addBindValue(scene.accessDevice);
    q.addBindValue(scene.description);
    q.addBindValue(scene.status);
    q.addBindValue(scene.updatedAt.toString(Qt::ISODateWithMs));

    if (!q.exec()) {
        qWarning() << "SceneRepository::insert() - Failed:" << q.lastError().text();
        return false;
    }

    return true;
}

bool SceneRepository::update(const QString &sceneId, const Scene &scene) {
    if (!m_db.isValid() || !m_db.isOpen()) return false;

    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE TB_Process SET "
        " PROCESS_SCENE_NAME = ?,"
        " PROCESS_SCENE_TYPE = ?,"
        " PROCESS_NAME = ?,"
        " scene_id = ?,"
        " file_path = ?,"
        " access_device = ?,"
        " description = ?,"
        " status = ?,"
        " updated_at = ?"
        " WHERE scene_id = ?"
    );
    q.addBindValue(scene.sceneName);
    q.addBindValue(scene.sceneType);
    q.addBindValue(scene.testName);
    q.addBindValue(scene.sceneId);
    q.addBindValue(scene.filePath);
    q.addBindValue(scene.accessDevice);
    q.addBindValue(scene.description);
    q.addBindValue(scene.status);
    q.addBindValue(scene.updatedAt.toString(Qt::ISODateWithMs));
    q.addBindValue(sceneId);

    return q.exec();
}

bool SceneRepository::remove(const QString &sceneId) {
    if (!m_db.isValid() || !m_db.isOpen()) return false;

    // 先获取 PROCESS_ID
    QSqlQuery getIdQuery(m_db);
    getIdQuery.prepare("SELECT PROCESS_ID FROM TB_Process WHERE scene_id = ?");
    getIdQuery.addBindValue(sceneId);
    if (!getIdQuery.exec() || !getIdQuery.next()) {
        return false;
    }
    int processId = getIdQuery.value(0).toInt();

    // 先删除 TB_PROCESSDETAIL 中的关联数据
    QSqlQuery detailQuery(m_db);
    detailQuery.prepare("DELETE FROM TB_PROCESSDETAIL WHERE Process_ID = ?");
    detailQuery.addBindValue(processId);
    detailQuery.exec();

    // 再删除 TB_Process 中的主记录
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM TB_Process WHERE scene_id = ?");
    q.addBindValue(sceneId);
    return q.exec();
}