#include "PasswordHasher.h"

#include <QCryptographicHash>
#include <QRandomGenerator>

QString PasswordHasher::hash(const QString &password)
{
    // 生成随机 Salt
    QByteArray salt = generateSalt();
    
    // 计算 hash = SHA256(salt + password)
    QByteArray combined = salt + password.toUtf8();
    QByteArray hashValue = sha256(combined);
    
    // 返回格式: salt$hash (Base64编码)
    return toBase64(salt) + "$" + toBase64(hashValue);
}

bool PasswordHasher::verify(const QString &password, const QString &hashedPassword)
{
    // 解析哈希格式
    QStringList parts = hashedPassword.split('$');
    if (parts.size() != 2) {
        // 如果不是哈希格式，可能是旧密码（明文），直接比较
        // 这是为了兼容迁移过程中的验证
        return password == hashedPassword;
    }
    
    QByteArray salt = fromBase64(parts[0]);
    QByteArray storedHash = fromBase64(parts[1]);
    
    // 计算输入密码的哈希
    QByteArray combined = salt + password.toUtf8();
    QByteArray computedHash = sha256(combined);
    
    // 使用恒定时间比较，防止时序攻击
    if (storedHash.size() != computedHash.size()) {
        return false;
    }
    
    int result = 0;
    for (int i = 0; i < storedHash.size(); ++i) {
        result |= (storedHash[i] ^ computedHash[i]);
    }
    
    return result == 0;
}

bool PasswordHasher::isHashed(const QString &str)
{
    // 检查是否是哈希格式 (salt$hash)
    QStringList parts = str.split('$');
    if (parts.size() != 2) {
        return false;
    }
    
    // 检查两部分都是有效的 Base64
    QByteArray salt = fromBase64(parts[0]);
    QByteArray hash = fromBase64(parts[1]);
    
    return !salt.isEmpty() && hash.size() == 32;  // SHA-256 输出 32 字节
}

QByteArray PasswordHasher::generateSalt()
{
    QByteArray salt(SALT_LENGTH, 0);
    QRandomGenerator *rng = QRandomGenerator::global();
    
    for (int i = 0; i < SALT_LENGTH; ++i) {
        salt[i] = static_cast<char>(rng->bounded(256));
    }
    
    return salt;
}

QByteArray PasswordHasher::sha256(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QString PasswordHasher::toBase64(const QByteArray &data)
{
    return data.toBase64();
}

QByteArray PasswordHasher::fromBase64(const QString &str)
{
    return QByteArray::fromBase64(str.toUtf8());
}