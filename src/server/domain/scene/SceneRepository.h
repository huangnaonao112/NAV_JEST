#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

class SceneRepository {
public:
    struct Scene {
        int processId;           // PROCESS_ID - 序号
        QString sceneName;       // PROCESS_SCENE_NAME - 场景名称
        QString sceneType;       // PROCESS_SCENE_TYPE - 场景类型（中高轨、低轨、地月空间、安全认证、抗干扰）
        QString testName;        // PROCESS_NAME - 测试类型（定位、测速等）
        QString sceneId;         // scene_id - 场景唯一标识
        QString filePath;        // file_path - 场景文件路径
        QString accessDevice;    // access_device - 接入设备
        QString description;     // description - 描述
        int status;              // status - 状态
        QDateTime createdAt;
        QDateTime updatedAt;
    };

    explicit SceneRepository(QSqlDatabase db);

    bool init();

    QVector<Scene> list() const;

    bool insert(const Scene &scene);
    bool update(const QString &sceneId, const Scene &scene);
    bool remove(const QString &sceneId);
    bool exists(const QString &sceneId) const;

private:
    QSqlDatabase m_db;
};
