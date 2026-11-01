#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QSqlDatabase>
#include "SceneConfigRepository.h"

class SceneRepository;

/**
 * 场景业务服务层
 * 负责协调 Repository 和处理业务逻辑
 */
class SceneService : public QObject
{
    Q_OBJECT

public:
    explicit SceneService(QSqlDatabase db, QObject *parent = nullptr);
    ~SceneService();

    // ========== 场景详情业务 ==========
    
    /**
     * 保存场景详情到数据库
     * @param sceneId 场景ID
     * @param detailData 前端传递的详情数据
     * @return 是否成功
     */
    bool saveDetail(const QString &sceneId, const QVariantMap &detailData);
    
    /**
     * 从数据库加载场景详情
     * @param sceneId 场景ID
     * @param testType 测试类型（从场景主表获取）
     * @return 详情数据（QVariantMap格式）
     */
    QVariantMap loadDetail(const QString &sceneId, const QString &testType = "定位精度");
    
    /**
     * 更新场景测试类型
     * @param sceneId 场景ID
     * @param newTestType 新测试类型
     * @param repo 场景仓储（用于更新主表）
     * @return 是否成功
     */
    bool updateTestType(const QString &sceneId, const QString &newTestType, SceneRepository *repo);

private:
    QSqlDatabase m_db;
    SceneConfigRepository *m_configRepo;
};