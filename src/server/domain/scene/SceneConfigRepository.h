#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>

/**
 * 场景步骤数据模型
 * 对应 TB_PROCESSDETAIL 表结构
 * 字段顺序：OPT_INDEX, Process_ID, OPT_CONTENT, OPT_NAME, OPT_TYPE
 */
struct SceneStep {
    int optIndex;           // OPT_INDEX - 配置序号（每个场景从0开始递增）
    int processId;          // Process_ID - 场景ID（外键关联 TB_Process.Process_ID）
    QJsonObject optContent; // OPT_CONTENT - JSON配置内容
    QString optName;        // OPT_NAME - 配置名称
    QString optType;        // OPT_TYPE - 配置大类
    QDateTime createdAt;
    QDateTime updatedAt;
};

/**
 * 场景配置仓储层
 * 基于 TB_Process + TB_PROCESSDETAIL 表结构设计
 */
class SceneConfigRepository : public QObject
{
    Q_OBJECT

public:
    explicit SceneConfigRepository(QSqlDatabase db, QObject *parent = nullptr);
    ~SceneConfigRepository();

    // ========== 场景主表操作 (TB_Process 表) ==========

    // 创建场景
    bool createScene(const QString& sceneId, const QString& sceneName,
                     const QString& sceneType, const QString& testName = "",
                     const QString& description = "");

    // 获取场景基本信息
    struct SceneInfo {
        int processId;           // PROCESS_ID - 序号
        QString sceneName;       // PROCESS_SCENE_NAME - 场景名称
        QString sceneType;       // PROCESS_SCENE_TYPE - 场景类型
        QString testName;        // PROCESS_NAME - 测试类型
        QString sceneId;         // scene_id - 场景唯一标识
        QString description;
        int status;
        QDateTime createdAt;
        QDateTime updatedAt;
    };
    SceneInfo getSceneInfo(const QString& sceneId);

    // 更新场景信息
    bool updateScene(const QString& sceneId, const QString& field, const QVariant& value);

    // 删除场景（会级联删除步骤）
    bool deleteScene(const QString& sceneId);

    // 获取所有场景列表
    QVector<SceneInfo> listAllScenes();

    // 按场景类型查询场景
    QVector<SceneInfo> listScenesBySceneType(const QString& sceneType);

    // 按流程集合查询场景（向后兼容）
    QVector<SceneInfo> listScenesByProcessSet(const QString& processSetName);

    // ========== 场景步骤操作 (TB_PROCESSDETAIL 表) ==========

    // 添加步骤
    bool addStep(const QString& sceneId, int stepIndex, const QJsonObject& stepConfig,
                 const QString& stepName, const QString& stepType);

    // 批量添加步骤
    bool addSteps(const QString& sceneId, const QVector<SceneStep>& steps);

    // 获取场景的所有步骤（按序号排序）
    QVector<SceneStep> getSceneSteps(const QString& sceneId);

    // 获取特定步骤
    SceneStep getStep(const QString& sceneId, int stepIndex);

    // 更新步骤配置
    bool updateStepConfig(const QString& sceneId, int stepIndex, const QJsonObject& stepConfig);

    // 更新步骤的特定 JSON 字段
    bool updateStepJsonField(const QString& sceneId, int stepIndex,
                             const QString& jsonPath, const QVariant& value);

    // 删除步骤
    bool deleteStep(const QString& sceneId, int stepIndex);

    // 删除场景的所有步骤
    bool deleteSceneSteps(const QString& sceneId);

    // ========== JSON 查询操作 ==========

    // 获取 JSON 中的特定字段值
    QVariant getJsonFieldValue(const QString& sceneId, int stepIndex, const QString& fieldPath);

    // 搜索包含特定字段的步骤
    QVector<SceneStep> searchStepsByJsonField(const QString& fieldPath, const QVariant& value);

    // 搜索包含特定键的步骤
    QVector<SceneStep> searchStepsByJsonKey(const QString& jsonKey);

    // ========== 批量操作 ==========

    // 复制场景
    bool cloneScene(const QString& sourceSceneId, const QString& newSceneId);

    // 导入场景配置
    bool importSceneConfig(const QString& sceneId, const QJsonArray& stepsConfig);

    // 导出场景配置为 JSON
    QJsonArray exportSceneConfig(const QString& sceneId);

    // ========== 事务支持 ==========

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

signals:
    void sceneCreated(const QString& sceneId);
    void sceneUpdated(const QString& sceneId);
    void sceneDeleted(const QString& sceneId);
    void stepAdded(const QString& sceneId, int stepIndex);
    void stepUpdated(const QString& sceneId, int stepIndex);
    void stepDeleted(const QString& sceneId, int stepIndex);
    void errorOccurred(const QString& error);

private:
    QSqlDatabase m_db;
    bool m_inTransaction = false;

    // 辅助方法
    int getProcessIdBySceneId(const QString& sceneId);
    SceneStep parseStepFromQuery(const QSqlQuery& query);
    bool stepExists(const QString& sceneId, int stepIndex);
};
