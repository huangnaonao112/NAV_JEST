#pragma once

#include <QString>
#include <QJsonObject>

/**
 * @brief 应用配置管理类
 * 
 * 配置优先级: 环境变量 > config.json > 默认值
 */
class Config {
public:
    static Config& instance();
    
    // 数据库配置
    QString dbHost() const;
    int dbPort() const;
    QString dbUser() const;
    QString dbPassword() const;
    QString dbName() const;
    
    // 服务器配置
    int httpPort() const;
    int wsPort() const;
    
    // Rack 客户端配置
    QString rackClientHost() const;
    int rackClientPort() const;

    // 本地应急账号（与数据库无关；DB 掉线/迁移时的登录兜底）
    bool localAccountEnabled() const;
    QString localAccountUsername() const;
    QString localAccountPasswordHash() const;
    
    // 配置文件路径
    QString configPath() const;
    bool loadConfig();
    
private:
    Config();
    ~Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    
    QString value(const QString &key, const QString &defaultValue) const;
    int intValue(const QString &key, int defaultValue) const;
    
    QJsonObject m_config;
    bool m_loaded = false;
};