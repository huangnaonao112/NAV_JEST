#include "Config.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QCoreApplication>
#include <QString>

Config& Config::instance() {
    static Config instance;
    return instance;
}

Config::Config() {
    // 注意：不要在构造函数中调用 SceneLogger，因为 Config 可能在 QGuiApplication 之前初始化
    loadConfig();
}

QString Config::configPath() const {
    // 优先使用 exe 目录下的 config.json
    QString exePath = QCoreApplication::applicationDirPath();
    QString configPath = QDir(exePath).filePath("config.json");

    if (QFile::exists(configPath)) {
        return configPath;
    }

    // 其次使用应用数据目录
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString altPath = QDir(dataPath).filePath("config.json");
    return altPath;
}

bool Config::loadConfig() {
    QString path = configPath();

    QFile file(path);
    if (!file.exists()) {
        m_loaded = false;
        return false;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Config: failed to open" << path;
        m_loaded = false;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Config: JSON parse error:" << error.errorString();
        m_loaded = false;
        return false;
    }

    m_config = doc.object();
    m_loaded = true;

    return true;
}

QString Config::value(const QString &key, const QString &defaultValue) const {
    QStringList parts = key.split('.');
    QJsonObject current = m_config;
    
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!current.contains(parts[i])) {
            return defaultValue;
        }
        current = current[parts[i]].toObject();
    }
    
    QString lastKey = parts.last();
    if (current.contains(lastKey)) {
        return current[lastKey].toString();
    }
    
    return defaultValue;
}

int Config::intValue(const QString &key, int defaultValue) const {
    QStringList parts = key.split('.');
    QJsonObject current = m_config;
    
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!current.contains(parts[i])) {
            return defaultValue;
        }
        current = current[parts[i]].toObject();
    }
    
    QString lastKey = parts.last();
    if (current.contains(lastKey)) {
        return current[lastKey].toInt();
    }
    
    return defaultValue;
}

// 数据库配置 - 优先级: 环境变量 > config.json > 默认值

QString Config::dbHost() const {
    QByteArray env = qgetenv("DB_HOST");
    if (!env.isEmpty()) {
        return QString::fromUtf8(env);
    }
    return value("database.host", "localhost");
}

int Config::dbPort() const {
    QByteArray env = qgetenv("DB_PORT");
    if (!env.isEmpty()) {
        return env.toInt();
    }
    return intValue("database.port", 3306);
}

QString Config::dbUser() const {
    QByteArray env = qgetenv("DB_USER");
    if (!env.isEmpty()) {
        return QString::fromUtf8(env);
    }
    return value("database.user", "root");
}

QString Config::dbPassword() const {
    QByteArray env = qgetenv("DB_PASSWORD");
    if (!env.isEmpty()) {
        return QString::fromUtf8(env);
    }
    return value("database.password", "lysk123");
}

QString Config::dbName() const {
    QByteArray env = qgetenv("DB_NAME");
    if (!env.isEmpty()) {
        return QString::fromUtf8(env);
    }
    return value("database.name", "navjest");
}

// 服务器配置

int Config::httpPort() const {
    QByteArray env = qgetenv("HTTP_PORT");
    if (!env.isEmpty()) {
        return env.toInt();
    }
    return intValue("server.http_port", 18080);
}

int Config::wsPort() const {
    QByteArray env = qgetenv("WS_PORT");
    if (!env.isEmpty()) {
        return env.toInt();
    }
    return intValue("server.ws_port", 18081);
}

// Rack 客户端配置

QString Config::rackClientHost() const {
    QByteArray env = qgetenv("RACK_CLIENT_HOST");
    if (!env.isEmpty()) {
        return QString::fromUtf8(env);
    }
    return value("rack_client.host", "127.0.0.1");
}

int Config::rackClientPort() const {
    QByteArray env = qgetenv("RACK_CLIENT_PORT");
    if (!env.isEmpty()) {
        return env.toInt();
    }
    return intValue("rack_client.port", 8000);
}

bool Config::localAccountEnabled() const {
    // 注意：enabled 在 config.json 里是 JSON 布尔值，value()->toString()
    // 对布尔返回空串（曾致本地账号永远禁用），须按 Bool 读取
    QStringList parts = QString("local_account.enabled").split('.');
    QJsonObject current = m_config;
    for (int i = 0; i < parts.size() - 1; ++i) {
        if (!current.contains(parts[i])) return true;   // 默认启用
        current = current[parts[i]].toObject();
    }
    const QString lastKey = parts.last();
    if (current.contains(lastKey)) {
        const QJsonValue v = current[lastKey];
        if (v.isBool()) return v.toBool();
        if (v.isString())
            return v.toString().compare("true", Qt::CaseInsensitive) == 0;
    }
    return true;   // 未配置 = 默认启用
}

QString Config::localAccountUsername() const {
    return value("local_account.username", "admin");
}

QString Config::localAccountPasswordHash() const {
    return value("local_account.password_hash", "");
}