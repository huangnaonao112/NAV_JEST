#pragma once

#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QMap>
#include <QVector>
#include "SceneConfigRepository.h"

/**
 * 场景详情数据转换器
 * 负责 QVariantMap (前端格式) ↔ QJsonObject (数据库格式) 的转换
 */
class SceneDetailMapper
{
public:
    /**
     * 步骤构建结果 - 包含完整步骤信息
     */
    struct StepBuildResult {
        int optIndex;           // 连续索引 (0, 1, 2...)
        QJsonObject optContent; // JSON配置内容
        QString optName;        // 步骤名称
        QString optType;        // 步骤类型
    };

    // ========== 保存转换：QVariantMap -> 步骤列表 ==========

    /**
     * 将前端 QVariantMap 转换为数据库格式的配置步骤列表
     * @param detailData 前端传递的详情数据
     * @return 步骤列表（使用连续的optIndex: 0, 1, 2...）
     */
    static QVector<StepBuildResult> buildSteps(const QVariantMap &detailData);

    /**
     * 将前端 QVariantMap 转换为数据库格式的配置步骤
     * @deprecated 请使用 buildSteps() 返回连续索引的步骤列表
     * @param detailData 前端传递的详情数据
     * @return 按optIndex组织的配置内容映射（可能有间隔）
     */
    static QMap<int, QJsonObject> mapToDbSteps(const QVariantMap &detailData);

    /**
     * 获取步骤的名称和类型（用于数据库存储）
     * @deprecated 请使用 buildSteps() 直接获取名称和类型
     */
    static QString getStepName(int optIndex, const QString &testType = "定位精度", const QString &sceneType = "");
    static QString getStepType(int optIndex, const QString &testType = "定位精度", const QString &sceneType = "");
    
    // ========== 加载转换：QJsonObject -> QVariantMap ==========
    
    /**
     * 将数据库格式的步骤数据转换为前端 QVariantMap
     * @param steps 数据库中的步骤列表
     * @param testType 测试类型（从场景主表获取）
     * @return 前端使用的 QVariantMap 格式
     */
    static QVariantMap dbStepsToMap(const QVector<SceneStep> &steps, const QString &testType = "定位精度");
    
    /**
     * 判断是否为干扰场景（根据步骤内容）
     */
    static bool isJammingScene(const QVector<SceneStep> &steps);

private:
    // 通道数据转换辅助方法
    static QJsonArray channelsToDb(const QVariantList &channelList);
    static QVariantList channelsFromDb(const QJsonArray &channelArray);

    // 中高轨场景专用通道转换
    static QJsonArray channelsToDb_MediumOrbit(const QVariantList &channelList);
    static QVariantList channelsFromDb_MediumOrbit(const QJsonArray &channelArray);

    // 压制干扰源通道转换
    // 数据库存储：对象格式 {"0": {...}, "1": {...}, "2": {...}}
    // 前端使用：数组格式 [{...}, {...}, {...}]
    static QJsonObject jammingChannelsToDb(const QVariantList &channelList, const QString &signalTypeName);
    static QVariantList jammingChannelsFromDb(const QJsonObject &channelObj);
};