#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QCoreApplication>
#include <QtQml/qqml.h>

/**
 * 文件日志工具类
 * 将关键操作日志写入文件，方便排查问题
 * 日志文件位置：应用程序目录下的 logs/scene_debug.log
 */
class SceneLogger : public QObject {
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    static SceneLogger& instance() {
        static SceneLogger logger;
        return logger;
    }

    Q_INVOKABLE void logQml(const QString &category, const QString &message) {
        log(category, message);
    }

    void log(const QString &category, const QString &message, bool consoleOutput = false) {
        QMutexLocker locker(&m_mutex);

        if (!ensureFileOpen()) {
            return;
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString logLine = QString("[%1] [%2] %3\n").arg(timestamp, category, message);

        m_stream << logLine;
        m_stream.flush();

        // 默认不输出到控制台，避免控制台日志过多
        // 只有明确指定 consoleOutput=true 时才输出到控制台
        if (consoleOutput) {
            qDebug() << "[" << category << "]" << message;
        }
    }

    void logSave(const QString &sceneId, const QString &sceneType, const QString &testType, int stepCount) {
        log("SAVE", QString("sceneId=%1, sceneType=%2, testType=%3, steps=%4")
            .arg(sceneId, sceneType, testType).arg(stepCount));
    }

    void logLoad(const QString &sceneId, const QString &sceneType, const QString &testType, int fieldCount) {
        log("LOAD", QString("sceneId=%1, sceneType=%2, testType=%3, fields=%4")
            .arg(sceneId, sceneType, testType).arg(fieldCount));
    }

    void logError(const QString &operation, const QString &error) {
        log("ERROR", QString("%1: %2").arg(operation, error));
    }

    void logDb(const QString &sql, const QString &params = "") {
        log("DB", QString("SQL: %1 | Params: %2").arg(sql.left(100), params.left(200)));
    }

    QString logFilePath() const {
        return m_logFilePath;
    }

public:
    explicit SceneLogger(QObject *parent = nullptr) : QObject(parent) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString logDir = appDir + "/logs";
        QDir dir(logDir);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        m_logFilePath = logDir + "/scene_debug.log";
    }

    ~SceneLogger() {
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

    QFile m_file;
    QTextStream m_stream;
    QString m_logFilePath;
    QMutex m_mutex;
};

// 便捷宏
#define SCENE_LOG_CAT(cat, msg) SceneLogger::instance().log(cat, msg)
#define SCENE_LOG(msg) SceneLogger::instance().log("INFO", msg)
#define SCENE_LOG_SAVE(id, type, ttype, cnt) SceneLogger::instance().logSave(id, type, ttype, cnt)
#define SCENE_LOG_LOAD(id, type, ttype, cnt) SceneLogger::instance().logLoad(id, type, ttype, cnt)
#define SCENE_LOG_ERROR(op, err) SceneLogger::instance().logError(op, err)
#define SCENE_LOG_DB(sql, params) SceneLogger::instance().logDb(sql, params)
