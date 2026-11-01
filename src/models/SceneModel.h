#pragma once

#include <QAbstractListModel>
#include <QtQml/qqml.h>
#include <QDateTime>
#include <QString>
#include <QVector>
#include <QVariantMap>
#include <QMap>

class Db;
class SceneRepository;
class SceneService;

/**
 * 场景列表模型
 * 纯 QAbstractListModel 职责，负责向 QML 提供数据
 * 业务逻辑委托给 SceneService 处理
 */
class SceneModel : public QAbstractListModel {
    QML_SINGLETON
    QML_ELEMENT
    Q_OBJECT
public:
    enum Roles {
        RowNumberRole = Qt::UserRole + 1,
        SceneIdRole,
        SceneNameRole,
        SceneTypeRole,
        FilePathRole,
        AccessDeviceRole,
        TestTypeRole,
        DescriptionRole,
        StatusRole,
        UpdatedAtRole
    };

    explicit SceneModel(QObject *parent = nullptr);
    ~SceneModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    
    Q_INVOKABLE bool add(const QString &sceneId,
                         const QString &sceneName,
                         const QString &sceneType,
                         const QString &accessDevice,
                         const QString &testType,
                         const QString &description);

    Q_INVOKABLE bool update(int index,
                            const QString &sceneId,
                            const QString &sceneName,
                            const QString &sceneType,
                            const QString &accessDevice,
                            const QString &testType,
                            const QString &description);

    Q_INVOKABLE bool removeAt(int index);

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool save() const;

    // 场景详情保存/加载
    Q_INVOKABLE bool saveDetail(const QString &sceneId, const QVariantMap &detailData);
    Q_INVOKABLE QVariantMap loadDetail(const QString &sceneId);
    
    // 更新场景测试类型
    Q_INVOKABLE bool updateTestType(const QString &sceneId, const QString &newTestType);

    // QML界面日志接口
    Q_INVOKABLE void logQmlRefresh(const QString &component, const QString &property,
                                    const QString &value, const QString &refreshType);
    Q_INVOKABLE void logQmlDisplaySnapshot(const QString &sceneId, const QVariantMap &displayData);
    Q_INVOKABLE void logQmlBindingCheck(const QString &property,
                                         const QString &expectedValue,
                                         const QString &actualValue,
                                         bool matched);
    Q_INVOKABLE void logQmlMessage(const QString &category, const QString &message);

    // 导出场景配置到JSON文件
    Q_INVOKABLE bool exportToJson(const QString &filePath, const QVariantMap &detailData);

    // 从JSON文件导入场景配置
    Q_INVOKABLE QVariantMap importFromJson(const QString &filePath);

// 工具方法
    Q_INVOKABLE QStringList sceneNameList() const;
    Q_INVOKABLE QStringList deviceTypeList() const;
    Q_INVOKABLE QString diagnoseDatabase() const;

private:
    struct Scene {
        int processId;
        QString sceneId;
        QString sceneName;      // PROCESS_SCENE_NAME - 场景名称
        QString sceneType;      // PROCESS_SCENE_TYPE - 场景类型
        QString testName;       // PROCESS_NAME - 测试类型
        QString filePath;
        QString accessDevice;
        QString description;
        int status;
        QDateTime createdAt;
        QDateTime updatedAt;
    };

    int findBySceneId(const QString &sceneId) const;

    QVector<Scene> m_scenes;
    int m_nextRowNumber;

    // 缓存所有场景详情，key 为 sceneId
    QMap<QString, QVariantMap> m_details;

    Db *m_db;
    SceneRepository *m_repo;
    SceneService *m_service;
};
