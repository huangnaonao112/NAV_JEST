#include "SceneService.h"
#include "SceneRepository.h"
#include "SceneDetailMapper.h"
#include "utils/SceneLogger.h"
#include "utils/CycleDataLogger.h"

#include <QDateTime>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SceneService::SceneService(QSqlDatabase db, QObject *parent)
    : QObject(parent)
    , m_db(db)
    , m_configRepo(new SceneConfigRepository(db))
{
}

SceneService::~SceneService()
{
    delete m_configRepo;
    m_configRepo = nullptr;
}

bool SceneService::saveDetail(const QString &sceneId, const QVariantMap &detailData)
{
    if (!m_configRepo) {
        SCENE_LOG_ERROR("saveDetail", "m_configRepo is null");
        CYCLE_LOG_ERROR("saveDetail", "m_configRepo is null", sceneId);
        return false;
    }

    // 获取测试类型
    QString testType = detailData.value("testType").toString();
    QString sceneType = detailData.value("sceneType").toString();

    // ★ 文件日志（不输出到控制台）
    SCENE_LOG_CAT("SAVE_START", QString("sceneId=%1, sceneType=%2, testType=%3")
              .arg(sceneId, sceneType, testType));

    // 数据流转追踪
    QString flowId = CYCLE_FLOW_BEGIN(sceneId, "SERVICE_SAVE");
    CYCLE_FLOW_NODE(flowId, "SERVICE_INPUT", detailData);

    // ★ 先删除所有旧步骤，避免残留数据
    bool deleteOk = m_configRepo->deleteSceneSteps(sceneId);
    SCENE_LOG_CAT("DELETE_OLD", QString("deleteSceneSteps result: %1").arg(deleteOk ? "OK" : "FAIL"));

    if (!deleteOk) {
        CYCLE_LOG_WARNING("deleteSceneSteps", "Failed to delete old steps", sceneId);
    }

    // 使用新方法构建步骤（返回连续索引的步骤列表）
    QVector<SceneDetailMapper::StepBuildResult> steps = SceneDetailMapper::buildSteps(detailData);
    SCENE_LOG_CAT("BUILD_STEPS", QString("built %1 steps").arg(steps.size()));

    // 创建新步骤
    int addedCount = 0;
    for (const auto &step : steps) {
        if (step.optName.isEmpty()) {
            continue;
        }

        if (m_configRepo->addStep(sceneId, step.optIndex, step.optContent, step.optName, step.optType)) {
            addedCount++;
            SCENE_LOG_CAT("ADD_STEP", QString("step[%1] %2 - OK").arg(step.optIndex).arg(step.optName));
        } else {
            SCENE_LOG_ERROR("ADD_STEP", QString("step[%1] %2 - FAILED").arg(step.optIndex).arg(step.optName));
            CYCLE_LOG_ERROR("addStep", QString("step[%1] %2 failed").arg(step.optIndex).arg(step.optName), sceneId);
        }
    }

    SCENE_LOG_CAT("SAVE_TOTAL", QString("Total added: %1 steps").arg(addedCount));
    SCENE_LOG_SAVE(sceneId, sceneType, testType, addedCount);

    // ★ 更新主表的时间戳，确保持久化记录被更新
    if (addedCount > 0) {
        bool updateOk = m_configRepo->updateScene(sceneId, "updated_at", QDateTime::currentDateTime().toString(Qt::ISODate));
        SCENE_LOG_CAT("UPDATE_TIMESTAMP", QString("result: %1").arg(updateOk ? "OK" : "FAIL"));

        // 记录数据库更新操作
        CYCLE_LOG_DB_UPDATE(sceneId, "updated_at", "", QDateTime::currentDateTime().toString(Qt::ISODate));
    }

    // ★ 保存JSON配置文件到exe同目录的config文件夹（默认路径）
    if (addedCount > 0) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString configDir = appDir + "/config";

        // 确保config目录存在
        QDir dir(configDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // 构建JSON文件路径（使用场景编号命名）
        QString jsonPath = configDir + "/" + sceneId + ".json";

        // ★ 更新数据库中的场景存储路径为完整JSON文件路径
        // 更新"场景配置"步骤的"场景存储路径"字段
        for (const auto &step : steps) {
            if (step.optName == "场景配置") {
                QJsonObject content = step.optContent;
                content["场景存储路径"] = jsonPath;
                m_configRepo->updateStepConfig(sceneId, step.optIndex, content);
                break;
            }
        }

        // 构建JSON内容（使用数据库格式）
        QJsonObject root;
        root["sceneName"] = detailData.value("sceneName").toString();
        root["sceneType"] = sceneType;
        root["testType"] = testType;

        // 步骤列表
        QJsonArray stepsArray;
        for (const auto &step : steps) {
            if (step.optName.isEmpty()) continue;

            QJsonObject stepObj;
            stepObj["optIndex"] = step.optIndex;
            stepObj["optName"] = step.optName;
            stepObj["optType"] = step.optType;
            // 如果是场景配置步骤，使用更新后的路径
            if (step.optName == "场景配置") {
                QJsonObject updatedContent = step.optContent;
                updatedContent["场景存储路径"] = jsonPath;
                stepObj["optContent"] = updatedContent;
            } else {
                stepObj["optContent"] = step.optContent;
            }
            stepsArray.append(stepObj);
        }
        root["steps"] = stepsArray;

        // 写入文件
        QFile file(jsonPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QJsonDocument doc(root);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            SCENE_LOG_CAT("JSON_EXPORT", QString("Saved to: %1").arg(jsonPath));
        } else {
            qWarning() << "SceneService::saveDetail - Failed to save JSON:" << jsonPath;
        }
    }

    // 结束数据流转追踪
    bool success = addedCount > 0;
    CYCLE_FLOW_END(flowId, success);

    return success;
}

QVariantMap SceneService::loadDetail(const QString &sceneId, const QString &testType)
{
    if (!m_configRepo) {
        SCENE_LOG_ERROR("loadDetail", "m_configRepo is null");
        return QVariantMap();
    }

    SCENE_LOG_CAT("LOAD_START", QString("sceneId=%1, testType=%2").arg(sceneId, testType));

    // 从数据库获取步骤
    QVector<SceneStep> steps = m_configRepo->getSceneSteps(sceneId);

    SCENE_LOG_CAT("LOAD_STEPS", QString("found %1 steps in DB").arg(steps.size()));

    if (steps.isEmpty()) {
        SCENE_LOG_ERROR("LOAD", "No steps found in database");
        return QVariantMap();
    }

    // 使用 Mapper 转换数据，传入测试类型
    QVariantMap result = SceneDetailMapper::dbStepsToMap(steps, testType);

    // ★ 文件日志：记录加载结果
    QString loadedSceneType = result.value("sceneType").toString();
    SCENE_LOG_LOAD(sceneId, loadedSceneType, testType, result.keys().size());
    SCENE_LOG_CAT("LOAD_RESULT", QString("sceneType=%1, keys=%2, channels=%3")
              .arg(loadedSceneType, result.keys().join(","))
              .arg(result.value("channels").toList().size()));

    return result;
}

bool SceneService::updateTestType(const QString &sceneId, const QString &newTestType, SceneRepository *repo)
{
    if (!m_configRepo || !repo) return false;

    // 删除旧配置步骤，新步骤由用户重新配置
    m_configRepo->deleteSceneSteps(sceneId);

    return true;
}