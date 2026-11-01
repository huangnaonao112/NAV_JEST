#include "SceneConfigRepository.h"
#include "utils/SceneLogger.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QDebug>

SceneConfigRepository::SceneConfigRepository(QSqlDatabase db, QObject *parent)
    : QObject(parent), m_db(db) {
}

SceneConfigRepository::~SceneConfigRepository() = default;

// ========== 场景主表操作 ==========

bool SceneConfigRepository::createScene(const QString& sceneId, const QString& sceneName,
                                         const QString& sceneType, const QString& testName,
                                         const QString& description) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO TB_Process (scene_id, PROCESS_SCENE_NAME, PROCESS_SCENE_TYPE, PROCESS_NAME, description, status)
        VALUES (?, ?, ?, ?, ?, 1)
    )");
    query.addBindValue(sceneId);
    query.addBindValue(sceneName);
    query.addBindValue(sceneType);
    query.addBindValue(testName.isEmpty() ? "定位精度" : testName);
    query.addBindValue(description);

    if (!query.exec()) {
        qWarning() << "Failed to create scene:" << query.lastError().text();
        emit errorOccurred(query.lastError().text());
        return false;
    }

    emit sceneCreated(sceneId);
    return true;
}

SceneConfigRepository::SceneInfo SceneConfigRepository::getSceneInfo(const QString& sceneId) {
    SceneInfo info;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM TB_Process WHERE scene_id = ?");
    query.addBindValue(sceneId);

    if (!query.exec() || !query.next()) {
        return info;
    }

    info.processId = query.value("PROCESS_ID").toInt();
    info.sceneId = query.value("scene_id").toString();
    info.sceneName = query.value("PROCESS_SCENE_NAME").toString();
    info.sceneType = query.value("PROCESS_SCENE_TYPE").toString();
    info.testName = query.value("PROCESS_NAME").toString();
    info.description = query.value("description").toString();
    info.status = query.value("status").toInt();
    info.createdAt = query.value("created_at").toDateTime();
    info.updatedAt = query.value("updated_at").toDateTime();

    return info;
}

bool SceneConfigRepository::updateScene(const QString& sceneId, const QString& field, const QVariant& value) {
    QSqlQuery query(m_db);
    query.prepare(QString("UPDATE TB_Process SET %1 = ? WHERE scene_id = ?").arg(field));
    query.addBindValue(value);
    query.addBindValue(sceneId);

    if (!query.exec()) {
        qWarning() << "Failed to update scene:" << query.lastError().text();
        return false;
    }

    emit sceneUpdated(sceneId);
    return query.numRowsAffected() > 0;
}

bool SceneConfigRepository::deleteScene(const QString& sceneId) {
    // 先获取 Process_ID
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        qWarning() << "Failed to delete scene: scene not found" << sceneId;
        return false;
    }

    // 先删除 TB_PROCESSDETAIL 中的关联数据
    QSqlQuery detailQuery(m_db);
    detailQuery.prepare("DELETE FROM TB_PROCESSDETAIL WHERE Process_ID = ?");
    detailQuery.addBindValue(processId);
    if (!detailQuery.exec()) {
        qWarning() << "Failed to delete scene details:" << detailQuery.lastError().text();
        return false;
    }

    // 再删除 TB_Process 中的主记录
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM TB_Process WHERE scene_id = ?");
    query.addBindValue(sceneId);

    if (!query.exec()) {
        qWarning() << "Failed to delete scene:" << query.lastError().text();
        return false;
    }

    emit sceneDeleted(sceneId);
    return query.numRowsAffected() > 0;
}

QVector<SceneConfigRepository::SceneInfo> SceneConfigRepository::listAllScenes() {
    QVector<SceneInfo> scenes;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM TB_Process ORDER BY created_at DESC");

    if (!query.exec()) {
        return scenes;
    }

    while (query.next()) {
        SceneInfo info;
        info.processId = query.value("PROCESS_ID").toInt();
        info.sceneId = query.value("scene_id").toString();
        info.sceneName = query.value("PROCESS_SCENE_NAME").toString();
        info.sceneType = query.value("PROCESS_SCENE_TYPE").toString();
        info.testName = query.value("PROCESS_NAME").toString();
        info.description = query.value("description").toString();
        info.status = query.value("status").toInt();
        info.createdAt = query.value("created_at").toDateTime();
        info.updatedAt = query.value("updated_at").toDateTime();
        scenes.append(info);
    }

    return scenes;
}

QVector<SceneConfigRepository::SceneInfo> SceneConfigRepository::listScenesBySceneType(const QString& sceneType) {
    QVector<SceneInfo> scenes;
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM TB_Process WHERE PROCESS_SCENE_TYPE = ? ORDER BY PROCESS_SCENE_NAME");
    query.addBindValue(sceneType);

    if (!query.exec()) {
        return scenes;
    }

    while (query.next()) {
        SceneInfo info;
        info.processId = query.value("PROCESS_ID").toInt();
        info.sceneId = query.value("scene_id").toString();
        info.sceneName = query.value("PROCESS_SCENE_NAME").toString();
        info.sceneType = query.value("PROCESS_SCENE_TYPE").toString();
        info.testName = query.value("PROCESS_NAME").toString();
        info.description = query.value("description").toString();
        info.status = query.value("status").toInt();
        info.createdAt = query.value("created_at").toDateTime();
        info.updatedAt = query.value("updated_at").toDateTime();
        scenes.append(info);
    }

    return scenes;
}

QVector<SceneConfigRepository::SceneInfo> SceneConfigRepository::listScenesByProcessSet(const QString& processSetName) {
    // 保留向后兼容，实际调用 listScenesBySceneType
    return listScenesBySceneType(processSetName);
}

// ========== 场景步骤操作 ==========

int SceneConfigRepository::getProcessIdBySceneId(const QString& sceneId) {
    QSqlQuery query(m_db);
    query.prepare("SELECT Process_ID FROM TB_Process WHERE scene_id = ?");
    query.addBindValue(sceneId);

    if (!query.exec() || !query.next()) {
        SCENE_LOG_ERROR("GET_PROCESS_ID", QString("sceneId=%1 not found in TB_Process").arg(sceneId));
        return -1;
    }

    int processId = query.value(0).toInt();
    SCENE_LOG_DB("GET_PROCESS_ID", QString("sceneId=%1 -> Process_ID=%2").arg(sceneId).arg(processId));
    return processId;
}

bool SceneConfigRepository::addStep(const QString& sceneId, int stepIndex, const QJsonObject& stepConfig,
                                     const QString& stepName, const QString& stepType) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        SCENE_LOG_ERROR("ADD_STEP", QString("sceneId=%1: Process_ID not found").arg(sceneId));
        return false;
    }

    QJsonDocument doc(stepConfig);
    QString configJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO TB_PROCESSDETAIL (OPT_INDEX, Process_ID, OPT_NAME, OPT_TYPE, OPT_CONTENT)
        VALUES (?, ?, ?, ?, ?)
        ON DUPLICATE KEY UPDATE
            OPT_NAME = VALUES(OPT_NAME),
            OPT_TYPE = VALUES(OPT_TYPE),
            OPT_CONTENT = VALUES(OPT_CONTENT),
            updated_at = CURRENT_TIMESTAMP
    )");
    query.addBindValue(stepIndex);
    query.addBindValue(processId);
    query.addBindValue(stepName);
    query.addBindValue(stepType);
    query.addBindValue(configJson);

    if (!query.exec()) {
        QString err = query.lastError().text();
        qWarning() << "Failed to add step:" << err;
        SCENE_LOG_ERROR("ADD_STEP_SQL", QString("sceneId=%1, step=%2, error=%3").arg(sceneId, stepName, err));
        return false;
    }

    SCENE_LOG_DB("ADD_STEP_OK", QString("Process_ID=%1, step[%2]=%3").arg(processId).arg(stepIndex).arg(stepName));
    emit stepAdded(sceneId, stepIndex);
    return true;
}

bool SceneConfigRepository::addSteps(const QString& sceneId, const QVector<SceneStep>& steps) {
    if (!beginTransaction()) {
        return false;
    }

    bool success = true;
    for (const auto& step : steps) {
        if (!addStep(sceneId, step.optIndex, step.optContent, step.optName, step.optType)) {
            success = false;
            break;
        }
    }

    if (success) {
        commitTransaction();
    } else {
        rollbackTransaction();
    }

    return success;
}

QVector<SceneStep> SceneConfigRepository::getSceneSteps(const QString& sceneId) {
    QVector<SceneStep> steps;

    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        SCENE_LOG_ERROR("GET_STEPS", QString("sceneId=%1: Process_ID not found").arg(sceneId));
        return steps;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM TB_PROCESSDETAIL WHERE Process_ID = ? ORDER BY OPT_INDEX ASC");
    query.addBindValue(processId);

    if (!query.exec()) {
        QString err = query.lastError().text();
        SCENE_LOG_ERROR("GET_STEPS_SQL", QString("Process_ID=%1, error=%2").arg(processId).arg(err));
        return steps;
    }

    while (query.next()) {
        steps.append(parseStepFromQuery(query));
    }

    SCENE_LOG_DB("GET_STEPS_OK", QString("Process_ID=%1, found %2 steps").arg(processId).arg(steps.size()));
    return steps;
}

SceneStep SceneConfigRepository::getStep(const QString& sceneId, int stepIndex) {
    SceneStep step;

    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        return step;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM TB_PROCESSDETAIL WHERE Process_ID = ? AND OPT_INDEX = ?");
    query.addBindValue(processId);
    query.addBindValue(stepIndex);

    if (!query.exec() || !query.next()) {
        return step;
    }

    return parseStepFromQuery(query);
}

bool SceneConfigRepository::updateStepConfig(const QString& sceneId, int stepIndex, const QJsonObject& stepConfig) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        return false;
    }

    QJsonDocument doc(stepConfig);
    QString configJson = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE TB_PROCESSDETAIL
        SET OPT_CONTENT = ?, updated_at = CURRENT_TIMESTAMP
        WHERE Process_ID = ? AND OPT_INDEX = ?
    )");
    query.addBindValue(configJson);
    query.addBindValue(processId);
    query.addBindValue(stepIndex);

    if (!query.exec()) {
        qWarning() << "Failed to update step config:" << query.lastError().text();
        return false;
    }

    emit stepUpdated(sceneId, stepIndex);
    return query.numRowsAffected() > 0;
}

bool SceneConfigRepository::updateStepJsonField(const QString& sceneId, int stepIndex,
                                                 const QString& jsonPath, const QVariant& value) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        return false;
    }

    QString valueStr = value.toString();
    if (value.typeId() == QMetaType::QString) {
        valueStr = "\"" + value.toString() + "\"";
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE TB_PROCESSDETAIL
        SET OPT_CONTENT = JSON_SET(OPT_CONTENT, ?, CAST(? AS JSON)),
            updated_at = CURRENT_TIMESTAMP
        WHERE Process_ID = ? AND OPT_INDEX = ?
    )");
    query.addBindValue(jsonPath);
    query.addBindValue(valueStr);
    query.addBindValue(processId);
    query.addBindValue(stepIndex);

    if (!query.exec()) {
        qWarning() << "Failed to update JSON field:" << query.lastError().text();
        return false;
    }

    emit stepUpdated(sceneId, stepIndex);
    return query.numRowsAffected() > 0;
}

bool SceneConfigRepository::deleteStep(const QString& sceneId, int stepIndex) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM TB_PROCESSDETAIL WHERE Process_ID = ? AND OPT_INDEX = ?");
    query.addBindValue(processId);
    query.addBindValue(stepIndex);

    if (!query.exec()) {
        qWarning() << "Failed to delete step:" << query.lastError().text();
        return false;
    }

    emit stepDeleted(sceneId, stepIndex);
    return query.numRowsAffected() > 0;
}

bool SceneConfigRepository::deleteSceneSteps(const QString& sceneId) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        SCENE_LOG_ERROR("DELETE_STEPS", QString("sceneId=%1: Process_ID not found").arg(sceneId));
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM TB_PROCESSDETAIL WHERE Process_ID = ?");
    query.addBindValue(processId);

    bool ok = query.exec();
    if (!ok) {
        QString err = query.lastError().text();
        SCENE_LOG_ERROR("DELETE_STEPS_SQL", QString("Process_ID=%1, error=%2").arg(processId).arg(err));
    } else {
        SCENE_LOG_DB("DELETE_STEPS_OK", QString("Process_ID=%1, deleted").arg(processId));
    }
    return ok;
}

// ========== JSON 查询操作 ==========

QVariant SceneConfigRepository::getJsonFieldValue(const QString& sceneId, int stepIndex, const QString& fieldPath) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        return QVariant();
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT JSON_UNQUOTE(JSON_EXTRACT(OPT_CONTENT, ?)) AS value
        FROM TB_PROCESSDETAIL
        WHERE Process_ID = ? AND OPT_INDEX = ?
    )");
    query.addBindValue(fieldPath);
    query.addBindValue(processId);
    query.addBindValue(stepIndex);

    if (!query.exec() || !query.next()) {
        return QVariant();
    }

    return query.value("value");
}

QVector<SceneStep> SceneConfigRepository::searchStepsByJsonField(const QString& fieldPath, const QVariant& value) {
    QVector<SceneStep> steps;

    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT * FROM TB_PROCESSDETAIL
        WHERE JSON_UNQUOTE(JSON_EXTRACT(OPT_CONTENT, ?)) = ?
        ORDER BY Process_ID, OPT_INDEX
    )");
    query.addBindValue(fieldPath);
    query.addBindValue(value.toString());

    if (!query.exec()) {
        return steps;
    }

    while (query.next()) {
        steps.append(parseStepFromQuery(query));
    }

    return steps;
}

QVector<SceneStep> SceneConfigRepository::searchStepsByJsonKey(const QString& jsonKey) {
    QVector<SceneStep> steps;

    QSqlQuery query(m_db);
    QString fullPath = "$." + jsonKey;
    query.prepare(R"(
        SELECT * FROM TB_PROCESSDETAIL
        WHERE JSON_CONTAINS_PATH(OPT_CONTENT, 'one', ?)
        ORDER BY Process_ID, OPT_INDEX
    )");
    query.addBindValue(fullPath);

    if (!query.exec()) {
        return steps;
    }

    while (query.next()) {
        steps.append(parseStepFromQuery(query));
    }

    return steps;
}

// ========== 批量操作 ==========

bool SceneConfigRepository::cloneScene(const QString& sourceSceneId, const QString& newSceneId) {
    if (!beginTransaction()) {
        return false;
    }

    bool success = true;

    SceneInfo sourceInfo = getSceneInfo(sourceSceneId);
    if (sourceInfo.sceneId.isEmpty()) {
        rollbackTransaction();
        return false;
    }

    if (!createScene(newSceneId, sourceInfo.sceneName + "_副本",
                     sourceInfo.sceneType, sourceInfo.testName, sourceInfo.description)) {
        rollbackTransaction();
        return false;
    }

    QVector<SceneStep> steps = getSceneSteps(sourceSceneId);
    for (const auto& step : steps) {
        if (!addStep(newSceneId, step.optIndex, step.optContent, step.optName, step.optType)) {
            success = false;
            break;
        }
    }

    if (success) {
        commitTransaction();
    } else {
        rollbackTransaction();
    }

    return success;
}

bool SceneConfigRepository::importSceneConfig(const QString& sceneId, const QJsonArray& stepsConfig) {
    if (!beginTransaction()) {
        return false;
    }

    bool success = true;

    if (!deleteSceneSteps(sceneId)) {
        rollbackTransaction();
        return false;
    }

    for (int i = 0; i < stepsConfig.size(); ++i) {
        QJsonObject stepObj = stepsConfig[i].toObject();
        int stepIndex = stepObj.value("step_index").toInt(i);
        QJsonObject config = stepObj.value("step_config").toObject();
        QString stepName = stepObj.value("step_name").toString();
        QString stepType = stepObj.value("step_type").toString();

        if (!addStep(sceneId, stepIndex, config, stepName, stepType)) {
            success = false;
            break;
        }
    }

    if (success) {
        commitTransaction();
    } else {
        rollbackTransaction();
    }

    return success;
}

QJsonArray SceneConfigRepository::exportSceneConfig(const QString& sceneId) {
    QJsonArray result;
    QVector<SceneStep> steps = getSceneSteps(sceneId);

    for (const auto& step : steps) {
        QJsonObject obj;
        obj["opt_index"] = step.optIndex;
        obj["process_id"] = step.processId;
        obj["opt_name"] = step.optName;
        obj["opt_type"] = step.optType;
        obj["opt_content"] = step.optContent;
        result.append(obj);
    }

    return result;
}

// ========== 事务支持 ==========

bool SceneConfigRepository::beginTransaction() {
    if (m_inTransaction) {
        return true;
    }

    if (!m_db.isValid() || !m_db.isOpen()) {
        return false;
    }

    bool result = m_db.transaction();
    if (!result) {
        QSqlQuery query(m_db);
        if (query.exec("START TRANSACTION")) {
            m_inTransaction = true;
            return true;
        }
        return false;
    }

    m_inTransaction = result;
    return m_inTransaction;
}

bool SceneConfigRepository::commitTransaction() {
    if (!m_inTransaction) {
        return false;
    }

    bool success = m_db.commit();
    if (!success) {
        QSqlQuery query(m_db);
        if (query.exec("COMMIT")) {
            success = true;
        }
    }

    m_inTransaction = false;
    return success;
}

bool SceneConfigRepository::rollbackTransaction() {
    if (!m_inTransaction) {
        return false;
    }

    bool success = m_db.rollback();
    if (!success) {
        QSqlQuery query(m_db);
        if (query.exec("ROLLBACK")) {
            success = true;
        }
    }

    m_inTransaction = false;
    return success;
}

// ========== 辅助方法 ==========

SceneStep SceneConfigRepository::parseStepFromQuery(const QSqlQuery& query) {
    SceneStep step;
    step.optIndex = query.value("OPT_INDEX").toInt();
    step.processId = query.value("Process_ID").toInt();
    step.optName = query.value("OPT_NAME").toString();
    step.optType = query.value("OPT_TYPE").toString();
    step.createdAt = query.value("created_at").toDateTime();
    step.updatedAt = query.value("updated_at").toDateTime();

    QString configJson = query.value("OPT_CONTENT").toString();
    if (!configJson.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(configJson.toUtf8());
        step.optContent = doc.object();
    }

    return step;
}

bool SceneConfigRepository::stepExists(const QString& sceneId, int stepIndex) {
    int processId = getProcessIdBySceneId(sceneId);
    if (processId < 0) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM TB_PROCESSDETAIL WHERE Process_ID = ? AND OPT_INDEX = ? LIMIT 1");
    query.addBindValue(processId);
    query.addBindValue(stepIndex);

    if (!query.exec()) {
        return false;
    }

    return query.next();
}