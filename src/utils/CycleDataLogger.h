#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QVariantMap>
#include <QVariantList>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

/**
 * 周期数据日志工具类
 * 记录数据库操作与QML界面交互的完整流程
 * 日志文件位置：应用程序目录下的 logs/cycle_data.log
 */
class CycleDataLogger {
public:
    static CycleDataLogger& instance() {
        static CycleDataLogger logger;
        return logger;
    }

    // ========== 数据库操作日志 ==========

    /**
     * 记录数据库保存操作
     * @param operation 操作名称 (SAVE/UPDATE/DELETE)
     * @param sceneId 场景ID
     * @param data 保存的数据
     * @param success 是否成功
     */
    void logDbSave(const QString &operation, const QString &sceneId,
                   const QVariantMap &data, bool success) {
        QString dataSummary = summarizeData(data);
        QString status = success ? "SUCCESS" : "FAILED";

        log("DB_SAVE", QString("[%1] sceneId=%2, status=%3\n    Data: %4")
            .arg(operation, sceneId, status, dataSummary));

        if (success) {
            logDataDetail(sceneId, data, "SAVED_TO_DB");
        }
    }

    /**
     * 记录数据库更新操作
     * @param sceneId 场景ID
     * @param field 更新的字段名
     * @param oldValue 旧值
     * @param newValue 新值
     */
    void logDbUpdate(const QString &sceneId, const QString &field,
                     const QString &oldValue, const QString &newValue) {
        log("DB_UPDATE", QString("sceneId=%1, field=%2\n    Old: %3\n    New: %4")
            .arg(sceneId, field, truncateValue(oldValue), truncateValue(newValue)));
    }

    /**
     * 记录数据库加载操作
     * @param sceneId 场景ID
     * @param data 加载的数据
     * @param source 数据来源 (DB/CACHE)
     */
    void logDbLoad(const QString &sceneId, const QVariantMap &data,
                   const QString &source = "DB") {
        QString dataSummary = summarizeData(data);
        log("DB_LOAD", QString("sceneId=%1, source=%2\n    Data: %3")
            .arg(sceneId, source, dataSummary));

        logDataDetail(sceneId, data, "LOADED_FROM_" + source);
    }

    // ========== QML界面交互日志 ==========

    /**
     * 记录QML请求加载数据
     * @param sceneId 场景ID
     * @param context 调用上下文 (哪个页面/组件)
     */
    void logQmlLoadRequest(const QString &sceneId, const QString &context) {
        log("QML_REQUEST", QString("sceneId=%1, context=%2, action=LOAD_DETAIL")
            .arg(sceneId, context));
    }

    /**
     * 记录QML收到数据
     * @param sceneId 场景ID
     * @param data 收到的数据
     * @param bindTarget 绑定目标属性
     */
    void logQmlDataReceived(const QString &sceneId, const QVariantMap &data,
                            const QString &bindTarget = "") {
        QString dataSummary = summarizeData(data);
        log("QML_RECEIVED", QString("sceneId=%1, bindTarget=%2\n    Data: %3")
            .arg(sceneId, bindTarget, dataSummary));
    }

    /**
     * 记录QML界面刷新状态
     * @param component 组件名称
     * @param property 属性名
     * @param value 当前值
     * @param refreshType 刷新类型 (INIT/UPDATE/BIND)
     */
    void logQmlRefresh(const QString &component, const QString &property,
                       const QString &value, const QString &refreshType) {
        log("QML_REFRESH", QString("component=%1, property=%2, type=%3\n    Value: %4")
            .arg(component, property, refreshType, truncateValue(value)));
    }

    /**
     * 记录QML界面完整状态快照
     * @param sceneId 场景ID
     * @param displayData 当前显示的数据
     */
    void logQmlDisplaySnapshot(const QString &sceneId, const QVariantMap &displayData) {
        QString dataDetail = variantMapToJson(displayData);
        log("QML_DISPLAY", QString("sceneId=%1\n    Display Content:\n%2")
            .arg(sceneId, indentJson(dataDetail)));
    }

    /**
     * 记录QML属性绑定状态
     * @param property 属性名
     * @param expectedValue 期望值
     * @param actualValue 实际值
     * @param matched 是否匹配
     */
    void logQmlBindingCheck(const QString &property,
                            const QString &expectedValue,
                            const QString &actualValue,
                            bool matched) {
        QString status = matched ? "MATCHED" : "MISMATCH";
        log("QML_BINDING", QString("property=%1, status=%2\n    Expected: %3\n    Actual: %4")
            .arg(property, status, truncateValue(expectedValue), truncateValue(actualValue)));
    }

    // ========== 数据流转追踪 ==========

    /**
     * 记录数据流转节点
     * @param flowId 流程ID
     * @param node 节点名称 (DB->SERVICE->MODEL->QML)
     * @param data 数据内容
     */
    void logDataFlow(const QString &flowId, const QString &node, const QVariantMap &data) {
        QString dataSummary = summarizeData(data);
        log("DATA_FLOW", QString("flowId=%1, node=%2\n    Data: %3")
            .arg(flowId, node, dataSummary));
    }

    /**
     * 开始一个数据流转追踪
     * @param sceneId 场景ID
     * @param operation 操作类型 (LOAD/SAVE)
     * @return 流程ID
     */
    QString beginDataFlow(const QString &sceneId, const QString &operation) {
        QString flowId = QString("%1_%2_%3")
            .arg(sceneId, operation)
            .arg(QDateTime::currentDateTime().toString("hhmmsszzz"));

        log("FLOW_START", QString("flowId=%1, sceneId=%2, operation=%3")
            .arg(flowId, sceneId, operation));

        return flowId;
    }

    /**
     * 结束数据流转追踪
     * @param flowId 流程ID
     * @param success 是否成功
     */
    void endDataFlow(const QString &flowId, bool success) {
        QString status = success ? "SUCCESS" : "FAILED";
        log("FLOW_END", QString("flowId=%1, status=%2").arg(flowId, status));
    }

    // ========== 错误日志 ==========

    void logError(const QString &operation, const QString &error,
                  const QString &context = "") {
        log("ERROR", QString("operation=%1, context=%2\n    Error: %3")
            .arg(operation, context, error));
    }

    void logWarning(const QString &operation, const QString &warning,
                    const QString &context = "") {
        log("WARNING", QString("operation=%1, context=%2\n    Warning: %3")
            .arg(operation, context, warning));
    }

    // ========== 工具方法 ==========

    QString logFilePath() const {
        return m_logFilePath;
    }

    /**
     * 记录自定义消息
     */
    void log(const QString &category, const QString &message) {
        QMutexLocker locker(&m_mutex);

        if (!ensureFileOpen()) {
            return;
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString logLine = QString("[%1] [%2] %3\n").arg(timestamp, category, message);

        m_stream << logLine;
        m_stream.flush();
    }

private:
    CycleDataLogger() {
        QString appDir = QCoreApplication::applicationDirPath();
        QString logDir = appDir + "/logs";
        QDir dir(logDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        m_logFilePath = logDir + "/cycle_data.log";
    }

    ~CycleDataLogger() {
        if (m_file.isOpen()) {
            m_file.close();
        }
    }

    bool ensureFileOpen() {
        if (!m_file.isOpen()) {
            m_file.setFileName(m_logFilePath);
            if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                m_stream.setDevice(&m_file);
                m_stream << "\n========== Application Started: "
                         << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                         << " ==========\n";
            } else {
                return false;
            }
        }
        return true;
    }

    /**
     * 数据摘要：提取关键字段
     */
    QString summarizeData(const QVariantMap &data) {
        QStringList summary;

        // 关键字段列表
        QStringList keyFields = {"sceneId", "sceneType", "testType", "scenePath",
                                  "freqSelect", "trackOutput", "channels"};

        for (const QString &key : keyFields) {
            if (data.contains(key)) {
                QVariant value = data.value(key);
                if (value.typeId() == QMetaType::QVariantList) {
                    summary << QString("%1=[%2 items]").arg(key).arg(value.toList().size());
                } else {
                    QString valStr = value.toString();
                    if (valStr.length() > 50) {
                        valStr = valStr.left(50) + "...";
                    }
                    summary << QString("%1=%2").arg(key, valStr);
                }
            }
        }

        if (summary.isEmpty()) {
            return QString("(empty, keys: %1)").arg(data.keys().join(","));
        }

        return summary.join(", ");
    }

    /**
     * 记录详细数据内容
     */
    void logDataDetail(const QString &sceneId, const QVariantMap &data,
                       const QString &operation) {
        QString jsonDetail = variantMapToJson(data);
        log("DATA_DETAIL", QString("sceneId=%1, operation=%2\n%3")
            .arg(sceneId, operation, indentJson(jsonDetail)));
    }

    /**
     * QVariantMap 转 JSON 字符串
     */
    QString variantMapToJson(const QVariantMap &data) {
        QJsonObject obj = QJsonObject::fromVariantMap(data);

        // 处理 channels 数组，简化输出
        if (obj.contains("channels")) {
            QJsonArray channels = obj["channels"].toArray();
            if (channels.size() > 3) {
                QJsonArray simplified;
                for (int i = 0; i < qMin(3, channels.size()); ++i) {
                    simplified.append(channels[i]);
                }
                obj["channels"] = QString("[%1 channels, showing first 3]")
                    .arg(channels.size());
                // 添加简化后的前3个通道
                obj["channels_preview"] = simplified;
            }
        }

        QJsonDocument doc(obj);
        return doc.toJson(QJsonDocument::Indented);
    }

    /**
     * 缩进 JSON 字符串
     */
    QString indentJson(const QString &json) {
        QStringList lines = json.split('\n');
        for (QString &line : lines) {
            line = "    " + line;
        }
        return lines.join('\n');
    }

    /**
     * 截断过长的值
     */
    QString truncateValue(const QString &value, int maxLen = 100) {
        if (value.length() > maxLen) {
            return value.left(maxLen) + "...";
        }
        return value;
    }

    QFile m_file;
    QTextStream m_stream;
    QString m_logFilePath;
    QMutex m_mutex;
};

// ========== 便捷宏 ==========

// 数据库操作
#define CYCLE_LOG_DB_SAVE(sceneId, data, success) \
    CycleDataLogger::instance().logDbSave("SAVE", sceneId, data, success)

#define CYCLE_LOG_DB_UPDATE(sceneId, field, oldVal, newVal) \
    CycleDataLogger::instance().logDbUpdate(sceneId, field, oldVal, newVal)

#define CYCLE_LOG_DB_LOAD(sceneId, data, source) \
    CycleDataLogger::instance().logDbLoad(sceneId, data, source)

// QML界面
#define CYCLE_LOG_QML_REQUEST(sceneId, context) \
    CycleDataLogger::instance().logQmlLoadRequest(sceneId, context)

#define CYCLE_LOG_QML_RECEIVED(sceneId, data, target) \
    CycleDataLogger::instance().logQmlDataReceived(sceneId, data, target)

#define CYCLE_LOG_QML_REFRESH(component, property, value, type) \
    CycleDataLogger::instance().logQmlRefresh(component, property, value, type)

#define CYCLE_LOG_QML_DISPLAY(sceneId, data) \
    CycleDataLogger::instance().logQmlDisplaySnapshot(sceneId, data)

#define CYCLE_LOG_QML_BINDING(property, expected, actual, matched) \
    CycleDataLogger::instance().logQmlBindingCheck(property, expected, actual, matched)

// 数据流转
#define CYCLE_FLOW_BEGIN(sceneId, op) \
    CycleDataLogger::instance().beginDataFlow(sceneId, op)

#define CYCLE_FLOW_NODE(flowId, node, data) \
    CycleDataLogger::instance().logDataFlow(flowId, node, data)

#define CYCLE_FLOW_END(flowId, success) \
    CycleDataLogger::instance().endDataFlow(flowId, success)

// 错误/警告
#define CYCLE_LOG_ERROR(op, err, ctx) \
    CycleDataLogger::instance().logError(op, err, ctx)

#define CYCLE_LOG_WARNING(op, warn, ctx) \
    CycleDataLogger::instance().logWarning(op, warn, ctx)

// 通用日志
#define CYCLE_LOG(cat, msg) \
    CycleDataLogger::instance().log(cat, msg)
