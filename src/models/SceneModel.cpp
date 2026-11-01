#include "SceneModel.h"
#include "utils/SceneLogger.h"
#include "utils/CycleDataLogger.h"

#include "server/infra/Db.h"
#include "server/domain/scene/SceneRepository.h"
#include "server/domain/scene/SceneService.h"
#include "server/domain/scene/SceneDetailMapper.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>

SceneModel::SceneModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_nextRowNumber(1)
    , m_db(new Db())
    , m_repo(nullptr)
    , m_service(nullptr)
{
    m_db->open();
    m_repo = new SceneRepository(m_db->database());
    m_repo->init();
    m_service = new SceneService(m_db->database());
}

SceneModel::~SceneModel()
{
    delete m_service;
    m_service = nullptr;
    delete m_repo;
    m_repo = nullptr;
    delete m_db;
    m_db = nullptr;
}

int SceneModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_scenes.size();
}

QVariant SceneModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return {};
    const int row = index.row();
    if (row < 0 || row >= m_scenes.size()) return {};

    const Scene &s = m_scenes[row];
    switch (role) {
    case RowNumberRole: return s.processId;
    case SceneIdRole: return s.sceneId;
    case SceneNameRole: return s.sceneName;
    case SceneTypeRole: return s.sceneType;
    case FilePathRole: return s.filePath;
    case AccessDeviceRole: return s.accessDevice;
    case TestTypeRole: return s.testName;
    case DescriptionRole: return s.description;
    case StatusRole: return s.status;
    case UpdatedAtRole: return s.updatedAt.toString("yyyy/MM/dd hh:mm:ss");
    default:
        return {};
    }
}

QHash<int, QByteArray> SceneModel::roleNames() const {
    return {
        {RowNumberRole, "rowNumber"},
        {SceneIdRole, "sceneId"},
        {SceneNameRole, "sceneName"},
        {SceneTypeRole, "sceneType"},
        {FilePathRole, "filePath"},
        {AccessDeviceRole, "accessDevice"},
        {TestTypeRole, "testType"},
        {DescriptionRole, "description"},
        {StatusRole, "status"},
        {UpdatedAtRole, "updatedAt"}
    };
}

QVariantMap SceneModel::get(int index) const {
    QVariantMap m;
    if (index < 0 || index >= m_scenes.size()) return m;
    const Scene &s = m_scenes[index];
    m["rowNumber"] = s.processId;
    m["sceneId"] = s.sceneId;
    m["sceneName"] = s.sceneName;
    m["sceneType"] = s.sceneType;
    m["filePath"] = s.filePath;
    m["accessDevice"] = s.accessDevice;
    m["testType"] = s.testName;
    m["description"] = s.description;
    m["status"] = s.status;
    m["updatedAt"] = s.updatedAt.toString("yyyy/MM/dd hh:mm:ss");
    return m;
}

int SceneModel::findBySceneId(const QString &sceneId) const {
    for (int i = 0; i < m_scenes.size(); ++i) {
        if (m_scenes[i].sceneId.compare(sceneId, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

bool SceneModel::add(const QString &sceneId,
                     const QString &sceneName,
                     const QString &sceneType,
                     const QString &accessDevice,
                     const QString &testType,
                     const QString &description)
{
    // 验证所有字段不能为空
    if (sceneId.trimmed().isEmpty()) {
        qWarning() << "SceneModel::add - 场景编号为空";
        return false;
    }
    if (sceneName.trimmed().isEmpty()) {
        qWarning() << "SceneModel::add - 场景名称为空";
        return false;
    }
    if (sceneType.trimmed().isEmpty()) {
        qWarning() << "SceneModel::add - 场景类型为空";
        return false;
    }
    if (accessDevice.trimmed().isEmpty()) {
        qWarning() << "SceneModel::add - 接入设备为空";
        return false;
    }
    if (testType.trimmed().isEmpty()) {
        qWarning() << "SceneModel::add - 测试类型为空";
        return false;
    }
    if (description.trimmed().isEmpty()) {
        qWarning() << "SceneModel::add - 场景描述为空";
        return false;
    }

    // 检查场景编号是否已存在
    if (findBySceneId(sceneId) != -1) {
        qWarning() << "SceneModel::add - 场景编号已存在:" << sceneId;
        return false;
    }

    Scene s;
    s.processId = m_nextRowNumber++;
    s.sceneId = sceneId.trimmed();
    s.sceneName = sceneName.trimmed();
    s.sceneType = sceneType.trimmed();
    s.accessDevice = accessDevice.trimmed();
    s.testName = testType.trimmed();
    s.description = description.trimmed();
    s.status = 1;
    s.updatedAt = QDateTime::currentDateTime();

    if (m_repo) {
        SceneRepository::Scene rs;
        rs.processId = s.processId;
        rs.sceneId = s.sceneId;
        rs.sceneName = s.sceneName;
        rs.sceneType = s.sceneType;
        rs.accessDevice = s.accessDevice;
        rs.testName = s.testName;
        rs.description = s.description;
        rs.status = s.status;
        rs.updatedAt = s.updatedAt;
        if (!m_repo->insert(rs)) {
            return false;
        }
    }

    beginInsertRows(QModelIndex(), m_scenes.size(), m_scenes.size());
    m_scenes.push_back(s);
    endInsertRows();

    return true;
}

bool SceneModel::update(int index,
                        const QString &sceneId,
                        const QString &sceneName,
                        const QString &sceneType,
                        const QString &accessDevice,
                        const QString &testType,
                        const QString &description)
{
    if (index < 0 || index >= m_scenes.size()) {
        qWarning() << "SceneModel::update - 无效的索引:" << index;
        return false;
    }

    // 验证所有字段不能为空
    const QString normalizedId = sceneId.trimmed();
    if (normalizedId.isEmpty()) {
        qWarning() << "SceneModel::update - 场景编号为空";
        return false;
    }
    if (sceneName.trimmed().isEmpty()) {
        qWarning() << "SceneModel::update - 场景名称为空";
        return false;
    }
    if (sceneType.trimmed().isEmpty()) {
        qWarning() << "SceneModel::update - 场景类型为空";
        return false;
    }
    if (accessDevice.trimmed().isEmpty()) {
        qWarning() << "SceneModel::update - 接入设备为空";
        return false;
    }
    if (testType.trimmed().isEmpty()) {
        qWarning() << "SceneModel::update - 测试类型为空";
        return false;
    }
    if (description.trimmed().isEmpty()) {
        qWarning() << "SceneModel::update - 场景描述为空";
        return false;
    }

    // 检查场景编号是否与其他场景冲突
    const int existing = findBySceneId(normalizedId);
    if (existing != -1 && existing != index) {
        qWarning() << "SceneModel::update - 场景编号已被其他场景使用:" << normalizedId;
        return false;
    }

    Scene &s = m_scenes[index];
    const QString oldId = s.sceneId;
    const QString oldTestType = s.testName;  // ★ 记录旧的测试类型
    s.sceneId = normalizedId;
    s.sceneName = sceneName.trimmed();
    s.sceneType = sceneType.trimmed();
    s.accessDevice = accessDevice.trimmed();
    s.testName = testType.trimmed();
    s.description = description.trimmed();
    s.updatedAt = QDateTime::currentDateTime();

    if (m_repo) {
        SceneRepository::Scene rs;
        rs.processId = s.processId;
        rs.sceneId = s.sceneId;
        rs.sceneName = s.sceneName;
        rs.sceneType = s.sceneType;
        rs.accessDevice = s.accessDevice;
        rs.testName = s.testName;
        rs.description = s.description;
        rs.status = s.status;
        rs.updatedAt = s.updatedAt;
        if (!m_repo->update(oldId, rs)) {
            return false;
        }
    }

    // ★ 测试类型变更时，清理 TB_ProcessDetail 旧步骤
    if (s.testName != oldTestType && m_service) {
        m_service->updateTestType(s.sceneId, s.testName, m_repo);
    }

    const QModelIndex idx = createIndex(index, 0);
    emit dataChanged(idx, idx, {SceneIdRole, SceneNameRole, SceneTypeRole, AccessDeviceRole, TestTypeRole, DescriptionRole, StatusRole, UpdatedAtRole});

    return true;
}

bool SceneModel::removeAt(int index) {
    if (index < 0 || index >= m_scenes.size()) return false;

    const QString id = m_scenes[index].sceneId;
    
    if (m_repo) {
        if (!m_repo->remove(id)) {
            return false;
        }
    }
    
    beginRemoveRows(QModelIndex(), index, index);
    m_scenes.removeAt(index);
    endRemoveRows();
    
    m_nextRowNumber = m_scenes.size() + 1;
    
    return true;
}

bool SceneModel::load() {
    if (!m_repo) return false;

    auto rows = m_repo->list();
    // 不再自动插入默认数据，完全依赖数据库

    beginResetModel();
    m_scenes.clear();
    m_scenes.reserve(rows.size());
    m_nextRowNumber = 1;
    for (const auto &rs : rows) {
        Scene s;
        s.processId = rs.processId;
        s.sceneId = rs.sceneId;
        s.sceneName = rs.sceneName;
        s.sceneType = rs.sceneType;
        s.testName = rs.testName;
        s.filePath = rs.filePath;
        s.accessDevice = rs.accessDevice;
        s.description = rs.description;
        s.status = rs.status;
        s.createdAt = rs.createdAt;
        s.updatedAt = rs.updatedAt;
        if (s.processId >= m_nextRowNumber) {
            m_nextRowNumber = s.processId + 1;
        }
        if (s.sceneId.trimmed().isEmpty()) continue;
        m_scenes.push_back(s);
    }
    endResetModel();

    // 不再预加载，改为按需加载，避免缓存混乱
    m_details.clear();

    return true;
}

bool SceneModel::save() const {
    return true;
}

bool SceneModel::saveDetail(const QString &sceneId, const QVariantMap &detailData) {
    if (!m_service) {
        CYCLE_LOG_ERROR("saveDetail", "m_service is null", sceneId);
        return false;
    }

    // 开始数据流转追踪
    QString flowId = CYCLE_FLOW_BEGIN(sceneId, "SAVE");

    CYCLE_FLOW_NODE(flowId, "MODEL_INPUT", detailData);

    bool success = m_service->saveDetail(sceneId, detailData);

    CYCLE_LOG_DB_SAVE(sceneId, detailData, success);
    CYCLE_FLOW_END(flowId, success);

    return success;
}

QVariantMap SceneModel::loadDetail(const QString &sceneId) {
    // 始终从数据库加载，确保数据最新
    if (!m_service) {
        SCENE_LOG_ERROR("MODEL_LOAD", "m_service is null");
        CYCLE_LOG_ERROR("loadDetail", "m_service is null", sceneId);
        return QVariantMap();
    }

    // 开始数据流转追踪
    QString flowId = CYCLE_FLOW_BEGIN(sceneId, "LOAD");

    // 从场景列表获取测试类型
    QString testType = "定位精度";
    int idx = findBySceneId(sceneId);
    if (idx >= 0) {
        testType = m_scenes[idx].testName;
    }

    SCENE_LOG_CAT("MODEL_LOAD_START", QString("sceneId=%1, testType=%2, idx=%3").arg(sceneId, testType).arg(idx));
    CYCLE_LOG_QML_REQUEST(sceneId, "SceneListView");

    QVariantMap result = m_service->loadDetail(sceneId, testType);

    // 记录数据流转节点
    CYCLE_FLOW_NODE(flowId, "SERVICE_RESULT", result);

    // 记录加载结果
    QString loadedSceneType = result.value("sceneType").toString();
    SCENE_LOG_CAT("MODEL_LOAD_RESULT", QString("sceneType=%1, keysCount=%2, keys=%3")
              .arg(loadedSceneType).arg(result.keys().size()).arg(result.keys().join(",")));

    // 记录数据库加载和QML接收
    CYCLE_LOG_DB_LOAD(sceneId, result, "DB");
    CYCLE_LOG_QML_RECEIVED(sceneId, result, "SceneDetailPage");

    // 结束数据流转追踪
    CYCLE_FLOW_END(flowId, true);

    return result;
}

bool SceneModel::updateTestType(const QString &sceneId, const QString &newTestType) {
    if (!m_repo) return false;

    int idx = findBySceneId(sceneId);
    if (idx < 0) return false;

    Scene &s = m_scenes[idx];
    QString oldTestType = s.testName;
    s.testName = newTestType;
    s.updatedAt = QDateTime::currentDateTime();

    SceneRepository::Scene rs;
    rs.processId = s.processId;
    rs.sceneId = s.sceneId;
    rs.sceneName = s.sceneName;
    rs.sceneType = s.sceneType;
    rs.accessDevice = s.accessDevice;
    rs.testName = s.testName;
    rs.description = s.description;
    rs.status = s.status;
    rs.updatedAt = s.updatedAt;

    if (!m_repo->update(sceneId, rs)) {
        s.testName = oldTestType;
        return false;
    }

    if (m_service) {
        m_service->updateTestType(sceneId, newTestType, m_repo);
    }

    const QModelIndex modelIdx = createIndex(idx, 0);
    emit dataChanged(modelIdx, modelIdx, {TestTypeRole, UpdatedAtRole});

    return true;
}

QStringList SceneModel::sceneNameList() const {
    QStringList list;
    for (const Scene &s : m_scenes) {
        if (!list.contains(s.sceneName)) {
            list.append(s.sceneName);
        }
    }
    return list;
}

QStringList SceneModel::deviceTypeList() const {
    return {
        QStringLiteral("地月空间载具"),
        QStringLiteral("无人机载设备"),
        QStringLiteral("月球车载设备"),
        QStringLiteral("地面站设备")
    };
}

QString SceneModel::diagnoseDatabase() const {
    QString result;
    QTextStream stream(&result);
    
    stream << "=== 数据库诊断 ===\n";
    
    if (!m_db || !m_db->database().isOpen()) {
        stream << "错误: 数据库未连接!\n";
        return result;
    }
    stream << "数据库已连接\n";
    
    QSqlQuery query(m_db->database());
    
    if (query.exec("SHOW TABLES LIKE 'tb_processdetail'")) {
        if (query.next()) {
            stream << "tb_processdetail表存在\n";
        } else {
            stream << "错误: tb_processdetail表不存在!\n";
            return result;
        }
    }
    
    if (query.exec("SELECT COUNT(*) FROM tb_processdetail")) {
        if (query.next()) {
            int count = query.value(0).toInt();
            stream << "\ntb_processdetail表中有" << count << "条记录\n";
        }
    }

    return result;
}

// ========== QML界面日志接口实现 ==========

void SceneModel::logQmlRefresh(const QString &component, const QString &property,
                                const QString &value, const QString &refreshType) {
    CYCLE_LOG_QML_REFRESH(component, property, value, refreshType);
}

void SceneModel::logQmlDisplaySnapshot(const QString &sceneId, const QVariantMap &displayData) {
    CYCLE_LOG_QML_DISPLAY(sceneId, displayData);
}

void SceneModel::logQmlBindingCheck(const QString &property,
                                     const QString &expectedValue,
                                     const QString &actualValue,
                                     bool matched) {
    CYCLE_LOG_QML_BINDING(property, expectedValue, actualValue, matched);
}

void SceneModel::logQmlMessage(const QString &category, const QString &message) {
    CYCLE_LOG(category, message);
}

bool SceneModel::exportToJson(const QString &filePath, const QVariantMap &detailData) {
    // 使用与数据库相同的格式导出：步骤列表格式
    // 通过 SceneDetailMapper::buildSteps 转换，确保格式一致

    QVector<SceneDetailMapper::StepBuildResult> steps = SceneDetailMapper::buildSteps(detailData);

    // 构建JSON
    QJsonObject root;
    root["sceneName"] = detailData.value("sceneName").toString();
    root["sceneType"] = detailData.value("sceneType").toString();
    root["testType"] = detailData.value("testType").toString();

    // 步骤列表
    QJsonArray stepsArray;
    for (const auto &step : steps) {
        QJsonObject stepObj;
        stepObj["optIndex"] = step.optIndex;
        stepObj["optName"] = step.optName;
        stepObj["optType"] = step.optType;
        stepObj["optContent"] = step.optContent;
        stepsArray.append(stepObj);
    }
    root["steps"] = stepsArray;

    // 写入文件
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "SceneModel::exportToJson - 无法打开文件:" << filePath;
        return false;
    }

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

QVariantMap SceneModel::importFromJson(const QString &filePath) {
    QVariantMap result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "SceneModel::importFromJson - 无法打开文件:" << filePath;
        return result;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "SceneModel::importFromJson - JSON解析错误:" << parseError.errorString();
        return result;
    }

    if (!doc.isObject()) {
        qWarning() << "SceneModel::importFromJson - JSON不是对象格式";
        return result;
    }

    QJsonObject root = doc.object();

    // 读取基本字段
    result["sceneName"] = root.value("sceneName").toString();
    result["sceneType"] = root.value("sceneType").toString();
    result["testType"] = root.value("testType").toString();

    // 读取步骤数组并转换为前端格式
    if (root.contains("steps") && root.value("steps").isArray()) {
        QJsonArray stepsArray = root.value("steps").toArray();

        // 转换为 SceneStep 格式
        QVector<SceneStep> steps;
        for (const QJsonValue &stepVal : stepsArray) {
            if (!stepVal.isObject()) continue;

            QJsonObject stepObj = stepVal.toObject();
            SceneStep step;
            step.optIndex = stepObj.value("optIndex").toInt(0);
            step.optName = stepObj.value("optName").toString();
            step.optType = stepObj.value("optType").toString();
            step.optContent = stepObj.value("optContent").toObject();
            steps.append(step);
        }

        // 使用 SceneDetailMapper 转换为前端格式
        QString testType = result.value("testType").toString();
        if (testType.isEmpty()) {
            testType = "定位精度";
        }

        QVariantMap detailMap = SceneDetailMapper::dbStepsToMap(steps, testType);

        // 合并基本字段和步骤数据（detailMap 优先，因为它包含转换后的完整数据）
        detailMap["sceneName"] = result.value("sceneName").toString();
        detailMap["sceneType"] = result.value("sceneType").toString();
        detailMap["testType"] = result.value("testType").toString();
        result = detailMap;
    } else {
        // 兼容旧格式：直接复制所有字段
        for (auto it = root.begin(); it != root.end(); ++it) {
            result[it.key()] = it.value().toVariant();
        }
    }

    return result;
}