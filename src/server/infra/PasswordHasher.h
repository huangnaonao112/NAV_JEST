#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>

/**
 * @brief 密码哈希工具类
 * 
 * 使用 SHA-256 + 随机 Salt 进行密码加密
 * 哈希格式: salt$hash (Base64编码)
 */
class PasswordHasher {
public:
    /**
     * @brief 对密码进行哈希
     * @param password 明文密码
     * @return 哈希后的字符串 (格式: salt$hash)
     */
    static QString hash(const QString &password);
    
    /**
     * @brief 验证密码
     * @param password 明文密码
     * @param hashedPassword 哈希后的密码
     * @return 验证成功返回 true
     */
    static bool verify(const QString &password, const QString &hashedPassword);
    
    /**
     * @brief 检查字符串是否是有效的哈希格式
     * @param str 要检查的字符串
     * @return 是哈希格式返回 true
     */
    static bool isHashed(const QString &str);

private:
    static constexpr int SALT_LENGTH = 32;  // Salt 字节数
    
    /**
     * @brief 生成随机 Salt
     */
    static QByteArray generateSalt();
    
    /**
     * @brief 计算 SHA-256 哈希
     */
    static QByteArray sha256(const QByteArray &data);
    
    /**
     * @brief Base64 编码
     */
    static QString toBase64(const QByteArray &data);
    
    /**
     * @brief Base64 解码
     */
    static QByteArray fromBase64(const QString &str);
};